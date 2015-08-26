#include "portaudio.hpp"
#include "helper.hpp"
#include "units.hpp"
#include "unitmanager.hpp"
#include "glutview.hpp"
#include "asio_network.hpp"
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <unordered_map>

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
    std::vector<AudioDevicePtr> inputDevices;
    {
        std::cout << "Input indexes :" << std::flush;
        std::string input;  std::getline(std::cin, input);
        std::vector<std::string> indexTokens;
        boost::split(indexTokens, input, boost::is_space());
        for(auto& token : indexTokens)
            inputDevices.push_back(devices.at(boost::lexical_cast<int>(token)));
    }
 
    // make and connect units
    UnitManager manager;

    // make output unit and group
	std::shared_ptr<ViewSystem> viewSystem(std::make_shared<GlutViewSystem>(argc, argv));
    auto view = viewSystem->createView(inputDevices.size());
    indexedForeach(inputDevices, [&manager, &system, &view](int i, const AudioDevicePtr& dev) {
        auto info = std::make_shared<MicSideGroup>(
            manager, system, dev, view, i, "127.0.0.1", 12345 + i
        );
        view->setGroup(i, info);
    });

    // run
    manager.startAll();
    viewSystem->run();
    manager.stopAll();
}


