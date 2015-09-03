#include "portaudio.hpp"
#include "helper.hpp"
#include "units.hpp"
#include "glutview.hpp"
#include "asio_network.hpp"
#include "daisharin.hpp"
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

class MicSideGroup : public Group
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
            toString(micVolume_->getRate()),
            toString(sinVolume_->getRate())
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

/*
class MicSideGroup : public Group
{
private:
    std::string name_;
    std::shared_ptr<MicOutUnit> mic_;
    std::shared_ptr<VolumeFilter> micVolume_;
    std::shared_ptr<OnOffFilter> micOnOff_;

    //std::shared_ptr<SinOutUnit> sin_;
    std::shared_ptr<SinFakeOutUnit> sin_;
    std::shared_ptr<VolumeFilter> sinVolume_;
    std::shared_ptr<OnOffFilter> sinOnOff_;

    std::shared_ptr<PrintFilter> print_;
    std::shared_ptr<AsioNetworkSendInUnit> send_;

public:
    MicSideGroup(UnitManager& manager, const std::shared_ptr<AudioSystem>& audioSystem, const AudioDevicePtr& micDev, const std::shared_ptr<View>& view, int viewIndex, const std::string& ip, unsigned short port)
    {
        const std::string
            devName = replaceSpaces("_", micDev->name()) + "_" + toString(viewIndex),
            micName = "mic_" + devName,
            micVolumeName = "vol_mic_" + devName,
            micOnOffName = "onoff_mic_" + devName,
            sinName = "sin_" + devName,
            sinVolumeName = "vol_sin_" + devName,
            sinOnOffName = "onoff_sin_" + devName,
            printName = "pfl_" + devName,
            sendName = "asiosnd_" + devName;
        name_ = devName;

        mic_ = manager.makeUnit<MicOutUnit>(micName, audioSystem->createInputStream(micDev));
        micVolume_ = manager.makeUnit<VolumeFilter>(micVolumeName);
        micOnOff_ = manager.makeUnit<OnOffFilter>(micOnOffName, true);
        //sin_ = manager.makeUnit<SinOutUnit>(sinName, 1000);
        sin_ = manager.makeUnit<SinFakeOutUnit>(sinName, 1000);
        sinVolume_ = manager.makeUnit<VolumeFilter>(sinVolumeName);
        sinOnOff_ = manager.makeUnit<OnOffFilter>(sinOnOffName, false);
        print_ = manager.makeUnit<PrintFilter>(printName, view, viewIndex);
        send_ = manager.makeUnit<AsioNetworkSendInUnit>(sendName, port, ip);

        //manager.connect({micName}, {micVolumeName});
        manager.connect({micName}, {sinName, micVolumeName});
        manager.connect({micVolumeName}, {micOnOffName});
        manager.connect({micOnOffName}, {printName, sendName});
        manager.connect({sinName}, {sinVolumeName});
        manager.connect({sinVolumeName}, {sinOnOffName});
        manager.connect({sinOnOffName}, {printName, sendName});
    }
    ~MicSideGroup(){}

    bool isAlive() override
    {
        return mic_->isAlive() && (micOnOff_->isOn() || sinOnOff_->isOn());
    }

    std::string createName() override
    {
        return name_;
    }

    std::vector<std::string> createOptionalInfo() override
    {
        return std::vector<std::string>({
            boost::lexical_cast<std::string>(micVolume_->getRate()),
            sinOnOff_->isOn() ? "1" : "0",
            boost::lexical_cast<std::string>(sinVolume_->getRate())
        });
    }

    void userInput(unsigned char ch) override
    {
        switch(ch)
        {
        case 's':
            // マルチスレッドの関係で、これしかできない（と思う）
            micOnOff_->set(false);
            sinOnOff_->set(false);
            break;
        case 'p':
            micOnOff_->set(false);
            sinOnOff_->set(true);
            break;
        case 'm':
            sinOnOff_->set(false);
            micOnOff_->set(true);
            break;
        case 'o':
            micVolume_->addRate(5);
            break;
        case 'l':
            micVolume_->addRate(-5);
            break;
        case 'i':
            sinVolume_->addRate(5);
            break;
        case 'k':
            sinVolume_->addRate(-5);
            break;
        case 'f':
            send_->stop();
            send_->start();
            break;
        }
    }

};
*/

class MicView : public GlutView
{
protected:
    void draw(int index, const GroupPtr& groupInfo)
    {
        drawLevelMeter(index, groupInfo);
    }

    void keyDown(const std::vector<GroupPtr>& groups, unsigned char key)
    {
        switch(key)
        {
        case 'p':   // trigger-begin
            for(auto& g : groups){
                auto gd = std::dynamic_pointer_cast<MicSideGroup>(g);
                gd->mic_->setMute(true);
                gd->sin_->setMute(false);
            }
            break;
        case 'e':   // trigger-begin
            for(auto& g : groups){
                auto gd = std::dynamic_pointer_cast<MicSideGroup>(g);
                gd->through_->setMute(true);
                gd->reverb_->setMute(false);
            }
            break;
        }
    }

    void keyDown(int index, const GroupPtr& groupInfo, unsigned char key)
    {
        auto group = std::dynamic_pointer_cast<MicSideGroup>(groupInfo);
        switch(key)
        {
        case 's':
            // This doesn't guarantee the change of the flag of mute in multi-threading environments.
            group->mic_->setMute(group->mic_->isMute() ? false : true);
            group->sin_->setMute(true);
            break;
        case 'o':
            group->micVolume_->addRate(5);
            break;
        case 'l':
            group->micVolume_->addRate(-5);
            break;
        case 'i':
            group->sinVolume_->addRate(5);
            break;
        case 'k':
            group->sinVolume_->addRate(-5);
            break;
        case 'f':
            group->send_->stop();
            group->send_->start();
            break;
        }
    }

    void keyUp(const std::vector<GroupPtr>& groups, unsigned char key)
    {
        switch(key)
        {
        case 'p':   // trigger-end
            for(auto& g : groups){
                auto gd = std::dynamic_pointer_cast<MicSideGroup>(g);
                gd->sin_->setMute(true);
                gd->mic_->setMute(false);
            }
            break;
        case 'e':   // trigger-end
            for(auto& g : groups){
                auto gd = std::dynamic_pointer_cast<MicSideGroup>(g);
                gd->reverb_->setMute(true);
                gd->through_->setMute(false);
            }
        }
    }

public:
    MicView()
        : GlutView("mic")
    {}
};

int main(int argc, char **argv)
{
    auto audioSystem = std::make_shared<PAAudioSystem>();
    auto& viewSystem = GlutViewSystem::getInstance();
    auto view = std::make_shared<MicView>();

    boost::thread viewThread([&viewSystem]() {
        viewSystem.run();
    });

    std::vector<std::shared_ptr<MicSideGroup>> groups;
    std::string input;
    while(std::getline(std::cin, input)){
        const static std::unordered_map<std::string, boost::function<void(const std::vector<std::string>&)>> procs = {
            {"devices", [&audioSystem](const std::vector<std::string>&) {
                writeDeviceInfo(std::cout, audioSystem->getValidDevices());
            }},
            {"start_in",   [&groups, &view, &audioSystem](const std::vector<std::string>& args) {
                int index = boost::lexical_cast<int>(args.at(1));
                auto device = audioSystem->getValidDevices().at(index);
                groups.push_back(std::make_shared<MicSideGroup>(
                    device->name(),
                    std::move(audioSystem->createInputStream(device)),
                    12345 + groups.size(),
                    "127.0.0.1"
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

    for(auto& g : groups)   g->stop();

    std::cout << "SUCCESSFULLY" << std::endl;
}

