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
    UnitManager& unitManager_;

public:
	GlutViewSystem(UnitManager& unitManager, int argc, char **argv);
	~GlutViewSystem();

	ViewPtr createView();
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
	Color(double red_, double green_, double blue_)
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

    UnitManager& unitManager_;

    struct GroupData
    {
        std::string name;
		std::pair<double, double> level;

		GroupData(const std::string& name_)
			: name(name_)
		{}
    };
    std::vector<GroupData> data_;
    boost::mutex mtx_;
	bool hasFinished_;

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

	void displayFunc() override;
	void reshapeFunc(int w, int h) override;
	void keyboardFunc(unsigned char key, int x, int y) override;
	void keyboardUpFunc(unsigned char key, int x, int y) override;
	void timerFunc(int idx) override;
	void closeFunc() override;

public:
    GlutView(UnitManager& unitManager);
    ~GlutView(){}

    UID issueGroup(const std::string& name) override;
    void updateLevelMeter(UID id, const PCMWave::Sample& sample) override;
};

#endif
