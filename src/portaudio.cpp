#include "portaudio.hpp"
#include "helper.hpp"
#include "error.hpp"
#include <algorithm>

std::array<float, PCMWave::BUFFER_SIZE * 2> wave2float(const PCMWave& src)
{
    std::array<float, PCMWave::BUFFER_SIZE * 2> ret;
    auto it = ret.begin();
    for(auto& s : src){
        *(it++) = s.left;
        *(it++) = s.right;
    }
    return std::move(ret);
}

PCMWave float2wave(const std::array<float, PCMWave::BUFFER_SIZE>& src)
{
    PCMWave ret;
    std::transform(src.begin(), src.end(), ret.begin(),
        [](float s) { return PCMWave::Sample(s, s); });
    return std::move(ret);
}

///

bool PAAudioSystem::isFirst_ = true;

///

PAAudioDevice::PAAudioDevice(PaDeviceIndex index)
    : devIndex_(index), info_(*Pa_GetDeviceInfo(index))
{}

///

PAAudioStream::PAAudioStream(PaStream *stream)
    : stream_(stream)
{}

PAAudioStream::~PAAudioStream()
{
    ZARU_CHECK_IF(Pa_CloseStream(stream_) != paNoError);
}

void PAAudioStream::start()
{
    ZARU_THROW_UNLESS(Pa_StartStream(stream_) == paNoError);
}

void PAAudioStream::stop()
{
    ZARU_THROW_UNLESS(Pa_StopStream(stream_) == paNoError);
}

PCMWave PAAudioStream::read()
{
    std::array<float, PCMWave::BUFFER_SIZE> buffer;
    auto res = Pa_ReadStream(stream_, buffer.data(), PCMWave::BUFFER_SIZE);
    ZARU_THROW_UNLESS(res == paNoError || res == paInputOverflowed);
    PCMWave ret;
    return std::move(float2wave(buffer));
}

void PAAudioStream::write(const PCMWave& wave)
{
    auto ret = Pa_WriteStream(stream_, wave2float(wave).data(), PCMWave::BUFFER_SIZE);
    ZARU_THROW_UNLESS(ret == paNoError || ret == paOutputUnderflowed);
}

///

PAAudioSystem::PAAudioSystem()
{
    if(isFirst_){
        isFirst_ = false;
        Pa_Initialize();
        // clean portaudio system when exit
        static const auto cleaner = []() { Pa_Terminate(); };
        atexit(cleaner);
    }
}

AudioDevicePtr PAAudioSystem::getDefaultInputDevice()
{
    return std::make_shared<PAAudioDevice>(Pa_GetDefaultInputDevice());
}

AudioDevicePtr PAAudioSystem::getDefaultOutputDevice()
{
    return std::make_shared<PAAudioDevice>(Pa_GetDefaultOutputDevice());
}

std::vector<AudioDevicePtr> PAAudioSystem::getValidDevices()
{
    std::vector<AudioDevicePtr> ret;
    for(int i = 0;i < Pa_GetDeviceCount();i++)
        ret.push_back(std::make_shared<PAAudioDevice>(i));
    return ret;
}

std::unique_ptr<AudioStream> PAAudioSystem::createInputStream(const AudioDevicePtr& device)
{
    auto dev = dynamic_cast<PAAudioDevice *>(device.get());

    PaStreamParameters inputParam;
    inputParam.device = dev->getIndex();
    inputParam.channelCount = 1;
    inputParam.sampleFormat = paFloat32;
    inputParam.suggestedLatency = dev->getInfo().defaultLowInputLatency;
    inputParam.hostApiSpecificStreamInfo = NULL;

    PaStream *stream = NULL;
    ZARU_THROW_UNLESS(Pa_OpenStream(
        &stream,
        &inputParam,
        NULL,
        PCMWave::SAMPLE_RATE,
        PCMWave::BUFFER_SIZE * 2,
        paClipOff,
        NULL,
        NULL
    ) == paNoError);

    auto ret = make_unique<PAAudioStream>(stream);
    return std::move(ret);
}

std::unique_ptr<AudioStream> PAAudioSystem::createOutputStream(const AudioDevicePtr& device)
{
    auto dev = dynamic_cast<PAAudioDevice *>(device.get());

    PaStreamParameters outputParam;
    outputParam.device = dev->getIndex();
    outputParam.channelCount = 2;
    outputParam.sampleFormat = paFloat32;
    outputParam.suggestedLatency = dev->getInfo().defaultHighOutputLatency;
    outputParam.hostApiSpecificStreamInfo = NULL;

    PaStream *stream = NULL;
    ZARU_THROW_UNLESS(Pa_OpenStream(
        &stream,
        NULL,
        &outputParam,
        PCMWave::SAMPLE_RATE,
        //PCMWave::BUFFER_SIZE * 1,
        paFramesPerBufferUnspecified,
        paClipOff,
        NULL,
        NULL
    ) == paNoError);

    return make_unique<PAAudioStream>(stream);
}

