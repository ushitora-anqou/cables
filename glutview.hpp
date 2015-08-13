#pragma once
#ifndef ___GLUTVIEW_HPP___
#define ___GLUTVIEW_HPP___

#include "view.hpp"
#include "glut.hpp"
#include <boost/thread.hpp>

class UnitManager;

class GlutViewSystem : public ViewSystem
{
private:
	static bool isFirst_;

public:
	GlutViewSystem(int argc, char **argv);
	~GlutViewSystem();

	ViewPtr createView(int groupSize);
	void run();
};

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
	double red, green, blue;

	Color(){}
    Color(int red_, int green_, int blue_)  // max : 255(0xff)
        : red(red_ / 255.0), green(green_ / 255.0), blue(blue_ / 255.0)
    {}
	Color(double red_, double green_, double blue_) // max : 1.0
		: red(red_), green(green_), blue(blue_)
	{}

	void operator()() const
	{
		glColor3d(red, green, blue);
	}
};

class GlutView : public View, public glut::Window
{
private:
	const double LEVEL_METER_DB_MIN = -60;

    struct GroupMutexedData
    {
		std::pair<double, double> level;
    };
    std::vector<GroupMutexedData> data_;
    int groupMask_;
    boost::mutex mtx_;

private:
	template<class Render>
	void draw(Render render)
	{
		glClear(GL_COLOR_BUFFER_BIT);
		render();
		swapBuffers();
	}
	void drawLine(double x0, double y0, double x1, double y1, const Color& color);
	void drawRectangle(const Rect& rect, const Color& color);
    void drawString(double x0, double y0, const Color& color, const std::string& msg);

	void displayFunc() override;
	void reshapeFunc(int w, int h) override;
	void keyboardFunc(unsigned char key, int x, int y) override;
	void keyboardUpFunc(unsigned char key, int x, int y) override;
	void timerFunc(int idx) override;
	void closeFunc() override;

public:
    GlutView(int groupSize);
    ~GlutView(){}

    void updateLevelMeter(int index, const PCMWave::Sample& sample) override;
};

#endif
