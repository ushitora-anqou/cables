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
    auto audioSystem = std::make_shared<PAAudioSystem>();
    auto& viewSystem = GlutViewSystem::getInstance();
    
    auto speaker = std::make_shared<SpeakerInUnit>(
        audioSystem->createOutputStream(
            audioSystem->getDefaultOutputDevice()));
    auto masterVolume = std::make_shared<VolumeFilter>();
    auto view = std::make_shared<MixerView>(masterVolume);

    connect({masterVolume}, {speaker});
    speaker->start();
    masterVolume->start();

    boost::thread viewThread([&viewSystem]() {
        viewSystem.run();
    });

    std::vector<std::shared_ptr<MixerSideGroup>> groups;
    std::string input;
    while(std::getline(std::cin, input)){
        const static std::unordered_map<std::string, boost::function<void(const std::vector<std::string>&)>> procs = {
            {"devices", [&audioSystem](const std::vector<std::string>&) {
                writeDeviceInfo(std::cout, audioSystem->getValidDevices());
            }},
            {"start_in",   [&groups, &view, &audioSystem, &masterVolume](const std::vector<std::string>& args) {
                int index = boost::lexical_cast<int>(args.at(1));
                auto device = audioSystem->getValidDevices().at(index);
                groups.push_back(std::make_shared<MixerSideGroup>(
                    12345 + groups.size(),
                    masterVolume
                ));
                view->addGroup(groups.back());
                groups.back()->start();
            }},
        };
        std::vector<std::string> args;
        boost::split(args, input, boost::is_space());
        if(args.front() == "quit")  break;
        procs.at(args.front())(args);
    }

    viewSystem.stop();
    viewThread.join();

    speaker->stop();
    masterVolume->stop();
    for(auto& g : groups)   g->stop();

    std::cout << "SUCCESSFULLY" << std::endl;
}

