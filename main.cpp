#include "socket.hpp"
#include "helper.hpp"
#include <iostream>

#include <cmath>
class SinOutUnit : public WaitThreadOutUnit
{
private:
    std::vector<double> sinTable_;
    int p_;

public:
    SinOutUnit(int f);
    ~SinOutUnit(){}

    PCMWave update();
};

SinOutUnit::SinOutUnit(int f)
    : sinTable_(PCMWave::SAMPLE_RATE / f, 0), p_(0)
{
    int t = sinTable_.size();
    for(int i = 0;i < t;i++){
        sinTable_.at(i) = static_cast<double>(::sin(2 * 3.14159265358979323846264338 * i / t));
    }
}

PCMWave SinOutUnit::update()
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

///

#include "wavefile.hpp"
// ファイルに書き込む
class FileInUnit : public Unit
{
private:
    WaveOutFile file_;

public:
    FileInUnit(const std::string& filename)
        : file_(filename)
    {}

    void inputImpl(const PCMWave& wave)
    {
        file_.write(wave);
    }
};

///

class VolumeFilter : public Unit
{
private:
    double rate_;

public:
    VolumeFilter(double rate)
        : rate_(rate)
    {}

    void inputImpl(const PCMWave& wave)
    {
        PCMWave ret;
        std::transform(
            wave.begin(), wave.end(),
            ret.begin(),
            [this](const PCMWave::Sample& s) { return s * rate_; }
        );
        send(ret);
    }
};

///

#include "audio.hpp"
#include "portaudio.hpp"

class MicOutUnit : public ThreadOutUnit
{
private:
    std::unique_ptr<AudioStream> stream_;

public:
    MicOutUnit(std::unique_ptr<AudioStream> stream);
    ~MicOutUnit(){}

    void construct();
    PCMWave update();
    void destruct();
};

MicOutUnit::MicOutUnit(std::unique_ptr<AudioStream> stream)
    : stream_(std::move(stream))
{}

void MicOutUnit::construct()
{
    stream_->start();
}

void MicOutUnit::destruct()
{
    stream_->stop();
}

PCMWave MicOutUnit::update()
{
    return std::move(stream_->read());
}

///

class SpeakerOutUnit : public Unit
{
private:
    std::unique_ptr<AudioStream> stream_;

public:
    SpeakerOutUnit(std::unique_ptr<AudioStream> stream)
        : stream_(std::move(stream))
    {}
    ~SpeakerOutUnit(){}

    void startImpl() { stream_->start(); }
    void stopImpl() { stream_->stop(); }
    void inputImpl(const PCMWave& wave) { stream_->write(wave); }
};

///

#include <unordered_map>

int main(int argc, char **argv)
{
    std::shared_ptr<AudioSystem> system(std::make_shared<PAAudioSystem>());

    std::unordered_map<std::string, UnitPtr> units = {
        {"mic", makeUnit<MicOutUnit>(system->createInputStream(system->getDefaultInputDevice()))},
        {"sin", makeUnit<SinOutUnit>(500)},
        {"vfl", makeUnit<VolumeFilter>(0.5)},
        {"infile", makeUnit<FileInUnit>("test.wav")},
        {"spk", makeUnit<SpeakerOutUnit>(system->createOutputStream(system->getDefaultOutputDevice()))},
    };
    connect({units.at("mic")}, {units.at("infile"), units.at("spk")});
    connect({units.at("sin")}, {units.at("vfl")});
    connect({units.at("vfl")}, {units.at("infile"), units.at("spk")});

    /*
    std::vector<UnitPtr> units = {
        makeUnit<MicOutUnit>(system->createInputStream(system->getDefaultInputDevice())),
        makeUnit<SinOutUnit>(500),
        makeUnit<FileInUnit>("test.wav"),
        makeUnit<SpeakerOutUnit>(system->createOutputStream(system->getDefaultOutputDevice())),
    };
    connect({units.at(0)}, {units.at(2), units.at(3)});
    connect({units.at(1)}, {units.at(2), units.at(3)});
    */

    for(auto& unit : units) unit.second->start();

    while(true)
    {
        std::string input;
        std::cin >> input;
        if(input == "quit")  break;
        else if(input == "stop"){
            std::cin >> input;
            auto& unit = *units.at(input);
            if(unit.isAlive())  unit.stop();
            else    std::cout << "Already stopped." << std::endl;
        }
        else if(input == "start"){
            std::cin >> input;
            auto& unit = *units.at(input);
            if(!unit.isAlive())  unit.start();
            else    std::cout << "Already started." << std::endl;
        }
    }

    for(auto& unit : units) unit.second->stop();


    return 0;
}
