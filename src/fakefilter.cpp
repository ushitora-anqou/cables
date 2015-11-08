#include "fakefilter.hpp"
#include "helper.hpp"
#include <cassert>

FakeFilterSwitchUnit::FakeFilterSwitchUnit(std::vector<FakeFilterPtr> filters)
    : nowIndex_(-1), filters_(std::move(filters))
{}

FakeFilterSwitchUnit::FakeFilterSwitchUnit(int nowIndex, std::vector<FakeFilterPtr> filters)
    : nowIndex_(nowIndex), filters_(std::move(filters))
{
    // size() returns uint64, so -1 means a very large number
    assert(nowIndex_ < 0 || nowIndex_ < filters_.size());
    nowIndex_ = std::max(-1, nowIndex);
}

void FakeFilterSwitchUnit::change(int index)
{
    SCOPED_LOCK(mtx_);

    // size() returns uint64, so -1 means a very large number
    assert(index < 0 || index < filters_.size());
    nowIndex_ = std::max(-1, index);
    if(nowIndex_ != -1) filters_.at(nowIndex_)->reset();
}

void FakeFilterSwitchUnit::inputImpl(const PCMWave& src)
{
    SCOPED_LOCK(mtx_);

    int index = nowIndex_;
    send(index < 0 ? src : std::move(filters_.at(index)->proc(src)));
}
