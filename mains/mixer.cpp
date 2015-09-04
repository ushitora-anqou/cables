#include "portaudio.hpp"
#include "helper.hpp"
#include "units.hpp"
#include "glutview.hpp"
#include "error.hpp"
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
            toString(volume_->getVolume())
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
    void draw(const std::vector<GroupPtr>& groups) override
    {
        drawString(600, 400,
            toString(masterVolume_->getVolume()), Color::blue());
    }

    void draw(int index, const GroupPtr& groupInfo) override
    {
        drawLevelMeter(index, groupInfo);
    }

    void keyDown(const std::vector<GroupPtr>& groups, unsigned char key) override
    {
        switch(key)
        {
        case 'i':
            masterVolume_->addVolume(5);
            break;
        case 'k':
            masterVolume_->addVolume(-5);
            break;
        }
    }

    void keyDown(int index, const GroupPtr& groupInfo, unsigned char key) override
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
            group->volume_->addVolume(5);
            break;
        case 'l':
            group->volume_->addVolume(-5);
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
    try{
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

        std::vector<std::shared_ptr<MixerSideGroup>> groups;
        boost::thread inputThread([&viewSystem, &audioSystem, &view, &masterVolume, &groups]() {
            std::string input;
            while(std::getline(std::cin, input)){
                try{
                    const static std::unordered_map<std::string, boost::function<void(const std::vector<std::string>&)>> procs = {
                        {"devices", [&audioSystem](const std::vector<std::string>&) {
                            writeDeviceInfo(std::cout, audioSystem->getValidDevices());
                        }},
                        {"start_in",   [&groups, &view, &audioSystem, &masterVolume](const std::vector<std::string>& args) {
                            int index = boost::lexical_cast<int>(args.at(1));
                            auto device = audioSystem->getValidDevices().at(index);
                            auto group = std::make_shared<MixerSideGroup>(
                                12345 + groups.size(),
                                masterVolume
                            );
                            group->start();
                            view->addGroup(group);
                            groups.push_back(group);
                        }},
                    };
                    std::vector<std::string> args;
                    boost::split(args, input, boost::is_space());
                    if(args.front() == "quit")  break;
                    procs.at(args.front())(args);
                }
                catch(std::exception& ex){
                    ZARU_CHECK(ex.what());
                }
            }

            viewSystem.stop();
        });
        viewSystem.run();

        speaker->stop();
        masterVolume->stop();
        for(auto& g : groups)   g->stop();

        std::cout << "SUCCESS" << std::endl;
    }
    catch(std::exception& ex){
        ZARU_CHECK(ex.what());
    }
}

