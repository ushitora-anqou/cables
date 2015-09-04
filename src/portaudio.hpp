#pragma once
#ifndef ___PORTAUDIO_HPP___
#define ___PORTAUDIO_HPP___

#include "audio.hpp"
#include <portaudio.h>
#include <string>

std::array<float, PCMWave::BUFFER_SIZE * 2> wave2float(const PCMWave& src);
PCMWave float2wave(const std::array<float, PCMWave::BUFFER_SIZE>& src);

class PAAudioDevice : public AudioDevice
{
private:
    PaDeviceIndex devIndex_;
    PaDeviceInfo info_;

public:
    PAAudioDevice(PaDeviceIndex index);
    ~PAAudioDevice(){}

    std::string name() const { return info_.name; }
    int inputChannel() const { return info_.maxInputChannels; }
    int outputChannel() const { return info_.maxOutputChannels; }

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
