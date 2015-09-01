#include "daisharin.hpp"
#include "helper.hpp"
#include <ctime>

Daisharin::Daisharin(const Config& config)
    : random_(static_cast<int>(::time(NULL))),    // random_device can't be used in MinGW
      dist_(0.0, 1.0),
      nowBufferIndex_(0),
      delayBuffer_(config.delayBufferSize, PCMWave::Sample(0, 0)),
      delayPoints_(config.delayPointsSize),
      feedPhase_(false),
      reverbTime_(config.reverbTime), 
      tremoloSpeed_(config.tremoloSpeed / PCMWave::SAMPLE_RATE * pi() * 2),
      bufferSize_(config.delayBufferSize),
      lpfHighDump_(config.lpfHighDump)
{
    indexedForeach(delayPoints_, [this](int i, DelayPoint& p) {
        p.offset = calcOffset();
        p.pan = calcPan();
        p.feedback = calcFeedback(p.offset);
        p.tremoloPhase = i * pi() * 2 / delayPoints_.size();
        p.lpfVal = 0;
    });
}

double Daisharin::createNoise()
{
    return dist_(random_);
}

int Daisharin::calcOffset()
{
    double noise = createNoise();
    noise = 0.1 + noise * 0.9;
    return std::min(static_cast<int>(delayBuffer_.size() * noise),
                    static_cast<int>(delayBuffer_.size() - 1));
}

double Daisharin::calcPan()
{
    return createNoise();
}

double Daisharin::calcFeedback(int offset)
{
    feedPhase_ ^= 1;
    return (feedPhase_ ? -1 : 1) *
        std::pow(
            0.001,
            offset / static_cast<double>(PCMWave::SAMPLE_RATE)
                / (reverbTime_ * 0.001))
        / std::sqrt(delayPoints_.size());
}

double Daisharin::calcTremolo(double phase)
{
    phase /= pi() * 2;
    if(0.1 <= phase && phase <= 0.9)    return 1.0;
    if(phase > 0.9) phase = 1.0 - phase;
    return 0.5 - std::cos(phase * 5 * pi() * 2) * 0.5;
}

PCMWave::Sample Daisharin::procReverb(const PCMWave::Sample& input)
{
    PCMWave::Sample output(input.left, input.left);
    for(auto& point : delayPoints_){
        int index = (delayBuffer_.size() + nowBufferIndex_ - point.offset) % delayBuffer_.size();
        double echoSrc =
            delayBuffer_.at(index).left  * (1 - point.pan)
          + delayBuffer_.at(index).right * point.pan;
        point.lpfVal =
            echoSrc      * (1.0 - lpfHighDump_)
          + point.lpfVal * lpfHighDump_;
        double outputSrc = point.lpfVal * point.feedback * calcTremolo(point.tremoloPhase);
        output.left  += outputSrc * point.pan;
        output.right += outputSrc * (1.0 - point.pan);
        point.tremoloPhase += tremoloSpeed_;
        if(point.tremoloPhase >= pi() * 2){
            point.tremoloPhase -= pi() * 2;
            point.offset = calcOffset();
            point.pan = calcPan();
            point.feedback = calcFeedback(point.offset);
        }
    }

    nowBufferIndex_++;
    if(nowBufferIndex_ >= delayBuffer_.size())  nowBufferIndex_ = 0;
    delayBuffer_.at(nowBufferIndex_) = output;
    return output - input;
}

PCMWave Daisharin::update(const PCMWave& input)
{
    PCMWave res;
    auto it = res.begin();
    for(auto& sample : input)
        *(it++) = procReverb(sample);
    return std::move(res);
}
