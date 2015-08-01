#pragma once
#ifndef ___AUDIO_HPP___
#define ___AUDIO_HPP___

#include "pcmwave.hpp"
#include <memory>
#include <vector>

class AudioStream
{
public:
    AudioStream(){}
    virtual ~AudioStream(){}

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual PCMWave read() = 0;
    virtual void write(const PCMWave& wave) = 0;
};

class AudioDevice
{
public:
    AudioDevice(){}
    virtual ~AudioDevice(){}

    virtual std::string name() const = 0;
};
using AudioDevicePtr = std::shared_ptr<AudioDevice>;

class AudioSystem
{
public:
    AudioSystem(){}
    virtual ~AudioSystem(){}

    virtual AudioDevicePtr getDefaultInputDevice() = 0;
    virtual AudioDevicePtr getDefaultOutputDevice() = 0;
    virtual std::vector<AudioDevicePtr> getValidDevices() = 0;
    virtual std::unique_ptr<AudioStream> createInputStream(
        const AudioDevicePtr& device) = 0;
    virtual std::unique_ptr<AudioStream> createOutputStream(
        const AudioDevicePtr& device) = 0;

};

#endif
