#include "portaudio.hpp"
#include "helper.hpp"
#include "units.hpp"
#include "unitmanager.hpp"
#include "glutview.hpp"
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <unordered_map>

#include "asio_network.hpp"

int main()
{
    std::shared_ptr<AudioSystem> audio(std::make_shared<PAAudioSystem>());
    std::shared_ptr<AsioNetworkSystem> network(std::make_shared<AsioNetworkSystem>());
    UnitManager manager;
    manager.makeUnit<MicOutUnit>("mic", audio->createInputStream(audio->getDefaultInputDevice()));
    manager.makeUnit<AsioNetworkSendUnit>("send", network->getSystemInfo(), 12345, "127.0.0.1");
    manager.makeUnit<AsioNetworkRecvUnit>("recv", network->getSystemInfo(), 12345);
    manager.makeUnit<SpeakerInUnit>("spk", audio->createOutputStream(audio->getDefaultOutputDevice()));
    manager.connect({"mic"}, {"send"});
    manager.connect({"recv"}, {"spk"});

    // run
    manager.startAll();
    sleepms(500000);
    manager.stopAll();
}


/*
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

    // run
    manager.startAll();
    viewSystem->run();
    manager.stopAll();
}
*/
