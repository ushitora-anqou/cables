#include "portaudio.hpp"
#include "helper.hpp"
#include "units.hpp"
#include "glutview.hpp"
#include "asio_network.hpp"
#include "daisharin.hpp"
#include "error.hpp"
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <unordered_map>

class ReverbFilter : public Unit
{
private:
    Daisharin dai_;

public:
    ReverbFilter(const Daisharin::Config& config)
        : dai_(config)
    {}

    void inputImpl(const PCMWave& wave)
    {
        send(dai_.update(wave));
    }
};

class SinFakeOutUnit : public Unit
{
private:
    std::vector<double> sinTable_;
    int p_;

public:
    SinFakeOutUnit(int f)
        : sinTable_(PCMWave::SAMPLE_RATE / f, 0), p_(0)
    {
        int t = sinTable_.size();
        for(int i = 0;i < t;i++){
            sinTable_.at(i) = static_cast<double>(::sin(2 * 3.14159265358979323846264338 * i / t));
        }
    }
    ~SinFakeOutUnit(){}

    void inputImpl(const PCMWave& wave)
    {
        auto& tbl = sinTable_;
        PCMWave ret;
        for(auto& s : ret){
            s.left = tbl.at(p_);
            s.right = tbl.at(p_);
            p_ = (p_ + 1) % tbl.size();
        }
        send(ret);
    }
};

class MicView;

class MicSideGroup : public GroupBase
{
    friend class MicView;
private:
    std::string name_;

    std::shared_ptr<MicOutUnit> mic_;
    std::shared_ptr<VolumeFilter> micVolume_;

    std::shared_ptr<SinFakeOutUnit> sin_;
    std::shared_ptr<VolumeFilter> sinVolume_;

    std::shared_ptr<ThroughFilter> through_;
    std::shared_ptr<ReverbFilter> reverb_;

    std::shared_ptr<PrintInUnit> print_;
    std::shared_ptr<AsioNetworkSendInUnit> send_;

public:
    MicSideGroup(const std::string& name, std::unique_ptr<AudioStream> micStream, unsigned short port, const std::string& ipAddr)
        : name_(name)
    {
        mic_ = std::make_shared<MicOutUnit>(std::move(micStream));
        micVolume_ = std::make_shared<VolumeFilter>();

        sin_ = std::make_shared<SinFakeOutUnit>(1000);
        sin_->setMute(true);
        sinVolume_ = std::make_shared<VolumeFilter>(5);

        through_ = std::make_shared<ThroughFilter>();
        Daisharin::Config config;
        config.delayBufferSize = 600 * PCMWave::SAMPLE_RATE / 1000;
        config.delayPointsSize = 50;
        config.reverbTime = 2000;
        config.tremoloSpeed = 6;
        config.lpfHighDump = 0.5;
        reverb_ = std::make_shared<ReverbFilter>(config);
        reverb_->setMute(true);

        print_ = std::make_shared<PrintInUnit>(*this);
        send_ = std::make_shared<AsioNetworkSendInUnit>(port, ipAddr);

        connect({mic_}, {micVolume_, sin_});
        connect({micVolume_}, {through_, reverb_});
        connect({sin_}, {sinVolume_});
        connect({sinVolume_}, {through_, reverb_});
        connect({through_, reverb_}, {print_, send_});
    }

    bool isAlive() override
    {
        return sin_->canSendContent() || mic_->canSendContent();
    }

    std::string createName() override
    {
        return name_;
    }

    std::vector<std::string> createOptionalInfo() override
    {
        return std::vector<std::string>({
            toString(micVolume_->getVolume()),
            toString(sinVolume_->getVolume())
        });
    }

    void start() override
    {
        mic_->start();
        micVolume_->start();
        sin_->start();
        sinVolume_->start();
        through_->start();
        reverb_->start();
        print_->start();
        send_->start();
    }

    void stop() override
    {
        mic_->stop();
        micVolume_->stop();
        sin_->stop();
        sinVolume_->stop();
        through_->stop();
        reverb_->stop();
        print_->stop();
        send_->stop();
    }
};

class MicView : public GlutView<MicSideGroup>
{
protected:
    void draw(int index, const std::shared_ptr<MicSideGroup>& groupInfo) override
    {
        drawLevelMeter(index, groupInfo);
    }

    void keyDown(const std::vector<std::shared_ptr<MicSideGroup>>& groups, unsigned char key) override
    {
        switch(key)
        {
        case 'p':   // trigger-begin
            for(auto& g : groups){
                g->mic_->setMute(true);
                g->sin_->setMute(false);
            }
            break;
        case 'e':   // trigger-begin
            for(auto& g : groups){
                g->through_->setMute(true);
                g->reverb_->setMute(false);
            }
            break;
        }
    }

    void keyDown(int index, const std::shared_ptr<MicSideGroup>& groupInfo, unsigned char key) override
    {
        auto& group = groupInfo;
        switch(key)
        {
        case 's':
            // This doesn't guarantee the change of the flag of mute in multi-threading environments.
            group->mic_->setMute(group->mic_->isMute() ? false : true);
            group->sin_->setMute(true);
            break;
        case 'o':
            group->micVolume_->addVolume(5);
            break;
        case 'l':
            group->micVolume_->addVolume(-5);
            break;
        case 'i':
            group->sinVolume_->addVolume(5);
            break;
        case 'k':
            group->sinVolume_->addVolume(-5);
            break;
        case 'f':
            group->send_->stop();
            group->send_->start();
            break;
        }
    }

    void keyUp(const std::vector<std::shared_ptr<MicSideGroup>>& groups, unsigned char key) override
    {
        switch(key)
        {
        case 'p':   // trigger-end
            for(auto& g : groups){
                g->sin_->setMute(true);
                g->mic_->setMute(false);
            }
            break;
        case 'e':   // trigger-end
            for(auto& g : groups){
                g->reverb_->setMute(true);
                g->through_->setMute(false);
            }
        }
    }

    void clickLeftDown(const std::vector<std::shared_ptr<MicSideGroup>>& groups, int x, int y)
    {
        int index = calcIndexFromXY(x, y);
        if(index >= groups.size())  return;
        auto g = groups.at(index);
        g->mic_->setMute(g->mic_->isMute() ? false : true);
        g->sin_->setMute(true);
    }

public:
    MicView()
        : GlutView("mic")
    {}
};

int main(int argc, char **argv)
{
    try{
        auto audioSystem = std::make_shared<PAAudioSystem>();
        auto& viewSystem = GlutViewSystem::getInstance();
        auto view = std::make_shared<MicView>();

        boost::thread viewThread([&viewSystem]() {
            viewSystem.run();
        });

        std::vector<std::shared_ptr<MicSideGroup>> groups;
        std::string input, ip = "127.0.0.1";
        while(std::getline(std::cin, input)){
            try{
                const static std::unordered_map<std::string, boost::function<void(const std::vector<std::string>&)>> procs = {
                    {"devices", [&audioSystem](const std::vector<std::string>&) {
                        writeDeviceInfo(std::cout, audioSystem->getValidDevices());
                    }},
                    {"ip", [&ip](const std::vector<std::string>& args) {
                        ip = args.at(1);
                    }},
                    {"start_in",   [&groups, &view, &audioSystem, &ip](const std::vector<std::string>& args) {
                        int index = boost::lexical_cast<int>(args.at(1));
                        unsigned short port = boost::lexical_cast<unsigned short>(args.at(2));
                        auto device = audioSystem->getValidDevices().at(index);
                        auto group = std::make_shared<MicSideGroup>(
                            device->name(),
                            std::move(audioSystem->createInputStream(device)),
                            port,
                            ip
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
        viewThread.join();

        for(auto& g : groups)   g->stop();

        std::cout << "SUCCESS" << std::endl;
    }
    catch(std::exception& ex){
        ZARU_CHECK(ex.what());
    }
}

