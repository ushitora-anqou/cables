#pragma once
#ifndef ___GLUTVIEW_HPP___
#define ___GLUTVIEW_HPP___

#include "glut.hpp"
#include "pcmwave.hpp"
#include "helper.hpp"
#include "group.hpp"
#include <boost/thread.hpp>
#include <atomic>
#include <memory>
#include <vector>

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
