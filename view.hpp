#pragma once
#ifndef ___VIEW_HPP___
#define ___VIEW_HPP___

#include "pcmwave.hpp"
#include <string>
#include <vector>

class View
{
public:
    using UID = int;

public:
    View(){}
    virtual ~View(){}

    virtual UID issueGroup(const std::string& name) = 0;
    virtual void updateLevelMeter(UID const PCMWave::Sample& sample) = 0;
};

#endif
