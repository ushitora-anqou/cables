#include "portaudio.hpp"
#include "helper.hpp"
#include "units.hpp"
#include <iostream>

#include "daisharin.hpp"

class EchoFilter : public Unit
{
private:
    Daisharin dai_;

public:
    EchoFilter(const Daisharin::Config& config)
        : dai_(config)
    {}

    void inputImpl(const PCMWave& wave)
    {
        send(dai_.update(wave));
    }
};

int main()
{
    std::shared_ptr<AudioSystem> audioSystem = std::make_shared<PAAudioSystem>();
    auto mic = std::make_shared<MicOutUnit>(audioSystem->createInputStream(audioSystem->getDefaultInputDevice()));
    auto spk = std::make_shared<SpeakerInUnit>(audioSystem->createOutputStream(audioSystem->getDefaultOutputDevice()));
    auto file = std::make_shared<FileInUnit>("test.wav");

    Daisharin::Config config;
    config.delayBufferSize = 600 * PCMWave::SAMPLE_RATE / 1000;
    config.delayPointsSize = 50;
    config.reverbTime = 2000;
    config.tremoloSpeed = 6;
    config.lpfHighDump = 0.5;
    auto fil = std::make_shared<EchoFilter>(config);

    connect({mic}, {fil});
    connect({fil}, {spk, file});

    mic->start();
    spk->start();
    fil->start();
    file->start();

    while(true){
        std::string input;
        std::cin >> input;
        if(input.at(0) == 'q')  break;
    }

    mic->stop();
    spk->stop();
    fil->stop();
    file->stop();
}

