#include "portaudio.hpp"
#include "helper.hpp"
#include "units.hpp"
#include "unitmanager.hpp"
#include "glutview.hpp"
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <unordered_map>

#include "asio_network.hpp"

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

class RecorderGroup: public Group
{
private:
    std::string name_;
    std::shared_ptr<MicOutUnit> mic_;
    std::shared_ptr<VolumeFilter> volume_;
    std::shared_ptr<OnOffFilter> onoff_;

public:
    RecorderGroup(const std::string& name, const std::shared_ptr<MicOutUnit>& mic, const std::shared_ptr<VolumeFilter>& volume, const std::shared_ptr<OnOffFilter>& onoff)
        : name_(name), mic_(mic), volume_(volume), onoff_(onoff)
    {}
    ~RecorderGroup(){}

    bool isAlive() override
    {
        return mic_->isAlive() && volume_->isAlive() && onoff_->isAlive() && onoff_->isOn();
    }

    std::string createName() override
    {
        return name_;
    }

    std::vector<std::string> createOptionalInfo() override
    {
        return std::vector<std::string>({boost::lexical_cast<std::string>(volume_->getRate())});
    }

    void userInput(unsigned char ch) override
    {
        switch(ch)
        {
        case 's':
            onoff_->turn();
            break;
        case 'o':
            volume_->addRate(5);
            break;
        case 'l':
            volume_->addRate(-5);
            break;
        }
    }

};

class MicSideGroup : public Group
{
private:
    std::string name_;
    std::shared_ptr<MicOutUnit> mic_;
    std::shared_ptr<VolumeFilter> micVolume_;
    std::shared_ptr<OnOffFilter> micOnOff_;

    std::shared_ptr<SinOutUnit> sin_;
    std::shared_ptr<VolumeFilter> sinVolume_;
    std::shared_ptr<OnOffFilter> sinOnOff_;

    std::shared_ptr<PrintFilter> print_;
    std::shared_ptr<AsioNetworkSendUnit> send_;

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
        sin_ = manager.makeUnit<SinOutUnit>(sinName, 1000);
        sinVolume_ = manager.makeUnit<VolumeFilter>(sinVolumeName);
        sinOnOff_ = manager.makeUnit<OnOffFilter>(sinOnOffName, false);
        print_ = manager.makeUnit<PrintFilter>(printName, view, viewIndex);
        send_ = manager.makeUnit<AsioNetworkSendUnit>(sendName, port, ip);

        manager.connect({micName}, {micVolumeName});
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
        }
    }

};

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

// mic side
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

/*
// mixer side
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
*/

/*
// recorder
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
            pumpName = "pmp_" + devName,
            onoffName = "swt_" + devName;

        manager.makeUnit<MicOutUnit>(micName, system->createInputStream(dev));
        manager.makeUnit<VolumeFilter>(volumeName);
        manager.makeUnit<PrintFilter>(printName, view, i);
        manager.makeUnit<FileInUnit>(fileName, fileName + ".wav");
        manager.makeUnit<PumpOutUnit>(pumpName);
        manager.makeUnit<OnOffFilter>(onoffName);
        
        manager.connect({micName}, {onoffName});
        manager.connect({onoffName}, {volumeName});
        manager.connect({volumeName}, {printName, fileName});
        outputUnitNames.push_back(volumeName);

        auto info = 
            std::make_shared<RecorderGroup>(
                devName,
                manager.getCastUnit<MicOutUnit>(micName).lock(),
                manager.getCastUnit<VolumeFilter>(volumeName).lock(),
                manager.getCastUnit<OnOffFilter>(onoffName).lock()
            );
        view->setGroup(i, info);
    });

    // connect group and inputs
    manager.connect(outputUnitNames, inputUnitNames);

    // run
    manager.startAll();
    viewSystem->run();
    manager.stopAll();
}
*/
