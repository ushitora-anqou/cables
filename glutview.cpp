#include "glutview.hpp"
#include "helper.hpp"
#include "unitmanager.hpp"
#include <boost/lexical_cast.hpp>

bool GlutViewSystem::isFirst_ = true;

GlutViewSystem::GlutViewSystem(int argc, char **argv)
{
	if(isFirst_){
		isFirst_ = false;
		glut::Init(argc, argv);
		glut::InitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	}
}

GlutViewSystem::~GlutViewSystem()
{
}

ViewPtr GlutViewSystem::createView(int viewSize)
{
	auto view = std::make_shared<GlutView>(viewSize);
	view->createTimer(50, 0);
	view->show();
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glut::IgnoreKeyRepeat(GL_TRUE);
	//glut::SetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);
	glut::SetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);

	return view;
}

void GlutViewSystem::run()
{
	glut::MainLoop();
	std::cout << "DEBUG" << std::endl;
}

///

GlutView::GlutView(int groupSize)
    : View(groupSize), glut::Window("recorder", 640, 480), data_(groupSize), groupMask_(0)
{}

void GlutView::updateLevelMeter(int index, const PCMWave::Sample& sample)
{
	double al = std::abs(sample.left), ar = std::abs(sample.right),
		   ldb = al == 0 ? LEVEL_METER_DB_MIN : 20 * log10(al),
		   rdb = ar == 0 ? LEVEL_METER_DB_MIN : 20 * log10(ar);
	auto db = std::make_pair(ldb, rdb);

	boost::mutex::scoped_lock lock(mtx_);
	data_.at(index).level = db;
}

void GlutView::drawLine(double x0, double y0, double x1, double y1, const Color& color)
{
	color();
	glBegin(GL_LINE_LOOP);
	glVertex2d(x0, y0);
	glVertex2d(x1, y1);
	glEnd();
}

void GlutView::drawRectangle(const Rect& rect, const Color& color)
{
	color();
	glBegin(GL_QUADS);
	glVertex2d(rect.left, rect.top);
	glVertex2d(rect.left, rect.bottom);
	glVertex2d(rect.right, rect.bottom);
	glVertex2d(rect.right, rect.top);
	glEnd();
}

void GlutView::drawString(double x0, double y0, const Color& color, const std::string& msg)
{
    color();
    glRasterPos2d(x0, y0);
    glutBitmapString(GLUT_BITMAP_HELVETICA_12, reinterpret_cast<const unsigned char *>(msg.c_str()));
}

void GlutView::displayFunc()
{
	draw([this]() {
		boost::mutex::scoped_lock lock(mtx_);

		static const double PIXEL_PER_DB = 8, LEVEL_METER_BAR_LENGTH = -LEVEL_METER_DB_MIN * PIXEL_PER_DB;
		static const int LINE_HEIGHT = 25;
		for(int i = 0;i < data_.size();i++){
            auto& data = data_.at(i);
			auto& ldb = data.level.first, rdb = data.level.second;
			const int y = i * LINE_HEIGHT * 2;
			drawRectangle(Rect(0, y, LEVEL_METER_BAR_LENGTH, y + LINE_HEIGHT * 2), Color(255, 0, 255));
			drawRectangle(Rect(0, y, LEVEL_METER_BAR_LENGTH * 0.6, y + LINE_HEIGHT * 2), Color(0, 255, 255));

			drawRectangle(Rect(
				0                                                      , y,
				LEVEL_METER_BAR_LENGTH * (1 - ldb / LEVEL_METER_DB_MIN), y + LINE_HEIGHT),
				Color(0, 255, 0));
			drawRectangle(Rect(
				0                                                      , y + LINE_HEIGHT,
				LEVEL_METER_BAR_LENGTH * (1 - rdb / LEVEL_METER_DB_MIN), y + LINE_HEIGHT * 2),
				Color(0, 255, 0));

            if(groupMask_ & (1 << i))
                drawRectangle(Rect(LEVEL_METER_BAR_LENGTH + 10, y, LEVEL_METER_BAR_LENGTH + 50, y + LINE_HEIGHT * 2), Color(255, 255, 0));

            drawString(LEVEL_METER_BAR_LENGTH + 20, y + LINE_HEIGHT + 6, Color(0, 0, 0),
                boost::lexical_cast<std::string>(getGroupInfo(i).volume.lock()->getRate()));

            if(!getGroupInfo(i).mic.lock()->isAlive())
                drawRectangle(Rect(0, y, LEVEL_METER_BAR_LENGTH, y + LINE_HEIGHT * 2), Color(192, 192, 192));

            drawString(0, y + LINE_HEIGHT + 6, Color(0, 0, 0), getGroupInfo(i).name);
		}
	});
}

void GlutView::reshapeFunc(int w, int h)
{
	glViewport(0, 0, w, h);
	glLoadIdentity();
	glOrtho(-0.5, (GLdouble)w - 0.5, (GLdouble)h - 0.5, -0.5, -1.0, 1.0);
}

void GlutView::keyboardFunc(unsigned char key, int x, int y)
{
    // change unit mask
    if('1' <= key && key <= '9'){
        groupMask_ |= (1 << (key - '0' - 1));
        return;
    }

    // group process
    static const std::unordered_map<unsigned char, boost::function<void(int)>> groupProcs = {
        {'o'   , [this](int index) { getGroupInfo(index).volume.lock()->addRate( 5); }},
        {'l'   , [this](int index) { getGroupInfo(index).volume.lock()->addRate(-5); }},
        {'s'   , [this](int index) {
            auto& mic = *getGroupInfo(index).mic.lock();
            if(mic.isAlive())   mic.stop();
            else    mic.start();
        }},
    };
    auto it = groupProcs.find(key);
    if(it != groupProcs.end()){
        for(int i = 0;i < data_.size();i++){
            if(!(groupMask_ & (1 << i)))    continue;
            it->second(i);
        }
        return;
    }

    // global process
	switch(key)
	{
	case '\033':    // ESC
        glutLeaveMainLoop();
        break;
    }
}

void GlutView::keyboardUpFunc(unsigned char key, int x, int y)
{
    if('1' <= key && key <= '9'){   // unit mask
        groupMask_ &= ~(1 << (key - '0' - 1));
        return;
    }
}

void GlutView::timerFunc(int idx)
{
	postRedisplay();
	createTimer(50, 0);
}

void GlutView::closeFunc()
{
}


