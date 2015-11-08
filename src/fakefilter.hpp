#pragma once
#ifndef ___FAKEFILTER_HPP___
#define ___FAKEFILTER_HPP___

#include "pcmwave.hpp"
#include "socket.hpp"
#include <boost/thread.hpp>
#include <memory>
#include <vector>

class FakeFilter
{
public:
    FakeFilter(){}
    virtual ~FakeFilter(){}

    virtual void reset() = 0;
    virtual PCMWave proc(const PCMWave& src) = 0;
};
using FakeFilterPtr = std::shared_ptr<FakeFilter>;

class FakeFilterSwitchUnit: public Unit
{
private:
    int nowIndex_;
    const std::vector<FakeFilterPtr> filters_;
    boost::mutex mtx_;

public:
    FakeFilterSwitchUnit(std::vector<FakeFilterPtr> filters);
    FakeFilterSwitchUnit(int nowIndex, std::vector<FakeFilterPtr> filters);

    // -1 : through
    void change(int index);

    void inputImpl(const PCMWave& src);
};

#endif
