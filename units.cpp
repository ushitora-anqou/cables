#include "units.hpp"
#include <cmath>
#include <algorithm>

PCMWave PumpOutUnit::update()
{
    PCMWave tmp;
    tmp.fill(PCMWave::Sample(0, 0));
    return tmp;
}

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

void VolumeFilter::inputImpl(const PCMWave& wave)
{
    PCMWave ret;
    std::transform(
        wave.begin(), wave.end(),
        ret.begin(),
        [this](const PCMWave::Sample& s) { return s * rate_; }
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

void MicOutUnit::destruct()
{
    stream_->stop();
}

PCMWave MicOutUnit::update()
{
    return std::move(stream_->read());
}

