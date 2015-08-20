#pragma once
#ifndef ___UNITS_HPP___
#define ___UNITS_HPP___

#include "pcmwave.hpp"
#include "wavefile.hpp"
#include "audio.hpp"
#include "socket.hpp"
#include <atomic>
#include <memory>
#include <vector>

class View;

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
    std::atomic<int> isOn_;
    const PCMWave emptyWave_;

public:
    OnOffFilter()
        : isOn_(1), emptyWave_(0)
    {
    }

    void inputImpl(const PCMWave& wave)
    {
        if(isOn_)   send(wave);
        else    send(emptyWave_);
    }

    void turn()
    {
        isOn_ ^= 1;
    }

    bool isOn()
    {
        return isOn_;
    }

    bool isOff()
    {
        return !isOn_;
    }
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

class PrintFilter : public Unit
{
private:
	std::shared_ptr<View> view_;
    int viewIndex_;

public:
	PrintFilter(const std::shared_ptr<View>& view, int index);

	void inputImpl(const PCMWave& wave);
};

#endif
