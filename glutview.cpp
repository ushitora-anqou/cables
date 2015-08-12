#include "glutview.hpp"
#include "helper.hpp"

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

ViewPtr GlutViewSystem::createView()
{
	auto view = std::make_shared<GlutView>();
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

View::UID GlutView::issueGroup(const std::string& name)
{
	boost::mutex::scoped_lock lock(mtx_);

	data_.emplace_back(name);
	return data_.size() - 1;
}

void GlutView::updateLevelMeter(View::UID id, const PCMWave::Sample& sample)
{
	double al = std::abs(sample.left), ar = std::abs(sample.right),
		   ldb = al == 0 ? LEVEL_METER_DB_MIN : 20 * log10(al),
		   rdb = ar == 0 ? LEVEL_METER_DB_MIN : 20 * log10(ar);
	auto db = std::make_pair(ldb, rdb);

	boost::mutex::scoped_lock lock(mtx_);
	data_.at(id).level = db;
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

void GlutView::displayFunc()
{
	draw([this]() {
		boost::mutex::scoped_lock lock(mtx_);

		static const double PIXEL_PER_DB = 4, LEVEL_METER_BAR_LENGTH = -LEVEL_METER_DB_MIN * PIXEL_PER_DB;
		static const int LINE_HEIGHT = 25;
		for(int i = 0;i < data_.size();i++){
			auto& ldb = data_.at(i).level.first, rdb = data_.at(i).level.second;
			const int y = i * LINE_HEIGHT;
			drawRectangle(Rect(0, y, LEVEL_METER_BAR_LENGTH, y + LINE_HEIGHT), Color(255, 0, 0));
			drawRectangle(Rect(0, y + LINE_HEIGHT, LEVEL_METER_BAR_LENGTH, y + LINE_HEIGHT * 2), Color(255, 0, 0));
			drawRectangle(Rect(0, y, LEVEL_METER_BAR_LENGTH * 0.6, y + LINE_HEIGHT), Color(0, 0, 255));
			drawRectangle(Rect(0, y + LINE_HEIGHT, LEVEL_METER_BAR_LENGTH * 0.6, y + LINE_HEIGHT * 2), Color(0, 0, 255));


			drawRectangle(Rect(
				0                                                      , y,
				LEVEL_METER_BAR_LENGTH * (1 - ldb / LEVEL_METER_DB_MIN), y + LINE_HEIGHT),
				Color(0, 255, 0));
			drawRectangle(Rect(
				0                                                      , y + LINE_HEIGHT,
				LEVEL_METER_BAR_LENGTH * (1 - rdb / LEVEL_METER_DB_MIN), y + LINE_HEIGHT * 2),
				Color(0, 255, 0));
			/*
			for(int x = 0;x < LEVEL_METER_BAR_LENGTH;x++){
				if(ldb >= (LEVEL_METER_BAR_LENGTH - x) * -DB_PER_PIXEL)
					drawLine(x, y, x, y + LINE_HEIGHT, Color(0, 255, 0));
				if(rdb >= (LEVEL_METER_BAR_LENGTH - x) * -DB_PER_PIXEL)
					drawLine(x, y + LINE_HEIGHT, x, y + LINE_HEIGHT * 2, Color(0, 255, 0));
			}
			*/
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
	switch(key)
	{
	case '\033':
        glutLeaveMainLoop();
		break;
	}
}

void GlutView::keyboardUpFunc(unsigned char key, int x, int y)
{
	switch(key)
	{
	case '\033':
		break;
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


