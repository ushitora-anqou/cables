#pragma once
#ifndef ___PITCHSHIFTER_HPP___
#define ___PITCHSHIFTER_HPP___

#include "pcmwave.hpp"
#include "calc.hpp"
#include <iostream>
#include <vector>

class PitchShifter
{
public:
    struct Config
    {
        double pitch, templateMS, pminMS, pmaxMS;
    };

private:
    const double rate_, pitch_;
    const int templateSize_, pmin_, pmax_;
    PCMWave prevWave_;

public:
    PitchShifter(const Config& config)
        : rate_(1 / config.pitch), pitch_(config.pitch),
          templateSize_(config.templateMS * PCMWave::SAMPLE_RATE),
          pmin_(config.pminMS * PCMWave::SAMPLE_RATE),
          pmax_(config.pmaxMS * PCMWave::SAMPLE_RATE)
    {}

    PCMWave proc(const PCMWave& nextWave)
    {
        auto& wave = prevWave_;
        std::vector<PCMWave::Sample> ffedWave(
            PCMWave::BUFFER_SIZE * pitch_ + 1,  // size / rate + 1
            PCMWave::Sample(0, 0));
        {
            int offset0 = 0, offset1 = 0;
            while(offset0 + pmax_ * 2 < PCMWave::BUFFER_SIZE){
                std::vector<PCMWave::Sample> x(wave.begin() + offset0,
                                               wave.begin() + offset0 + templateSize_);
                double maxR = 0;
                int p = pmin_;
                for(int i = pmin_;i <= pmax_;i++){
                    std::vector<PCMWave::Sample> y(wave.begin() + offset0 + i,
                                                   wave.begin() + offset0 + i + templateSize_);
                    double r = 0;
                    for(int j = 0;j < templateSize_;j++)    r += x.at(j).left * y.at(j).left;
                    if(maxR >= r)   continue;
                    maxR = r;
                    p = i;
                }

                for(int i = 0;i < p;i++){
                    ffedWave.at(offset1 + i)  = wave.at(offset0 + i) * (p - i) / p;
                    ffedWave.at(offset1 + i) += wave.at(offset0 + p + i) * i / p;
                }

                int q = p / (rate_ - 1.0) + 0.5;
                for(int i = p;i < q;i++){
                    if(offset0 + p + i >= PCMWave::BUFFER_SIZE) break;
                    ffedWave.at(offset1 + i) = wave.at(offset0 + p + i);
                }
                /*
                for(int i = p;i < q;i++){
                    int si = offset0 + p + i, di = offset1 + i;
                    if(di >= ffedWave.size())   break;
                    ffedWave.at(di) =
                        si < PCMWave::BUFFER_SIZE ? wave.at(si)
                                                  //: nextWave.at(si - PCMWave::BUFFER_SIZE);
                                                  : PCMWave::Sample(0, 0);
                }
                */

                offset0 += p + q;
                offset1 += q;
            }

            for(int i = offset1;i < ffedWave.size();i++)
                ffedWave.at(i) = wave.at(i - offset1 + offset0);
        }

        PCMWave res(0);
        const int J = 24;
        for(int i = 0;i < PCMWave::BUFFER_SIZE;i++){
            double t = pitch_ * i;
            int offset = t, j = offset - J / 2;
            for(int j = std::max(0, offset - J / 2);j <= std::min<int>(ffedWave.size() - 1, offset + J / 2);j++){
                res.at(i) += ffedWave.at(j) * sinc(pi() * (t - j));
            }
        }
        
        prevWave_ = nextWave;

        return std::move(res);
    }
};

#endif
