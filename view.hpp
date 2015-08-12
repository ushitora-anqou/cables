#pragma once
#ifndef ___VIEW_HPP___
#define ___VIEW_HPP___

#include "pcmwave.hpp"
#include <string>
#include <vector>
#include <memory>

class View;
using ViewPtr = std::shared_ptr<View>;

class ViewSystem
{
public:
	ViewSystem(){}
	virtual ~ViewSystem(){}

	virtual ViewPtr createView() = 0;
	virtual void run() = 0;
};

class View
{
public:
    using UID = int;

public:
    View(){}
    virtual ~View(){}

    virtual UID issueGroup(const std::string& name) = 0;
    virtual void updateLevelMeter(UID uid, const PCMWave::Sample& sample) = 0;
};

#endif
