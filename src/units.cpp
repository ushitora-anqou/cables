#include "units.hpp"
#include "glutview.hpp"
#include "calc.hpp"
#include "error.hpp"
#include <cmath>
#include <algorithm>

void VolumeFilter::setVolume(int volume)
{
    if(volume < 0)  return;
    volume_ = volume;
}

void VolumeFilter::addVolume(int diff)
{
    setVolume(std::max(0, volume_ + diff));
}

int VolumeFilter::getVolume()
{
    return volume_;
}

void VolumeFilter::inputImpl(const PCMWave& wave)
{
    int vol = volume_;
    PCMWave ret;
    std::transform(
        wave.begin(), wave.end(),
        ret.begin(),
        [vol](const PCMWave::Sample& s) { return s * divd(vol, 100); }
    );
    send(ret);
}

///

MicOutUnit::MicOutUnit(std::unique_ptr<AudioStream> stream)
    : stream_(std::move(stream))
{}

void MicOutUnit::construct()
{
    stream_->start();
}

void MicOutUnit::destruct() noexcept
{
    try{
        stream_->stop();
    }
    catch(std::exception& ex){
        ZARU_CHECK(ex.what());
    }
    catch(...){
        ZARU_CHECK("fatal error");
    }
}

PCMWave MicOutUnit::update()
{
    return std::move(stream_->read());
}

///

PrintInUnit::PrintInUnit(GroupBase& groupInfo)
    : groupInfo_(groupInfo)
{}

void PrintInUnit::inputImpl(const PCMWave& wave)
{
    groupInfo_.updateWaveLevel(*wave.begin());
}

///

void NoiseGateFilter::inputImpl(const PCMWave& src)
{
    const double t = threshold_;
    PCMWave next;
    std::transform(
        src.begin(), src.end(), next.begin(),
        [t](const PCMWave::Sample& s) {
            return PCMWave::Sample(isInEq(-t, s.left,  t) ? 0 : s.left,
                                   isInEq(-t, s.right, t) ? 0 : s.right);
        }
    );
    send(next);
}
