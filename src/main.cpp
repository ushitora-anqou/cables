#include "portaudio.hpp"
#include "helper.hpp"
#include "units.hpp"
#include "unitmanager.hpp"
#include "glutview.hpp"
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <unordered_map>

#include "asio_network.hpp"


class RecorderGroup: public Group
{
private:
    std::string name_;
    std::shared_ptr<MicOutUnit> mic_;
    std::shared_ptr<VolumeFilter> volume_;
    std::shared_ptr<OnOffFilter> onoff_;

public:
    RecorderGroup(const std::string& name, const std::shared_ptr<MicOutUnit>& mic, const std::shared_ptr<VolumeFilter>& volume, const std::shared_ptr<OnOffFilter>& onoff)
        : name_(name), mic_(mic), volume_(volume), onoff_(onoff)
    {}
    ~RecorderGroup(){}

    bool isAlive() override
    {
        return mic_->isAlive() && volume_->isAlive() && onoff_->isAlive() && onoff_->isOn();
    }

    std::string createName() override
    {
        return name_;
    }

    std::vector<std::string> createOptionalInfo() override
    {
        return std::vector<std::string>({boost::lexical_cast<std::string>(volume_->getRate())});
    }

    void userInput(unsigned char ch) override
    {
        switch(ch)
        {
        case 's':
            onoff_->turn();
            break;
        case 'o':
            volume_->addRate(5);
            break;
        case 'l':
            volume_->addRate(-5);
            break;
        }
    }

};

/*
// recorder
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
        const std::string speakerName = "spk_" + replaceSpaces("_", dev->name()) + "_"
                    + toString(std::count(inputUnitNames.begin(), inputUnitNames.end(), speakerName));
        manager.makeUnit<SpeakerInUnit>(speakerName, system->createOutputStream(dev));
        inputUnitNames.push_back(speakerName);
    });

    // make output unit and group
	std::shared_ptr<ViewSystem> viewSystem(std::make_shared<GlutViewSystem>(argc, argv));
    auto view = viewSystem->createView(inputDevices.size());
    std::vector<std::string> outputUnitNames;
    indexedForeach(inputDevices, [&system, &manager, &view, &outputUnitNames](int i, const AudioDevicePtr& dev) {
        const std::string
            devName = replaceSpaces("_", dev->name()) + "_"
                + toString(std::count(outputUnitNames.begin(), outputUnitNames.end(), devName)),
            micName = "mic_" + devName,
            volumeName = "vol_" + devName,
            printName = "pfl_" + devName,
            fileName = "wfl_" + devName,
            pumpName = "pmp_" + devName,
            onoffName = "swt_" + devName;

        manager.makeUnit<MicOutUnit>(micName, system->createInputStream(dev));
        manager.makeUnit<VolumeFilter>(volumeName);
        manager.makeUnit<PrintFilter>(printName, view, i);
        manager.makeUnit<FileInUnit>(fileName, fileName + ".wav");
        manager.makeUnit<PumpOutUnit>(pumpName);
        manager.makeUnit<OnOffFilter>(onoffName);
        
        manager.connect({micName}, {onoffName});
        manager.connect({onoffName}, {volumeName});
        manager.connect({volumeName}, {printName, fileName});
        outputUnitNames.push_back(volumeName);

        auto info = 
            std::make_shared<RecorderGroup>(
                devName,
                manager.getCastUnit<MicOutUnit>(micName).lock(),
                manager.getCastUnit<VolumeFilter>(volumeName).lock(),
                manager.getCastUnit<OnOffFilter>(onoffName).lock()
            );
        view->setGroup(i, info);
    });

    // connect group and inputs
    manager.connect(outputUnitNames, inputUnitNames);

    // run
    manager.startAll();
    viewSystem->run();
    manager.stopAll();
}
*/
