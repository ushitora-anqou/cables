#include "cable.hpp"
#include "helper.hpp"
#include <cassert>
#include <utility>

CablePtr connect(const std::vector<UnitPtr>& from, const std::vector<UnitPtr>& to)
{
    CablePtr ret = std::make_shared<Cable>(
        std::move(std::vector<UnitWeakPtr>(to.begin(), to.end())));
    int index = 0;
    for(auto unit : from)
        unit->socket_ = make_unique<Socket>(index++, ret);
    return std::move(ret);
}

///

void Socket::write(const PCMWave& wave) const
{
    cable_.lock()->write(index_, wave);
}

///

Unit::~Unit()
{
    assert(!isAlive_);
}

void Unit::start()
{
    assert(!isAlive_);
    isAlive_ = true;
    startImpl();
}

void Unit::stop()
{
    assert(isAlive_);
    isAlive_ = false;
    stopImpl();
}

void Unit::input(const PCMWave& wave)
{
    assert(isAlive_);
    inputImpl(wave);
}

///

Cable::Cable(std::vector<UnitWeakPtr> units)
    : units_(std::move(units)), flagWriter_(0)
{
    pool_.fill(PCMWave::Sample(0, 0));
}

void Cable::write(int index, const PCMWave& src)
{
    boost::mutex::scoped_lock lock(mtx_);

    if(flagWriter_ & index){    // 二回目
        for(auto& unit : units_)    unit.lock()->input(pool_);
        pool_ = src;
        flagWriter_ = 0;
        return;
    }
    flagWriter_ |= (1 << index);

    // たしこむ
    std::transform(
        pool_.begin(), pool_.end(),
        src.begin(), pool_.begin(),
        [](const PCMWave::Sample& t, const PCMWave::Sample& f) {
            return t + f;
        }
    );
}
