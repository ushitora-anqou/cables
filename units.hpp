#pragma once
#ifndef ___UNITS_HPP___
#define ___UNITS_HPP___

#include "pcmwave.hpp"
#include "wavefile.hpp"
#include "audio.hpp"
#include "socket.hpp"
#include <memory>
#include <vector>

class PumpOutUnit : public WaitThreadOutUnit
{
public:
    PumpOutUnit(){}
    ~PumpOutUnit(){}

    PCMWave update();
};

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

class VolumeFilter : public Unit
{
private:
    double rate_;

public:
    VolumeFilter(double rate)
        : rate_(rate)
    {}

    void inputImpl(const PCMWave& wave);
};

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

#endif
