#include "portaudio.hpp"
#include "helper.hpp"
#include "units.hpp"
#include "unitmanager.hpp"
#include "glutview.hpp"
#include "asio_network.hpp"
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <unordered_map>

class MixerSideGroup : public Group
{
private:
    std::string name_;
    std::shared_ptr<VolumeFilter> vol_;
    std::shared_ptr<PrintFilter> pfl_;
    std::shared_ptr<AsioNetworkRecvUnit> recv_;
    std::shared_ptr<OnOffFilter> onoff_;

public:
    MixerSideGroup(UnitManager& manager, const std::shared_ptr<View>& view, int viewIndex, unsigned short port, const std::string& masterVolumeName)
        : name_("conn_" + toString(viewIndex))
    {
        const std::string
            recvName = "recv_" + toString(viewIndex),
            volName = "vol_" + toString(viewIndex),
            printName = "pfl_" + toString(viewIndex),
            onoffName = "onoff_" + toString(viewIndex),
            pmpName = "pmp_" + toString(viewIndex);

        vol_ = manager.makeUnit<VolumeFilter>(volName);
        pfl_ = manager.makeUnit<PrintFilter>(printName, view, viewIndex);
        recv_ = manager.makeUnit<AsioNetworkRecvUnit>(recvName, port);
        onoff_ = manager.makeUnit<OnOffFilter>(onoffName);
        manager.makeUnit<PumpOutUnit>(pmpName);

        manager.connect({recvName, pmpName}, {volName});
        manager.connect({volName}, {onoffName});
        manager.connect({onoffName}, {printName, masterVolumeName});
    }

    bool isAlive() override
    {
        return recv_->isAlive() && recv_->canSendToNext();
    }

    std::string createName() override
    {
        return name_;
    }

    std::vector<std::string> createOptionalInfo() override
    {
        return std::vector<std::string>({
            boost::lexical_cast<std::string>(vol_->getRate())
        });
    }



    void userInput(unsigned char ch) override
    {
        switch(ch)
        {
        case 's':
            onoff_->turn();
            break;
        case 'o':
            vol_->addRate(5);
            break;
        case 'l':
            vol_->addRate(-5);
            break;
        }
    }
};

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

	std::shared_ptr<ViewSystem> viewSystem(std::make_shared<GlutViewSystem>(argc, argv));
    auto view = viewSystem->createView(3);

    manager.makeUnit<VolumeFilter>("mastervolume");
    for(int i = 0;i < 3;i++){
        auto group = std::make_shared<MixerSideGroup>(
            manager, view, i, 12345 + i, "mastervolume"
        );
        view->setGroup(i, group);
    }

    // connect group and inputs
    manager.connect({"mastervolume"}, inputUnitNames);

    // run
    manager.startAll();
    viewSystem->run();
    manager.stopAll();
}

