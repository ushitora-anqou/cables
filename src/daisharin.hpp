#pragma once
#ifndef ___DAISHA_HPP___
#define ___DAISHA_HPP___

// thanks to http://codezine.jp/article/detail/315

#include "pcmwave.hpp"
#include <vector>
#include <random>

class Daisharin
{
private:
    struct DelayPoint
    {
        int offset;
        double pan;
        double feedback;
        double tremoloPhase;
        double lpfVal;
    };

    std::mt19937 random_;
    std::uniform_real_distribution<double> dist_;
    int nowBufferIndex_;
    std::vector<PCMWave::Sample> delayBuffer_;
    std::vector<DelayPoint> delayPoints_;
    bool feedPhase_;
    double reverbTime_;
    double tremoloSpeed_;
    const int bufferSize_;
    const double lpfHighDump_;

private:
    // ランダムな数を作る関係でメンバにせざるをえない
    // These 'calc' functions are obliged
    // to belong to this class because of the random-number generator
    double createNoise();
    int calcOffset();
    double calcPan();
    double calcFeedback(int offset);
    double calcTremolo(double phase);
    PCMWave::Sample procReverb(const PCMWave::Sample& sample);

public:
    struct Config
    {
        int delayBufferSize, delayPointsSize;
        double reverbTime, tremoloSpeed, lpfHighDump;
    };

public:
    Daisharin(const Config& config);

    PCMWave update(const PCMWave& input);
};

#endif
