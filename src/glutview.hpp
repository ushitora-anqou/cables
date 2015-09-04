#pragma once
#ifndef ___GLUTVIEW_HPP___
#define ___GLUTVIEW_HPP___

#include "glut.hpp"
#include "pcmwave.hpp"
#include "group.hpp"
#include <boost/thread.hpp>
#include <atomic>
#include <memory>
#include <vector>

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

class GlutView;

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

/*
class GlutViewSystem
{
private:
	static bool isFirst_;
    bool hasFinished_;

public:
	GlutViewSystem(int argc, char **argv);
	~GlutViewSystem();

	void run();
};
*/

class GlutView : public glut::Window
{
private:
    const int
        LEVEL_METER_DB_MIN = -60,
        PIXEL_PER_DB = 8,

        UNIT_HEIGHT = 50,

        SELECTED_MARK_POS_X = 0,
        SELECTED_MARK_WIDTH = 50,

        LEVEL_METER_POS_X = SELECTED_MARK_WIDTH,
        LEVEL_METER_WIDTH = -LEVEL_METER_DB_MIN * PIXEL_PER_DB, 
        LEVEL_METER_HEIGHT = UNIT_HEIGHT / 2,

        OPTIONAL_POS_X = LEVEL_METER_POS_X + LEVEL_METER_WIDTH,
        OPTIONAL_UNIT_WIDTH = 50;

    int groupMask_;

    boost::mutex mtx_;
    std::vector<GroupPtr> groupInfoList_;

protected:
    void displayFunc() override;
	void reshapeFunc(int w, int h) override;
	void keyboardFunc(unsigned char key, int x, int y) override;
	void keyboardUpFunc(unsigned char key, int x, int y) override;
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
    void drawLevelMeter(int i, const GroupPtr& groupInfo);

protected:
    virtual void draw(const std::vector<GroupPtr>& groups){}
    virtual void draw(int index, const GroupPtr& groupInfo){}

    virtual void keyDown(const std::vector<GroupPtr>& groups, unsigned char key){}
    virtual void keyDown(int index, const GroupPtr& groupInfo, unsigned char key){}
    virtual void keyUp(const std::vector<GroupPtr>& groups, unsigned char key){}
    virtual void keyUp(int index, const GroupPtr& groupInfo, unsigned char key){}

public:
    GlutView(const std::string& title);
    virtual ~GlutView(){}

    void addGroup(const GroupPtr& info);
};

#endif
