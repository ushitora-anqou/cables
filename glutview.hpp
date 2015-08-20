#pragma once
#ifndef ___GLUTVIEW_HPP___
#define ___GLUTVIEW_HPP___

#include "view.hpp"
#include "glut.hpp"
#include "helper.hpp"
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

class GlutView : public View, public glut::Window
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

    /*
	const int
        LEVEL_METER_DB_MIN = -60,
        PIXEL_PER_DB = 8,
        LEVEL_METER_BAR_LENGTH = -LEVEL_METER_DB_MIN * PIXEL_PER_DB, 
        LEVEL_METER_BAR_HEIGHT = 25,
        LEVEL_METER_LEFT_MARGIN = 50,
        LEVEL_METER_RIGHT_MARGIN = 50,
        WINDOW_WIDTH = LEVEL_METER_LEFT_MARGIN + LEVEL_METER_BAR_LENGTH + LEVEL_METER_RIGHT_MARGIN;
        */

    int groupMask_;
    boost::mutex mtx_;
    std::vector<std::pair<double, double>> mtxedDBLevels_;

private:
    void displayFunc() override;
	void reshapeFunc(int w, int h) override;
	void keyboardFunc(unsigned char key, int x, int y) override;
	void keyboardUpFunc(unsigned char key, int x, int y) override;
	void timerFunc(int idx) override;
	void closeFunc() override;

    // native draw functions
    void applyColor(const Color& color);
	template<class Render>
	void draw(Render render)
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

public:
    GlutView(int groupSize);
    ~GlutView(){}

    void updateLevelMeter(int index, const PCMWave::Sample& sample);
};

#endif
