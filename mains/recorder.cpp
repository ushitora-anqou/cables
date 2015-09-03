#include "portaudio.hpp"
#include "helper.hpp"
#include "units.hpp"
#include "glutview.hpp"
#include <boost/algorithm/string.hpp>
#include <iostream>


class RecorderGroup : public Group
{
    friend class RecorderView;
private:
    std::string name_;

    std::shared_ptr<MicOutUnit> mic_;
    std::shared_ptr<PrintInUnit> print_;
    std::shared_ptr<FileInUnit> file_;

public:
    RecorderGroup(const std::string& name, std::unique_ptr<AudioStream> micStream, const std::vector<UnitPtr>& speakers)
        : name_(name)
    {
        mic_ = std::make_shared<MicOutUnit>(std::move(micStream));
        print_ = std::make_shared<PrintInUnit>(*this);
        file_ = std::make_shared<FileInUnit>(name + ".wav");

        connect({mic_}, {print_, file_});
        connect({mic_}, speakers);
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
        return std::vector<std::string>({});
    }

    void start() override
    {
        mic_->start();
        print_->start();
        file_->start();
    }
    
    void stop() override
    {
        mic_->stop();
        print_->stop();
        file_->stop();
    }
};

class RecorderView : public GlutView
{
protected:
    void draw(int index, const GroupPtr& groupInfo)
    {
        drawLevelMeter(index, groupInfo);
    }

    void keyDown(int index, const GroupPtr& groupInfo, unsigned char key)
    {
        auto group = std::dynamic_pointer_cast<RecorderGroup>(groupInfo);
        switch(key)
        {
        case 's':
            // doesn't guarantee
            group->mic_->setMute(group->mic_->isMute() ? false : true);
            break;
        }
    }
public:
    RecorderView()
        : GlutView("recorder")
    {}
};

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

    // make input unit e.g. speaker
    std::vector<UnitPtr> inputUnits;
    indexedForeach(outputDevices, [&system, &inputUnits](int i, const AudioDevicePtr& dev) {
        inputUnits.push_back(std::make_shared<SpeakerInUnit>(system->createOutputStream(dev)));
    });


    auto& viewSystem = GlutViewSystem::getInstance();
    std::shared_ptr<RecorderView> view = std::make_shared<RecorderView>();

    std::vector<std::shared_ptr<RecorderGroup>> groups;
    indexedForeach(inputDevices, [&system, &view, &groups, &inputUnits](int i, const AudioDevicePtr& dev) {
        const std::string name =
            boost::regex_replace(   // convert the device's name to use as a file name
                dev->name(),
                boost::regex("[\\s\\\\/*:?\"<>|]+"),
                "_",
                boost::format_all);
        groups.push_back(std::make_shared<RecorderGroup>(name, std::move(system->createInputStream(dev)), inputUnits));
        view->addGroup(groups.front());
    });

    for(auto& g : groups)   g->start();
    for(auto& u : inputUnits)   u->start();

    boost::thread viewThread([&viewSystem]() {
        viewSystem.run();
    });
    std::string input;
    while(std::getline(std::cin, input)){
        if(input == "quit") break;
    }
    viewSystem.stop();
    viewThread.join();

    for(auto& g : groups)   g->stop();
    for(auto& u : inputUnits)   u->stop();
}
