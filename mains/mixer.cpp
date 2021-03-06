#include "portaudio.hpp"
#include "helper.hpp"
#include "units.hpp"
#include "glutview.hpp"
#include "error.hpp"
#include "asio_network.hpp"
#include <boost/algorithm/string.hpp>
#include <iostream>

class MixerView;

class MixerSideGroup : public GroupBase
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

class MixerView : public GlutView<MixerSideGroup>
{
private:
    std::shared_ptr<VolumeFilter> masterVolume_;

protected:
    void draw(const std::vector<std::shared_ptr<MixerSideGroup>>& groups) override
    {
        drawString(600, 400,
            toString(masterVolume_->getVolume()), Color::blue());
    }

    void draw(int index, const std::shared_ptr<MixerSideGroup>& groupInfo) override
    {
        drawLevelMeter(index, groupInfo);
    }

    void keyDown(const std::vector<std::shared_ptr<MixerSideGroup>>& groups, unsigned char key) override
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

    void keyDown(int index, const std::shared_ptr<MixerSideGroup>& groupInfo, unsigned char key) override
    {
        auto& group = groupInfo;
        switch(key)
        {
        case 'f':
            group->recv_->stop();
            group->recv_->start();
            break;
        }
    }

    void clickLeftDown(const std::vector<std::shared_ptr<MixerSideGroup>>& groups, int x, int y)
    {
        int index = calcIndexFromXY(x, y);
        if(index >= groups.size())  return;
        auto g = groups.at(index);
        g->recv_->setMute(!g->recv_->isMute());
    }

    void wheelUp(const std::vector<std::shared_ptr<MixerSideGroup>>& groups, int x, int y)
    {
        int index = calcIndexFromXY(x, y);
        if(index >= groups.size())  return;
        auto g = groups.at(index);
        g->volume_->addVolume(5);
    }

    void wheelDown(const std::vector<std::shared_ptr<MixerSideGroup>>& groups, int x, int y)
    {
        int index = calcIndexFromXY(x, y);
        if(index >= groups.size())  return;
        auto g = groups.at(index);
        g->volume_->addVolume(-5);
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
        
        auto devices = audioSystem->getValidDevices();
        writeDeviceInfo(std::cout, devices);
        std::cout << "Output: " << std::flush;
        int index;  std::cin >> index;
        auto speaker = std::make_shared<SpeakerInUnit>(
            audioSystem->createOutputStream(devices.at(index)));

        auto masterVolume = std::make_shared<VolumeFilter>();
        auto view = std::make_shared<MixerView>(masterVolume);

        connect({masterVolume}, {speaker});
        speaker->start();
        masterVolume->start();

        std::vector<std::shared_ptr<MixerSideGroup>> groups;
        boost::thread inputThread([&viewSystem, &audioSystem, &view, &masterVolume, &groups]() {
            std::string input;
            int prevPort = 10000;
            while(std::getline(std::cin, input)){
                try{
                    const static std::unordered_map<std::string, boost::function<void(const std::vector<std::string>&)>> procs = {
                        {"bg", [&groups, &view, &masterVolume](const std::vector<std::string>& args) {
                            unsigned short port = boost::lexical_cast<unsigned short>(args.at(1));
                            auto group = std::make_shared<MixerSideGroup>(
                                port,
                                masterVolume
                            );
                            group->start();
                            view->addGroup(group);
                            groups.push_back(group);
                        }},
                        {"bgp", [&prevPort](const std::vector<std::string>& args) {
                            std::vector<std::string> newArgs(args);
                            newArgs.push_back(boost::lexical_cast<std::string>(++prevPort));
                            procs.at("bg")(newArgs);
                        }},
                        {"sync", [&groups](const std::vector<std::string>& args) {
                            for(auto& g : groups)   g->stop();
                            for(auto& g : groups)   g->start();
                        }}
                    };
                    std::vector<std::string> args;
                    boost::split(args, input, boost::is_space());
                    if(args.front() == "quit")  break;

                    auto it = procs.find(args.front());
                    if(it == procs.end()){
                        std::cout << "NOT FOUND" << std::endl;
                        continue;
                    }
                    it->second(args); 

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

