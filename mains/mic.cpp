#include "portaudio.hpp"
#include "helper.hpp"
#include "units.hpp"
#include "glutview.hpp"
#include "asio_network.hpp"
#include "daisharin.hpp"
#include "error.hpp"
#include "fakefilter.hpp"
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <unordered_map>

class ReverbFakeFilter : public FakeFilter
{
private:
    const Daisharin::Config config_;
    std::unique_ptr<Daisharin> dai_;

public:
    ReverbFakeFilter(const Daisharin::Config& config)
        : config_(config), dai_(make_unique<Daisharin>(config))
    {}

    void reset() override
    {
        dai_ = make_unique<Daisharin>(config_);
    }

    PCMWave proc(const PCMWave& src) override
    {
        return std::move(dai_->update(src));
    }
};

class SinFakeOutFilter : public FakeFilter
{
private:
    std::vector<double> sinTable_;
    int p_;
    double v_;

public:
    SinFakeOutFilter(int f, double v)
        : sinTable_(PCMWave::SAMPLE_RATE / f, 0), p_(0), v_(v)
    {
        int t = sinTable_.size();
        for(int i = 0;i < t;i++){
            sinTable_.at(i) = static_cast<double>(::sin(2 * 3.14159265358979323846264338 * i / t) * v);
        }
    }
    ~SinFakeOutFilter(){}

    void reset() override {}

    PCMWave proc(const PCMWave&) override
    {
        auto& tbl = sinTable_;
        PCMWave ret;
        for(auto& s : ret){
            s.left = tbl.at(p_);
            s.right = tbl.at(p_);
            p_ = (p_ + 1) % tbl.size();
        }
        return std::move(ret);
    }
};

class MicView;

class MicSideGroup : public GroupBase
{
    friend class MicView;

    enum {
        INDEX_THROUGH = -1,
        INDEX_SIN = 0,
        INDEX_REVERB = 1
    };
private:
    std::string name_;

    std::shared_ptr<MicOutUnit> mic_;
    std::shared_ptr<VolumeFilter> micVolume_;

    std::shared_ptr<FakeFilterSwitchUnit> filters_;    // sin, reverb

    std::shared_ptr<PrintInUnit> print_;
    std::shared_ptr<AsioNetworkSendInUnit> send_;

public:
    MicSideGroup(const std::string& name, std::unique_ptr<AudioStream> micStream, unsigned short port, const std::string& ipAddr)
        : name_(name)
    {
        mic_ = std::make_shared<MicOutUnit>(std::move(micStream));
        micVolume_ = std::make_shared<VolumeFilter>();

        Daisharin::Config config;
        config.delayBufferSize = 600 * PCMWave::SAMPLE_RATE / 1000;
        config.delayPointsSize = 50;
        config.reverbTime = 2000;
        config.tremoloSpeed = 6;
        config.lpfHighDump = 0.5;
        filters_ = std::make_shared<FakeFilterSwitchUnit>(std::vector<FakeFilterPtr>({
            std::make_shared<SinFakeOutFilter>(1000, 0.05),
            std::make_shared<ReverbFakeFilter>(config)
        }));

        print_ = std::make_shared<PrintInUnit>(*this);
        send_ = std::make_shared<AsioNetworkSendInUnit>(port, ipAddr);

        connect({mic_}, {micVolume_});
        connect({micVolume_}, {filters_});
        connect({filters_}, {print_, send_});
    }

    bool isAlive() override
    {
        return mic_->canSendContent();
    }

    std::string createName() override
    {
        return name_;
    }

    std::vector<std::string> createOptionalInfo() override
    {
        return std::vector<std::string>({
            toString(micVolume_->getVolume())
        });
    }

    void start() override
    {
        mic_->start();
        micVolume_->start();
        filters_->start();
        print_->start();
        send_->start();
    }

    void stop() override
    {
        mic_->stop();
        micVolume_->stop();
        filters_->stop();
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
            for(auto& g : groups)
                if(!g->mic_->isMute())
                    g->filters_->change(MicSideGroup::INDEX_SIN);
            break;
        case 'e':   // trigger-begin
            for(auto& g : groups)
                if(!g->mic_->isMute())
                    g->filters_->change(MicSideGroup::INDEX_REVERB);
            break;
        }
    }

    void keyDown(int index, const std::shared_ptr<MicSideGroup>& groupInfo, unsigned char key) override
    {
        auto& group = groupInfo;
        switch(key)
        {
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
        case 'e':   // trigger-end
            for(auto& g : groups)
                g->filters_->change(MicSideGroup::INDEX_THROUGH);
            break;
        }
    }

    void clickLeftDown(const std::vector<std::shared_ptr<MicSideGroup>>& groups, int x, int y)
    {
        int index = calcIndexFromXY(x, y);
        if(index >= groups.size())  return;
        auto g = groups.at(index);
        if(g->mic_->isMute()){
            g->mic_->setMute(false);
        }
        else{
            g->mic_->setMute(true);
            g->filters_->change(MicSideGroup::INDEX_THROUGH);
        }
    }

    void wheelUp(const std::vector<std::shared_ptr<MicSideGroup>>& groups, int x, int y)
    {
        int index = calcIndexFromXY(x, y);
        if(index >= groups.size())  return;
        auto g = groups.at(index);
        g->micVolume_->addVolume(5);
    }

    void wheelDown(const std::vector<std::shared_ptr<MicSideGroup>>& groups, int x, int y)
    {
        int index = calcIndexFromXY(x, y);
        if(index >= groups.size())  return;
        auto g = groups.at(index);
        g->micVolume_->addVolume(-5);
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
        int prevPort = 10000;
        while(std::getline(std::cin, input)){
            try{
                const static std::unordered_map<std::string, boost::function<void(const std::vector<std::string>&)>> procs = {
                    {"dev", [&audioSystem](const std::vector<std::string>&) {
                        writeDeviceInfo(std::cout, audioSystem->getValidDevices());
                    }},
                    {"ip", [&ip](const std::vector<std::string>& args) {
                        ip = args.at(1);
                    }},
                    {"bg",   [&groups, &view, &audioSystem, &ip, &prevPort](const std::vector<std::string>& args) {
                        int index = boost::lexical_cast<int>(args.at(1));
                        unsigned short port = prevPort = boost::lexical_cast<unsigned short>(args.at(2));
                        auto device = audioSystem->getValidDevices().at(index);
                        auto group = std::make_shared<MicSideGroup>(
                            device->name() + " : " + boost::lexical_cast<std::string>(port),
                            std::move(audioSystem->createInputStream(device)),
                            port,
                            ip
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
        viewThread.join();

        for(auto& g : groups)   g->stop();

        std::cout << "SUCCESS" << std::endl;
    }
    catch(std::exception& ex){
        ZARU_CHECK(ex.what());
    }
}

