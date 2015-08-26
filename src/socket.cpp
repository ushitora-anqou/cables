#include "socket.hpp"
#include "helper.hpp"
#include <cassert>
#include <utility>

void connect(const std::vector<UnitPtr>& prevUnits, const std::vector<UnitPtr>& nextUnits)
{
    for(auto& prev : prevUnits)
        for(auto& next : nextUnits)
            prev->connectTo(next);
}

///

Unit::Socket::Socket(Unit& parent)
    : canRecvFromPrev_(false), canSendToNext_(false), parent_(parent)
{}

void Unit::Socket::open()
{
    {
        SCOPED_LOCK(sendMtx_);
        canSendToNext_ = true;
    }
    {
        SCOPED_LOCK(recvMtx_);
        canRecvFromPrev_ = true;
    }
}

void Unit::Socket::close()
{
    {
        SCOPED_LOCK(sendMtx_);
        canSendToNext_ = false;
    }
    {
        SCOPED_LOCK(recvMtx_);
        canRecvFromPrev_ = false;
        for(auto& pool : recvPool_)
            pool.second.clear();
    }
}

void Unit::Socket::addNextSocket(const SocketPtr& next)
{
    SCOPED_LOCK(sendMtx_);
    nextSockets_.push_back(next);
}

void Unit::Socket::addPrevSocket(const SocketPtr& prev)
{
    SCOPED_LOCK(recvMtx_);
    recvPool_.insert(std::make_pair(prev, std::deque<PCMWave>()));
}

void Unit::Socket::write(const PCMWave& src)
{
    SCOPED_LOCK(sendMtx_);
    if(!canSendToNext_) return;

    for(auto& socket : nextSockets_)
        socket->onRecv(shared_from_this(), src);
}

void Unit::Socket::onRecv(const SocketPtr& sender, const PCMWave& src)
{
    // 受け取れる状態をこの関数内で保証
    SCOPED_LOCK(recvMtx_);
    if(!canRecvFromPrev_)   return;
    
    recvPool_.at(sender).push_back(src);

    // 全てのUnitからRecvしたか
    // canSendToNext() == true であるUnitのみRecvを待つ
    // 先にQueueの判定をして、Queueに溜まっていれば生死に関わらずそれの処理を行う
    if(std::all_of(recvPool_.begin(), recvPool_.end(),
        [](const std::pair<SocketPtr, std::deque<PCMWave>>& item) {
            auto& socket = *item.first;
            auto& que = item.second;
            return !que.empty() || !socket.canSendToNext();
        }))
    {
        emitPool();
    }
}

void Unit::Socket::emitPool()
{
    PCMWave wave;
    wave.fill(PCMWave::Sample(0, 0));
    for(auto& pool : recvPool_){
        auto& que = pool.second;
        if(que.empty()) continue;
        std::transform(
            que.front().begin(), que.front().end(),
            wave.begin(), wave.begin(),
            [](const PCMWave::Sample& t, const PCMWave::Sample& f) {
                return t + f;
            }
        );
        que.pop_front();
    }

    parent_.input(wave);
}

///

Unit::Unit()
    : isAlive_(false)
{
    socket_ = std::make_shared<Socket>(*this);
}

Unit::~Unit()
{
    boost::shared_lock<boost::shared_mutex> lock(mtx_);
    assert(!isAlive_);
}

void Unit::connectTo(const UnitPtr& next)
{
    this->socket_->addNextSocket(next->socket_);
    next->socket_->addPrevSocket(this->socket_);
}

bool Unit::isAlive()
{
    //boost::shared_lock<boost::shared_mutex> lock(mtx_);
    return isAlive_;
}

void Unit::start()
{
    // guarantee this unit is alive in the following impl function call
    boost::upgrade_lock<boost::shared_mutex> readLock(mtx_);
    if(!isAlive_){
        boost::upgrade_to_unique_lock<boost::shared_mutex> writeLock(readLock);
        isAlive_ = true;
        socket_->open();
        startImpl();
    }
}

void Unit::stop()
{
    // guarantee this unit is alive in the following impl function call
    boost::upgrade_lock<boost::shared_mutex> readLock(mtx_);
    if(isAlive_){
        boost::upgrade_to_unique_lock<boost::shared_mutex> writeLock(readLock);
        isAlive_ = false;
        socket_->close();
        stopImpl();
    }
}

void Unit::input(const PCMWave& wave)
{
    boost::shared_lock<boost::shared_mutex> lock(mtx_);
    if(isAlive_)    inputImpl(wave);
}

void Unit::send(const PCMWave& wave)
{
    socket_->write(wave);
}

void Unit::setSocketStatus(bool status)
{
    if(status)  socket_->open();
    else        socket_->close();
}

///

void ThreadOutUnit::startImpl()
{
    hasFinished_ = false;
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
                int interval = 100 - getInterval(prevTime, getNowTime());
                if(interval >= 10)  sleepms(interval == 100 ? 90 : interval);
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
