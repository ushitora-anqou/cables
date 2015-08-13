#include "portaudio.hpp"
#include "helper.hpp"
#include "units.hpp"
#include "unitmanager.hpp"
#include "glutview.hpp"
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <cmath>
#include <iostream>
#include <unordered_map>


class PrintFilter : public Unit
{
private:
	std::shared_ptr<View> view_;
    int viewIndex_;

public:
	PrintFilter(const std::shared_ptr<View>& view, int index)
		: view_(view), viewIndex_(index)
	{}

	void inputImpl(const PCMWave& wave)
	{
		view_->updateLevelMeter(viewIndex_, *wave.begin());
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

/*
void connect(const std::unordered_map<std::string, UnitPtr>& units, const std::vector<std::string>& from, const std::vector<std::string>& to)
{
    std::vector<UnitPtr> fromUnits, toUnits;
    for(auto& str : from)   fromUnits.push_back(units.at(str));
    for(auto& str : to)     toUnits.push_back(units.at(str));
    connect(fromUnits, toUnits);
}
*/



int main(int argc, char **argv)
{
    std::shared_ptr<AudioSystem> system(std::make_shared<PAAudioSystem>());

    // User input
    auto devices = system->getValidDevices();
    indexedForeach(devices, [](int i, AudioDevicePtr& dev){
        std::cout <<
            i << ": " << dev->name() <<
            " (i:" << dev->inputChannel() <<
            " ,o:" << dev->outputChannel() <<
            ")" << std::endl;
    });
    std::vector<AudioDevicePtr> inputDevices, outputDevices;
    {
        std::cout << "Input indexes :" << std::flush;
        std::string input;  std::getline(std::cin, input);
        std::vector<std::string> indexTokens;
        boost::split(indexTokens, input, boost::is_space());
        for(auto& token : indexTokens)
            inputDevices.push_back(devices.at(boost::lexical_cast<int>(token)));

    }
    {
        std::cout << "Output indexes :" << std::flush;
        std::string input;  std::getline(std::cin, input);
        std::vector<std::string> indexTokens;
        boost::split(indexTokens, input, boost::is_space());
        for(auto& token : indexTokens)
            outputDevices.push_back(devices.at(boost::lexical_cast<int>(token)));
    }

    // make and connect units
    UnitManager manager;

    // make input unit e.g. speaker
    std::vector<std::string> inputUnitNames;
    indexedForeach(outputDevices, [&system, &manager, &inputUnitNames](int i, const AudioDevicePtr& dev) {
        const std::string speakerName = "spk_" + dev->name();
        manager.makeUnit<SpeakerInUnit>(speakerName, system->createOutputStream(dev));
        inputUnitNames.push_back(speakerName);
    });

    // make output unit and group
	std::shared_ptr<ViewSystem> viewSystem(std::make_shared<GlutViewSystem>(argc, argv));
    auto view = viewSystem->createView(inputDevices.size());
    std::vector<std::string> outputUnitNames;
    indexedForeach(inputDevices, [&system, &manager, &view, &outputUnitNames](int i, const AudioDevicePtr& dev) {
        const std::string&
            devName = boost::regex_replace(dev->name(), boost::regex("\\s+"), "_", boost::format_all),
            micName = "mic_" + devName,
            volumeName = "vol_" + devName,
            printName = "pfl_" + devName,
            fileName = "wfl_" + devName,
            pumpName = "pmp_" + devName;

        manager.makeUnit<MicOutUnit>(micName, system->createInputStream(dev));
        manager.makeUnit<VolumeFilter>(volumeName);
        manager.makeUnit<PrintFilter>(printName, view, i);
        manager.makeUnit<FileInUnit>(fileName, fileName + ".wav");
        manager.makeUnit<PumpOutUnit>(pumpName);
        
        manager.connect({micName, pumpName}, {volumeName});
        manager.connect({volumeName}, {printName, fileName});
        outputUnitNames.push_back(volumeName);

        GroupInfo info;
        info.name = devName;
        info.mic = manager.getCastUnit<MicOutUnit>(micName);
        info.volume = manager.getCastUnit<VolumeFilter>(volumeName);
        view->setGroupInfo(i, info);
    });

    // connect group and inputs
    manager.connect(outputUnitNames, inputUnitNames);

    /*
    UnitManager manager;
    manager.makeUnit<PumpOutUnit>("pmp0");
    manager.makeUnit<MicOutUnit>("mic0", system->createInputStream(system->getDefaultInputDevice()));
    manager.makeUnit<VolumeFilter>("vol0");
    manager.makeUnit<PrintFilter>("pfl0", view, 0);
    manager.makeUnit<SpeakerInUnit>("spk", system->createOutputStream(system->getDefaultOutputDevice()));

    manager.connect({"pmp0", "mic0"}, {"vol0"});
    manager.connect({"vol0"}, {"pfl0"});
    manager.connect({"pfl0"}, {"spk"});

    {
        GroupInfo info;
        info.name = "mic0";
        info.mic = manager.getCastUnit<MicOutUnit>("mic0");
        info.volume = manager.getCastUnit<VolumeFilter>("vol0");
        view->setGroupInfo(0, info);
    }
    */

    manager.startAll();
    viewSystem->run();
    manager.stopAll();
}


/*
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

	//auto ptr = make_unique<boost::thread>([]() {
	//	std::string input;
	//	std::cin >> input;
	//});

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
*/
