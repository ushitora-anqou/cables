#include "portaudio.hpp"
#include "helper.hpp"
#include "units.hpp"
#include "glutview.hpp"
#include "asio_network.hpp"
#include <boost/algorithm/string.hpp>
#include <iostream>

class MixerView;

class MixerSideGroup : public Group
{
    friend class MixerView;
private:
    std::string name_;

    std::shared_ptr<AsioNetworkRecvOutUnit> recv_;
    std::shared_ptr<VolumeFilter> volume_;
    std::shared_ptr<PrintInUnit> print_;

public:
    MixerSideGroup(unsigned short port, const std::shared_ptr<VolumeFilter>& masterVolume)
        : name_("conn_" + toString(port))
    {
        recv_ = std::make_shared<AsioNetworkRecvOutUnit>(port);
        volume_ = std::make_shared<VolumeFilter>();
        print_ = std::make_shared<PrintInUnit>(*this);

        connect({recv_}, {volume_});
        connect({volume_}, {print_, masterVolume});
    }

    bool isAlive() override
    {
        return recv_->canSendContent();
    }

    std::string createName() override
    {
        return name_;
    }

    std::vector<std::string> createOptionalInfo() override
    {
        return std::vector<std::string>({
            toString(volume_->getRate())
        });
    }

    void start() override
    {
        recv_->start();
        volume_->start();
        print_->start();
    }

    void stop() override
    {
        recv_->stop();
        volume_->stop();
        print_->stop();
    }
};

/*
class MixerSideGroup : public Group
{
private:
    std::string name_;
    std::shared_ptr<VolumeFilter> vol_;
    std::shared_ptr<PrintFilter> pfl_;
    std::shared_ptr<AsioNetworkRecvOutUnit> recv_;
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
        recv_ = manager.makeUnit<AsioNetworkRecvOutUnit>(recvName, port);
        onoff_ = manager.makeUnit<OnOffFilter>(onoffName);
        manager.makeUnit<PumpOutUnit>(pmpName);

        manager.connect({recvName, pmpName}, {volName});
        manager.connect({volName}, {onoffName});
        manager.connect({onoffName}, {printName, masterVolumeName});
    }

    bool isAlive() override
    {
        return recv_->canSocketSendToNext();
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
*/

class MixerView : public GlutView
{
private:
    std::shared_ptr<VolumeFilter> masterVolume_;

protected:
    void draw()
    {
        drawString(600, 400,
            toString(masterVolume_->getRate()), Color::blue());
    }

    void draw(int index, const GroupPtr& groupInfo)
    {
        drawLevelMeter(index, groupInfo);
    }

    void keyDown(unsigned char key)
    {
        switch(key)
        {
        case 'i':
            masterVolume_->addRate(5);
            break;
        case 'k':
            masterVolume_->addRate(-5);
            break;
        }
    }

    void keyDown(int index, const GroupPtr& groupInfo, unsigned char key)
    {
        auto group = std::dynamic_pointer_cast<MixerSideGroup>(groupInfo);
        switch(key)
        {
        case 's':
            // intend to xor mute flag
            // but, not accurate in multi-thrading envirnments
            // but, this way is VERY VERY EASY!!
            group->recv_->setMute(!group->recv_->isMute());
            break;
        case 'o':
            group->volume_->addRate(5);
            break;
        case 'l':
            group->volume_->addRate(-5);
            break;
        }
    }

public:
    MixerView(const std::shared_ptr<VolumeFilter>& masterVolume)
        : GlutView("mixer"), masterVolume_(masterVolume)
    {}
};

int main(int argc, char **argv)
{
    std::shared_ptr<AudioSystem> system(std::make_shared<PAAudioSystem>());

    // User input
    auto devices = system->getValidDevices();
    writeDeviceInfo(std::cout, devices);
    std::vector<AudioDevicePtr> inputDevices, outputDevices;
    {
        std::cout << "Output indexes :" << std::flush;
        std::string input;  std::getline(std::cin, input);
        std::vector<std::string> indexTokens;
        boost::split(indexTokens, input, boost::is_space());
        for(auto& token : indexTokens)
            outputDevices.push_back(devices.at(boost::lexical_cast<int>(token)));
    }

    auto speaker = std::make_shared<SpeakerInUnit>(
        system->createOutputStream(system->getDefaultOutputDevice()));
    auto masterVolume = std::make_shared<VolumeFilter>();
    connect({masterVolume}, {speaker});

	std::shared_ptr<GlutViewSystem> viewSystem = std::make_shared<GlutViewSystem>(argc, argv);
    std::shared_ptr<MixerView> view = std::make_shared<MixerView>(masterVolume);

    std::vector<std::shared_ptr<MixerSideGroup>> groups;
    groups.push_back(std::make_shared<MixerSideGroup>(
        12345 + 0, masterVolume
    ));
    view->addGroup(groups.back());

    speaker->start();
    masterVolume->start();
    for(auto& g : groups)   g->start();

    viewSystem->run();

    speaker->stop();
    masterVolume->stop();
    for(auto& g : groups)   g->stop();

/*
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
*/
}

