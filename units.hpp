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

class OnOffFilter : public Unit
{
private:
    bool isOn_;

public:
    OnOffFilter()
        : isOn_(true)
    {}

    void inputImpl(const PCMWave& wave)
    {
        if(isOn_)   send(wave);
    }

    void turn() { isOn_ = isOn_ ? false : true; }
};

class VolumeFilter : public Unit
{
private:
    boost::mutex mtx_;
    int rate_;  // no effect = 100

public:
    VolumeFilter(int rate = 100)
        : rate_(rate)
    {}

    void setRate(int rate);
    void addRate(int interval);
    int getRate();

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

class SpeakerInUnit : public Unit
{
private:
    std::unique_ptr<AudioStream> stream_;

public:
    SpeakerInUnit(std::unique_ptr<AudioStream> stream)
        : stream_(std::move(stream))
    {}
    ~SpeakerInUnit(){}

    void startImpl() { stream_->start(); }
    void stopImpl() { stream_->stop(); }
    void inputImpl(const PCMWave& wave) { stream_->write(wave); }
};

#endif
