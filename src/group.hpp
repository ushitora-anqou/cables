#pragma once
#ifndef ___GROUP_HPP___
#define ___GROUP_HPP___

#include "pcmwave.hpp"
#include <boost/thread.hpp>
#include <memory>
#include <string>
#include <vector>

class Group
{
private:
    boost::mutex mtx_;
    std::pair<double, double> waveLevel_;

public:
    Group(){}
    virtual ~Group(){}

    void updateWaveLevel(const PCMWave::Sample& sample);
    std::pair<double, double> getWaveLevel();

    virtual bool isAlive() = 0;
    virtual std::string createName() = 0;
    virtual std::vector<std::string> createOptionalInfo() = 0;

    virtual void start(){}
    virtual void stop(){}
};
using GroupPtr = std::shared_ptr<Group>;

#endif
