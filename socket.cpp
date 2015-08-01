#include "socket.hpp"
#include "helper.hpp"
#include <cassert>
#include <utility>

void connect(const std::vector<UnitPtr>& from, const std::vector<UnitPtr>& to)
{
    for(auto& f : from){
        for(auto& t : to){
            f->socket_->toSockets_.push_back(t->socket_);
            t->socket_->pool_.insert(
                std::make_pair(f->socket_->getUID(), std::queue<PCMWave>()));
        }
    }
}

///

Unit::Socket::Socket(Unit *parent)
    : uid_(boost::uuids::random_generator()()), parent_(parent)
{}


void Unit::Socket::write(const PCMWave& src)
{
    for(auto& s : toSockets_)   s.lock()->onRecv(uid_, src);
}

void Unit::Socket::onRecv(const UID& uid, const PCMWave& src)
{
    boost::mutex::scoped_lock lock(mtx_);
    
    pool_.at(uid).push(src);

    // 全てのUnitからRecvしたか
    if(std::all_of(pool_.begin(), pool_.end(),
        [](const std::pair<UID, std::queue<PCMWave>>& item) { return !item.second.empty(); }))
    {
        emitPool();
    }
}

void Unit::Socket::emitPool()
{
    PCMWave wave;
    wave.fill(PCMWave::Sample(0, 0));
    for(auto& p : pool_){
        std::transform(
            p.second.front().begin(), p.second.front().end(),
            wave.begin(), wave.begin(),
            [](const PCMWave::Sample& t, const PCMWave::Sample& f) {
                return t + f;
            }
        );
        p.second.pop();
    }

    parent_->input(wave);
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

bool Unit::isAlive() const
{
    return isAlive_;
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
    hasFinished_ = false;
    proc_ = make_unique<boost::thread>(
        [this](){
            construct();
            auto prevTime = getNowTime();
            while(!hasFinished_){
                PCMWave wave(std::move(update()));
                sleepms(90 - getInterval(prevTime, getNowTime()));
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
