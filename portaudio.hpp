#pragma once
#ifndef ___PORTAUDIO_HPP___
#define ___PORTAUDIO_HPP___

#include "audio.hpp"
#include <portaudio.h>
#include <string>

class PAAudioDevice : public AudioDevice
{
private:
    PaDeviceIndex devIndex_;
    PaDeviceInfo info_;

public:
    PAAudioDevice(PaDeviceIndex index);
    ~PAAudioDevice(){}

    std::string name() const { return info_.name; }

    PaDeviceIndex getIndex() const { return devIndex_; }
    const PaDeviceInfo& getInfo() const { return info_; }
};

class PAAudioStream : public AudioStream
{
private:
    PaStream *stream_;

public:
    PAAudioStream(PaStream *stream);
    ~PAAudioStream();

    void start();
    void stop();
    PCMWave read();
    void write(const PCMWave& wave);
};

class PAAudioSystem : public AudioSystem
{
private:
    static bool isFirst_;

public:
    PAAudioSystem();
    ~PAAudioSystem(){}

    AudioDevicePtr getDefaultInputDevice();
    AudioDevicePtr getDefaultOutputDevice();
    std::vector<AudioDevicePtr> getValidDevices();
    std::unique_ptr<AudioStream> createInputStream(
        const AudioDevicePtr& device);
    std::unique_ptr<AudioStream> createOutputStream(
        const AudioDevicePtr& device);
};

#endif