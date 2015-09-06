#pragma once
#ifndef ___GLUTVIEW_HPP___
#define ___GLUTVIEW_HPP___

#include "glut.hpp"
#include "pcmwave.hpp"
#include <boost/thread.hpp>
#include <atomic>
#include <memory>
#include <vector>

#include "helper.hpp"
#include "calc.hpp"
#include "error.hpp"
#include <boost/lexical_cast.hpp>


struct Rect
{
	double left, top, right, bottom;

	Rect(){}
	Rect(double left_, double top_, double right_, double bottom_)
		: left(left_), top(top_), right(right_), bottom(bottom_)
	{}
};

struct Color
{
	int r, g, b;

	Color(){}
    Color(int r_, int g_, int b_)
        : r(r_), g(g_), b(b_)
    {}

    static Color black()   { return Color(  0,   0,   0); }
    static Color white()   { return Color(255, 255, 255); }
    static Color red()     { return Color(255,   0,   0); }
    static Color green()   { return Color(  0, 255,   0); }
    static Color blue()    { return Color(  0,   0, 255); }
    static Color yellow()  { return Color(255, 255,   0); }
    static Color cyan()    { return Color(  0, 255, 255); }
    static Color magenta() { return Color(255,   0, 255); }
    static Color gray()    { return Color(128, 128, 128); }
};

class GlutViewSystem
{
private:
    bool hasFinished_;

private:
    GlutViewSystem();
    ~GlutViewSystem();
    // non-copyable
    GlutViewSystem(const GlutViewSystem&);
    GlutViewSystem& operator=(const GlutViewSystem&);

public:
    static GlutViewSystem& getInstance()    // maybe not thread safe
    {
        static GlutViewSystem instance;
        return instance;
    }
    void run();
    void stop();
};

template<class Group>
class GlutView : public glut::Window
{
private:
    const int
        LEVEL_METER_DB_MIN = -60,
        PIXEL_PER_DB = 4,

        UNIT_HEIGHT = 120,

        SELECTED_MARK_POS_X = 0,
        SELECTED_MARK_WIDTH = 50,

        LEVEL_METER_POS_X = SELECTED_MARK_WIDTH,
        LEVEL_METER_WIDTH = -LEVEL_METER_DB_MIN * PIXEL_PER_DB, 
        LEVEL_METER_HEIGHT = UNIT_HEIGHT / 2,

        OPTIONAL_POS_X = LEVEL_METER_POS_X + LEVEL_METER_WIDTH,
        OPTIONAL_UNIT_WIDTH = 50;

    int groupMask_;

    boost::mutex mtx_;
    std::vector<std::shared_ptr<Group>> groupInfoList_;

protected:
    void displayFunc() override;
	void reshapeFunc(int w, int h) override;
	void keyboardFunc(unsigned char key, int x, int y) override;
	void keyboardUpFunc(unsigned char key, int x, int y) override;
    void mouseFunc(int button, int state, int x, int y) override;
	void timerFunc(int idx) override;
	void closeFunc() override;

    // native draw functions
    void applyColor(const Color& color);
	template<class Render>
	void drawScoped(Render render)
	{
		glClear(GL_COLOR_BUFFER_BIT);
		render();
		swapBuffers();
	}
	void drawLine(double x0, double y0, double x1, double y1, const Color& color);
	void drawRectangle(const Rect& rect, const Color& color);
    void drawString(double x0, double y0, const std::string& msg, const Color& color);

    // applicative draw functions
    int calcY(int index, int ch = 0) { return LEVEL_METER_HEIGHT * (index * 2 + ch); }
    void drawLevelMeterBox(int index, int ch, double widthRatio, const Color& color);
    void drawLevelMeterBack(int index, double widthRatio, const Color& color);
    void drawSelectedUnitMark(int index);
    void drawUnitString(int index, int x, const std::string& msg, const Color& color);
    void drawLevelMeter(int i, const std::shared_ptr<Group>& groupInfo);
    int calcIndexFromXY(int x, int y);

protected:
    virtual void draw(const std::vector<std::shared_ptr<Group>>& groups){}
    virtual void draw(int index, const std::shared_ptr<Group>& groupInfo){}

    virtual void keyDown(const std::vector<std::shared_ptr<Group>>& groups, unsigned char key){}
    virtual void keyDown(int index, const std::shared_ptr<Group>& groupInfo, unsigned char key){}
    virtual void keyUp(const std::vector<std::shared_ptr<Group>>& groups, unsigned char key){}
    virtual void keyUp(int index, const std::shared_ptr<Group>& groupInfo, unsigned char key){}

    virtual void clickLeftDown(const std::vector<std::shared_ptr<Group>>& groups, int x, int y){}
    virtual void clickLeftUp(const std::vector<std::shared_ptr<Group>>& groups, int x, int y){}
    virtual void clickRightDown(const std::vector<std::shared_ptr<Group>>& groups, int x, int y){}
    virtual void clickRightUp(const std::vector<std::shared_ptr<Group>>& groups, int x, int y){}
    virtual void wheelUp(const std::vector<std::shared_ptr<Group>>& groups, int x, int y){}
    virtual void wheelDown(const std::vector<std::shared_ptr<Group>>& groups, int x, int y){}

public:
    GlutView(const std::string& title);
    virtual ~GlutView(){}

    void addGroup(const std::shared_ptr<Group>& info);
};

///

inline GlutViewSystem::GlutViewSystem()
    : hasFinished_(true)
{
    char *dummyArgv[] = {"dummy_name"};
    glut::Init(1, dummyArgv);
    glut::InitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
}

inline GlutViewSystem::~GlutViewSystem()
{
}

inline void GlutViewSystem::run()
{
    ZARU_THROW_UNLESS(hasFinished_ == true);
    hasFinished_ = false;
    while(!hasFinished_){
        glutMainLoopEvent();
    }
}

inline void GlutViewSystem::stop()
{
    hasFinished_ = true;
}

///

template<class Group>
GlutView<Group>::GlutView(const std::string& title)
    : glut::Window(title.c_str(), 640, 480), groupMask_(0)
{
    createTimer(50, 0);
    show();
    glClearColor(1.0, 1.0, 1.0, 1.0);
	glut::IgnoreKeyRepeat(GL_TRUE);
	//glut::SetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);
	//glut::SetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);


}

template<class Group>
void GlutView<Group>::addGroup(const std::shared_ptr<Group>& info)
{
    SCOPED_LOCK(mtx_);
    groupInfoList_.push_back(info);
}

template<class Group>
void GlutView<Group>::applyColor(const Color& color)
{
    glColor3d(divd(color.r, 0xff), divd(color.g, 0xff), divd(color.b, 0xff));
}

template<class Group>
void GlutView<Group>::drawLine(double x0, double y0, double x1, double y1, const Color& color)
{
    applyColor(color);
	glBegin(GL_LINE_LOOP);
	glVertex2d(x0, y0);
	glVertex2d(x1, y1);
	glEnd();
}

template<class Group>
void GlutView<Group>::drawRectangle(const Rect& rect, const Color& color)
{
    applyColor(color);
	glBegin(GL_QUADS);
	glVertex2d(rect.left, rect.top);
	glVertex2d(rect.left, rect.bottom);
	glVertex2d(rect.right, rect.bottom);
	glVertex2d(rect.right, rect.top);
	glEnd();
}

template<class Group>
void GlutView<Group>::drawString(double x0, double y0, const std::string& msg, const Color& color)
{
    applyColor(color);
    glRasterPos2d(x0, y0);
    glutBitmapString(GLUT_BITMAP_HELVETICA_12, reinterpret_cast<const unsigned char *>(msg.c_str()));
}

template<class Group>
void GlutView<Group>::drawLevelMeterBox(int index, int ch, double widthRatio, const Color& color)
{
    if(widthRatio < 0)  return;
    const int
        x = LEVEL_METER_POS_X,
        y = calcY(index, ch),
        w = LEVEL_METER_WIDTH * widthRatio,
        h = LEVEL_METER_HEIGHT;
    drawRectangle(Rect(x, y, x + w, y + LEVEL_METER_HEIGHT), color);
}

template<class Group>
void GlutView<Group>::drawLevelMeterBack(int index, double widthRatio, const Color& color)
{
    drawLevelMeterBox(index, 0, widthRatio, color);
    drawLevelMeterBox(index, 1, widthRatio, color);
}

template<class Group>
void GlutView<Group>::drawSelectedUnitMark(int index)
{
    const int
        x = SELECTED_MARK_POS_X,
        y = calcY(index),
        w = SELECTED_MARK_WIDTH,
        h = UNIT_HEIGHT;
    drawRectangle(Rect(x, y, x + w, y + h), Color::yellow());
}

template<class Group>
void GlutView<Group>::drawUnitString(int index, int x, const std::string& msg, const Color& color)
{
    drawString(x, calcY(index, 1) + 6, msg, color);
}

template<class Group>
void GlutView<Group>::drawLevelMeter(int i, const std::shared_ptr<Group>& groupInfo)
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

template<class Group>
int GlutView<Group>::calcIndexFromXY(int x, int y)
{
    return y / UNIT_HEIGHT;
}

template<class Group>
void GlutView<Group>::displayFunc()
{
	drawScoped([this]() {
        SCOPED_LOCK(mtx_);

        indexedForeach(groupInfoList_, [this](int i, const std::shared_ptr<Group>& groupInfo) {
            draw(i, groupInfo);
		});
        draw(groupInfoList_);
	});
}

template<class Group>
void GlutView<Group>::reshapeFunc(int w, int h)
{
	glViewport(0, 0, w, h);
	glLoadIdentity();
	glOrtho(-0.5, (GLdouble)w - 0.5, (GLdouble)h - 0.5, -0.5, -1.0, 1.0);
}

template<class Group>
void GlutView<Group>::keyboardFunc(unsigned char key, int x, int y)
{
    // change unit mask
    if('1' <= key && key <= '9'){
        groupMask_ |= (1 << (key - '0' - 1));
        return;
    }

    {
        SCOPED_LOCK(mtx_);
        indexedForeach(groupInfoList_, [this, key](int i, const std::shared_ptr<Group>& groupInfo) {
            // not continue. just in a lambda 
            if(!(groupMask_ & (1 << i)))    return;
            keyDown(i, groupInfo, key);
        });
        keyDown(groupInfoList_, key);
    }
}

template<class Group>
void GlutView<Group>::keyboardUpFunc(unsigned char key, int x, int y)
{
    if('1' <= key && key <= '9'){   // unit mask
        groupMask_ &= ~(1 << (key - '0' - 1));
        return;
    }

    {
        SCOPED_LOCK(mtx_);
        indexedForeach(groupInfoList_, [this, key](int i, const std::shared_ptr<Group>& groupInfo) {
            // not continue. just in a lambda 
            if(!(groupMask_ & (1 << i)))    return;
            keyUp(i, groupInfo, key);
        });
        keyUp(groupInfoList_, key);
    }
}

template<class Group>
void GlutView<Group>::mouseFunc(int button, int state, int x, int y)
{
    SCOPED_LOCK(mtx_);

    switch(button)
    {
    case GLUT_LEFT_BUTTON:
        if(state == GLUT_UP)    clickLeftUp(groupInfoList_, x, y);
        if(state == GLUT_DOWN)  clickLeftDown(groupInfoList_, x, y);    break;
    case GLUT_RIGHT_BUTTON:
        if(state == GLUT_UP)    clickRightUp(groupInfoList_, x, y);
        if(state == GLUT_DOWN)  clickRightDown(groupInfoList_, x, y);   break;
    case 3: wheelUp(groupInfoList_, x, y);      break;
    case 4: wheelDown(groupInfoList_, x, y);    break;
    }
}

template<class Group>
void GlutView<Group>::timerFunc(int idx)
{
	postRedisplay();
	createTimer(50, 0);
}

template<class Group>
void GlutView<Group>::closeFunc()
{
}

#endif
