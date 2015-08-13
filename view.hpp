#pragma once
#ifndef ___VIEW_HPP___
#define ___VIEW_HPP___

#include "pcmwave.hpp"
#include "units.hpp"
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

	virtual ViewPtr createView(int groupSize) = 0;
	virtual void run() = 0;
};

struct GroupInfo
{
    std::string name;
    std::weak_ptr<VolumeFilter> volume;
};

class View
{
private:
    std::vector<GroupInfo> groupInfoList_;

protected:
    const GroupInfo& getGroupInfo(int index) const { return groupInfoList_.at(index); }

public:
    View(int groupSize)
        : groupInfoList_(groupSize)
    {}
    virtual ~View(){}

    // un-mutexed function
    // can be called only when init
    void setGroupInfo(int index, const GroupInfo& info)
    {
        groupInfoList_.at(index) = info;
    }

    virtual void updateLevelMeter(int index, const PCMWave::Sample& sample) = 0;
};

#endif
