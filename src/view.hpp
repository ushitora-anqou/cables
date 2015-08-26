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

	virtual ViewPtr createView(int groupSize) = 0;
	virtual void run() = 0;
};

class Group
{
public:
    Group(){}
    virtual ~Group(){}

    virtual bool isAlive() = 0;
    virtual std::string createName() = 0;
    virtual std::vector<std::string> createOptionalInfo() = 0;
    virtual void userInput(unsigned char ch) = 0;
};
using GroupPtr = std::shared_ptr<Group>;

class View
{
private:
    std::vector<GroupPtr> groupInfoList_;

protected:
    int getGroupSize() { return groupInfoList_.size(); }
    Group& getGroup(int index) { return *groupInfoList_.at(index); }

public:
    View(int groupSize)
        : groupInfoList_(groupSize)
    {}
    virtual ~View(){}

    // un-mutexed function
    // can be called only when init
    void setGroup(int index, const GroupPtr& info)
    {
        groupInfoList_.at(index) = info;
    }

    virtual void updateLevelMeter(int index, const PCMWave::Sample& sample) = 0;
};

#endif
