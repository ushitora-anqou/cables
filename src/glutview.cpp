#include "glutview.hpp"
#include <boost/lexical_cast.hpp>

GlutViewSystem::GlutViewSystem()
    : hasFinished_(true)
{
    char *dummyArgv[] = {"dummy_name"};
    glut::Init(1, dummyArgv);
    glut::InitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
}

GlutViewSystem::~GlutViewSystem()
{
}

void GlutViewSystem::run()
{
    assert(hasFinished_ == true);
    hasFinished_ = false;
    while(!hasFinished_){
        glutMainLoopEvent();
    }
}

void GlutViewSystem::stop()
{
    hasFinished_ = true;
}

///

GlutView::GlutView(const std::string& title)
    : glut::Window(title.c_str(), 640, 480), groupMask_(0)
{
    createTimer(50, 0);
    show();
    glClearColor(1.0, 1.0, 1.0, 1.0);
	glut::IgnoreKeyRepeat(GL_TRUE);
	//glut::SetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);
	glut::SetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);


}

void GlutView::addGroup(const GroupPtr& info)
{
    SCOPED_LOCK(mtx_);
    groupInfoList_.push_back(info);
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
    if(widthRatio < 0)  return;
    const int
        x = LEVEL_METER_POS_X,
        y = calcY(index, ch),
        w = LEVEL_METER_WIDTH * widthRatio,
        h = LEVEL_METER_HEIGHT;
    drawRectangle(Rect(x, y, x + w, y + LEVEL_METER_HEIGHT), color);
}

void GlutView::drawLevelMeterBack(int index, double widthRatio, const Color& color)
{
    drawLevelMeterBox(index, 0, widthRatio, color);
    drawLevelMeterBox(index, 1, widthRatio, color);
}

void GlutView::drawSelectedUnitMark(int index)
{
    const int
        x = SELECTED_MARK_POS_X,
        y = calcY(index),
        w = SELECTED_MARK_WIDTH,
        h = UNIT_HEIGHT;
    drawRectangle(Rect(x, y, x + w, y + h), Color::yellow());
}

void GlutView::drawUnitString(int index, int x, const std::string& msg, const Color& color)
{
    drawString(x, calcY(index, 1) + 6, msg, color);
}

void GlutView::drawLevelMeter(int i, const GroupPtr& groupInfo)
{
    auto db = groupInfo->getWaveLevel();
    double ldb = std::isinf(db.first) ? LEVEL_METER_DB_MIN : db.first,
           rdb = std::isinf(db.second) ? LEVEL_METER_DB_MIN : db.second;

    // background
    drawLevelMeterBack(i,   1, Color::magenta());
    drawLevelMeterBack(i, 0.9, Color::cyan());
    // meter
    drawLevelMeterBox(i, 0, 1 - ldb / LEVEL_METER_DB_MIN, Color::green());
    drawLevelMeterBox(i, 1, 1 - rdb / LEVEL_METER_DB_MIN, Color::green());

    // alive
    if(!groupInfo->isAlive())
        drawLevelMeterBack(i, 1, Color::gray());

    // selected unit highlight
    if(groupMask_ & (1 << i))   drawSelectedUnitMark(i);

    // draw optional data
    auto optData = std::move(groupInfo->createOptionalInfo());
    indexedForeach(optData, [this, i](int oi, const std::string& msg) {
        drawUnitString(i, OPTIONAL_POS_X + OPTIONAL_UNIT_WIDTH * oi,
            msg, Color::black());
    });

    // name
    drawUnitString(i, LEVEL_METER_POS_X,
        groupInfo->createName(), Color::black());
}

void GlutView::displayFunc()
{
	drawScoped([this]() {
        SCOPED_LOCK(mtx_);

        indexedForeach(groupInfoList_, [this](int i, const GroupPtr& groupInfo) {
            draw(i, groupInfo);
		});
        draw();
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

    {
        SCOPED_LOCK(mtx_);
        indexedForeach(groupInfoList_, [this, key](int i, const GroupPtr& groupInfo) {
            // not continue. just in a lambda 
            if(!(groupMask_ & (1 << i)))    return;
            keyDown(i, groupInfo, key);
        });
        keyDown(key);
    }
}

void GlutView::keyboardUpFunc(unsigned char key, int x, int y)
{
    if('1' <= key && key <= '9'){   // unit mask
        groupMask_ &= ~(1 << (key - '0' - 1));
        return;
    }

    {
        SCOPED_LOCK(mtx_);
        indexedForeach(groupInfoList_, [this, key](int i, const GroupPtr& groupInfo) {
            // not continue. just in a lambda 
            if(!(groupMask_ & (1 << i)))    return;
            keyUp(i, groupInfo, key);
        });
        keyUp(key);
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


