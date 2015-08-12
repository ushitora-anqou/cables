#include "portaudio.hpp"
#include "helper.hpp"
#include "units.hpp"
#include <cmath>
#include <iostream>
#include <unordered_map>

#include "glutview.hpp"

class PrintFilter : public Unit
{
private:
	std::shared_ptr<View> view_;
	View::UID uid_;

public:
	PrintFilter(const std::shared_ptr<View>& view, const std::string& name)
		: view_(view), uid_(view->issueGroup(name))
	{}

	void inputImpl(const PCMWave& wave)
	{
		view_->updateLevelMeter(uid_, *wave.begin());
		send(wave);
	}
};


/*

#include <curses.h>

class PrintFilter : public Unit
{
private:
    WINDOW *win_;

private:
    void printLine(int length, const PCMWave::Sample& sample);

public:
    PrintFilter(WINDOW *win)
        : win_(win)
    {}
    ~PrintFilter(){}

    void inputImpl(const PCMWave& wave);
};

void PrintFilter::printLine(int length, const PCMWave::Sample& sample)
{
    clear();
    double al = std::abs(sample.left), ar = std::abs(sample.right),
           ldb = al == 0 ? length * 5 : 20 * std::log10(al),
           rdb = ar == 0 ? length * 5 : 20 * std::log10(ar);

    const double DB_PER_CHAR = 0.5;
    wmove(win_, 0, 0);  wclrtoeol(win_);
    wmove(win_, 1, 0);  wclrtoeol(win_);
    mvwaddch(win_, 0, 0, '*');
    mvwaddch(win_, 1, 0, '*');
    for(int i = 0;i < length;i++){
        if(ldb >= i * -DB_PER_CHAR)    mvwaddch(win_, 0, length - i, '*');
        if(rdb >= i * -DB_PER_CHAR)    mvwaddch(win_, 1, length - i, '*');
    }
    refresh();
}

void PrintFilter::inputImpl(const PCMWave& wave)
{
    printLine(120, *wave.begin());
    send(wave);
}

*/

void connect(const std::unordered_map<std::string, UnitPtr>& units, const std::vector<std::string>& from, const std::vector<std::string>& to)
{
    std::vector<UnitPtr> fromUnits, toUnits;
    for(auto& str : from)   fromUnits.push_back(units.at(str));
    for(auto& str : to)     toUnits.push_back(units.at(str));
    connect(fromUnits, toUnits);
}

/*
int main(int argc, char **argv)
{
	std::shared_ptr<ViewSystem> viewSystem = std::make_shared<GlutViewSystem>(argc, argv);
	auto view = viewSystem->createView();
	auto ptr = make_unique<boost::thread>([]() {
		std::string input;
		std::cin >> input;
	});
	glut::MainLoop();
	ptr->join();
	return 0;
}
*/

int main(int argc, char **argv)
{
    //assert(initscr() != NULL);

    std::shared_ptr<AudioSystem> system(std::make_shared<PAAudioSystem>());
	std::shared_ptr<ViewSystem> viewSystem = std::make_shared<GlutViewSystem>(argc, argv);

    std::unordered_map<std::string, UnitPtr> units = {
        {"mic", makeUnit<MicOutUnit>(system->createInputStream(system->getDefaultInputDevice()))},
        //{"sin", makeUnit<SinOutUnit>(500)},
        //{"vfl", makeUnit<VolumeFilter>(0.5)},
        {"infile", makeUnit<FileInUnit>("test.wav")},
        {"spk", makeUnit<SpeakerOutUnit>(system->createOutputStream(system->getDefaultOutputDevice()))},
        //{"pfl", makeUnit<PrintFilter>(stdscr)},
        {"pfl", makeUnit<PrintFilter>(viewSystem->createView(), "pfl")},
        //{"pmp", makeUnit<PumpOutUnit>()},
    };
    //connect({units.at("mic")}, {units.at("infile"), units.at("spk")});
    //connect({units.at("sin")}, {units.at("vfl")});
    //connect({units.at("vfl")}, {units.at("infile"), units.at("spk")});
    //connect(units, {"mic"}, {"infile", "spk"});
    //connect(units, {"sin", "pmp"}, {"vfl"});
    //connect(units, {"vfl"}, {"infile", "spk"});
    //connect(units, {"pfl"}, {"spk"});
	connect({units.at("mic")}, {units.at("pfl")});
	connect({units.at("pfl")}, {units.at("infile"), units.at("spk")});
    //connect(units, {"sin"}, {"vfl", "spk"});
    //connect(units, {"vfl"}, {"vfl", "spk"});

	//auto view = viewSystem->createView();
    for(auto& unit : units) unit.second->start();

	/*
	auto ptr = make_unique<boost::thread>([]() {
		std::string input;
		std::cin >> input;
	});
	*/

	viewSystem->run();
    //units.at("sin")->stop();

    //while(true)
    {
        //std::string input;
        //std::cin >> input;
        //if(input == "quit")  break;
    }

    for(auto& unit : units){
        if(unit.second->isAlive())  unit.second->stop();
    }

    //endwin();

    return 0;
}
