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
	auto db = std::make_pair(
        std::max(static_cast<double>(LEVEL_METER_DB_MIN),
            sample.left  == 0 ? LEVEL_METER_DB_MIN : calcDB(sample.left)),
        std::max(static_cast<double>(LEVEL_METER_DB_MIN),
            sample.right == 0 ? LEVEL_METER_DB_MIN : calcDB(sample.right)));

	boost::mutex::scoped_lock lock(mtx_);
	data_.at(index).level = db;
}

void GlutView::applyColor(const Color& color)
{
    glColor3d(divd(color.r, 0xff), divd(color.g, 0xff), divd(color.b, 0xff));
}

void GlutView::drawLine(double x0, double y0, double x1, double y1, const Color& color)
{
    applyColor(color);
	glBegin(GL_LINE_LOOP);
	glVertex2d(x0, y0);
	glVertex2d(x1, y1);
	glEnd();
}

void GlutView::drawRectangle(const Rect& rect, const Color& color)
{
    applyColor(color);
	glBegin(GL_QUADS);
	glVertex2d(rect.left, rect.top);
	glVertex2d(rect.left, rect.bottom);
	glVertex2d(rect.right, rect.bottom);
	glVertex2d(rect.right, rect.top);
	glEnd();
}

void GlutView::drawString(double x0, double y0, const std::string& msg, const Color& color)
{
    applyColor(color);
    glRasterPos2d(x0, y0);
    glutBitmapString(GLUT_BITMAP_HELVETICA_12, reinterpret_cast<const unsigned char *>(msg.c_str()));
}

void GlutView::drawLevelMeterBox(int index, int ch, double widthRatio, const Color& color)
{
    const int
        x = LEVEL_METER_LEFT_MARGIN,
        y = calcY(index, ch),
        w = LEVEL_METER_BAR_LENGTH * widthRatio;
    drawRectangle(Rect(x, y, x + w, y + LEVEL_METER_BAR_HEIGHT), color);
}

void GlutView::drawLevelMeterBack(int index, double widthRatio, const Color& color)
{
    drawLevelMeterBox(index, 0, widthRatio, color);
    drawLevelMeterBox(index, 1, widthRatio, color);
}

void GlutView::drawSelectedUnitMark(int index)
{
    const int
        x = 0,
        y = calcY(index),
        w = LEVEL_METER_LEFT_MARGIN;
    drawRectangle(Rect(x, y, x + w, y + LEVEL_METER_BAR_HEIGHT * 2), Color::yellow());
}

void GlutView::drawUnitString(int index, int x, const std::string& msg, const Color& color)
{
    drawString(x, calcY(index, 1) + 6, msg, color);
}

void GlutView::displayFunc()
{
	draw([this]() {
		boost::mutex::scoped_lock lock(mtx_);

        indexedForeach(data_, [this](int i, GroupMutexedData& data) {
            // draw level meter
            auto& ldb = data.level.first, rdb = data.level.second;
            // background
            drawLevelMeterBack(i,   1, Color::magenta());
            drawLevelMeterBack(i, 0.6, Color::cyan());
            // meter
            drawLevelMeterBox(i, 0, 1 - ldb / LEVEL_METER_DB_MIN, Color::green());
            drawLevelMeterBox(i, 1, 1 - rdb / LEVEL_METER_DB_MIN, Color::green());

            // alive
            if(!getGroupInfo(i).mic.lock()->isAlive())
                drawLevelMeterBack(i, 1, Color::gray());

            // selected unit highlight
            if(groupMask_ & (1 << i))   drawSelectedUnitMark(i);

            // volume
            drawUnitString(i, WINDOW_WIDTH - LEVEL_METER_RIGHT_MARGIN + 20,
                boost::lexical_cast<std::string>(getGroupInfo(i).volume.lock()->getRate()),
                Color::black());

            // name
            drawUnitString(i, LEVEL_METER_LEFT_MARGIN, getGroupInfo(i).name, Color::black());
		});
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


