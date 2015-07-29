#include "socket.hpp"
#include "helper.hpp"
#include <cassert>
#include <utility>

void connect(const std::vector<UnitPtr>& from, const std::vector<UnitPtr>& to)
{
    for(auto& f : from){
        for(auto& t : to){
            f->socket_->toSockets_.push_back(t->socket_);
            t->socket_->fromFlags_.insert(
                std::make_pair(f->socket_->getUID(), false));
        }
    }
}

///

void Socket::write(const PCMWave& src)
{
    for(auto& s : toSockets_)   s.lock()->onRecv(uid_, src);
}

void Socket::onRecv(const UID& uid, const PCMWave& src)
{
    boost::mutex::scoped_lock lock(mtx_);

    bool isAllTrue = std::all_of(fromFlags_.begin(), fromFlags_.end(),
        [](const std::pair<UID, bool>& item) { return item.second; });
    auto it = fromFlags_.find(uid); assert(it != fromFlags_.end());
    bool isTwice = it->second;
    if(isAllTrue || isTwice){   // なげる
        parent_->input(pool_);
        for(auto& item : fromFlags_)    item.second = false;
        if(isTwice){
            pool_ = src;
            it->second = true;
        }
        return;
    }
    it->second = true;

    std::transform(
        pool_.begin(), pool_.end(),
        src.begin(), pool_.begin(),
        [](const PCMWave::Sample& t, const PCMWave::Sample& f) {
            return t + f;
        }
    );
}

///

Unit::Unit()
    : isAlive_(false)
{
    socket_ = std::make_shared<Socket>(this);
}

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

void Unit::send(const PCMWave& wave)
{
    socket_->write(wave);
}

///

void ThreadOutUnit::startImpl()
{
    proc_ = make_unique<boost::thread>(
        [this](){
            construct();
            while(!hasFinished_){
                PCMWave wave(std::move(update()));
                if(!hasFinished_)   send(wave);
            }
            destruct();
        }
    );
}

void ThreadOutUnit::stopImpl()
{
    hasFinished_ = true;
    proc_->join();
}

///

void WaitThreadOutUnit::startImpl()
{
    proc_ = make_unique<boost::thread>(
        [this](){
            construct();
            auto prevTime = getNowTime();
            while(!hasFinished_){
                PCMWave wave(std::move(update()));
                sleepms(100 - getInterval(prevTime, getNowTime()));
                prevTime = getNowTime();
                if(!hasFinished_)   send(wave);
            }
            destruct();
        }
    );
}

void WaitThreadOutUnit::stopImpl()
{
    hasFinished_ = true;
    proc_->join();
}
