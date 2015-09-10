#pragma once
#ifndef ___UNITS_HPP___
#define ___UNITS_HPP___

#include "pcmwave.hpp"
#include "wavefile.hpp"
#include "audio.hpp"
#include "socket.hpp"
#include "group.hpp"
#include <atomic>
#include <memory>
#include <vector>

class View;

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

class ThroughFilter : public Unit
{
public:
    ThroughFilter(){}

    void inputImpl(const PCMWave& wave)
    {
        send(wave);
    }
};

class VolumeFilter : public Unit
{
private:
    std::atomic<int> volume_;   // no effect is 100

public:
    VolumeFilter(int volume = 100)
        : volume_(volume)
    {}

    void setVolume(int volume);
    void addVolume(int diff);
    int getVolume();

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
    void destruct() noexcept;
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

class PrintInUnit : public Unit
{
private:
    GroupBase& groupInfo_;

public:
	PrintInUnit(GroupBase& groupInfo);

	void inputImpl(const PCMWave& wave);
};

class NoiseGateFilter : public Unit
{
private:
    double threshold_;

public:
    NoiseGateFilter(double threshold)
        : threshold_(threshold)
    {}

    void inputImpl(const PCMWave& src);
};

#endif
