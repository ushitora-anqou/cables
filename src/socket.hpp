#pragma once
#ifndef ___SOCKET_HPP___
#define ___SOCKET_HPP___

#include "pcmwave.hpp"
#include <boost/thread.hpp>
#include <cstdint>
#include <atomic>
#include <map>
#include <memory>
#include <queue>

class Unit;
class Socket;

using UnitPtr = std::shared_ptr<Unit>;
using SocketPtr = std::shared_ptr<Socket>;

// 24時間テレビ38「愛は地球を救う」（2015年）
// つなぐ　〜時を超えて笑顔を〜
void connect(const std::vector<UnitPtr>& from, const std::vector<UnitPtr>& to);
// 与えられたsocketからのデータの入手可能性を調べる
// 再帰的に前のSocketを調べて、canSendToNext() == trueの一本線が引ければtrue
bool isAvailableSocket(const SocketPtr& socket);

class Socket : public std::enable_shared_from_this<Socket>  // thread safe
{
    friend void connect(const std::vector<UnitPtr>&, const std::vector<UnitPtr>&);
    friend bool isAvailableSocket(const SocketPtr&);

private:
    boost::mutex recvMtx_, sendMtx_;
    bool canRecvFromPrev_, canSendToNext_;
    std::vector<SocketPtr> nextSockets_;
    std::map<SocketPtr, std::deque<PCMWave>> recvPool_;
    Unit &parent_;

private:
    void emitPool();

public:
    Socket(Unit& parent);
    ~Socket(){}

    void open();
    void close();
    void addNextSocket(const SocketPtr& next);
    void addPrevSocket(const SocketPtr& prev);

    bool canSendToNext() const { return canSendToNext_; }
    bool canRecvFromPrev() const { return canRecvFromPrev_; }

    void write(const PCMWave& src);
    void onRecv(const SocketPtr& sender, const PCMWave& src);
};

class Unit : public std::enable_shared_from_this<Unit>
{
    friend void connect(const std::vector<UnitPtr>&, const std::vector<UnitPtr>&);
    
private:
    bool isAlive_;
    boost::shared_mutex mtx_;
    SocketPtr socket_;
    std::atomic<int> isMute_;

    const PCMWave emptyWave_;

protected:
    void send(const PCMWave& wave);
    void setSocketStatus(bool isOpen);

public:
    Unit();
    virtual ~Unit();

    void connectTo(const UnitPtr& next);
    void setMute(bool isMute);

    bool isAlive() const;
    bool isMute() const;
    bool canSocketSendToNext() const { return socket_->canSendToNext(); }
    bool canSendContent() const { return isAlive() && !isMute() && canSocketSendToNext(); }

    void start();
    void stop();
    void input(const PCMWave& wave);

//protected:    //TODO
    virtual void startImpl(){}
    virtual void stopImpl(){}
    virtual void inputImpl(const PCMWave& wave){}

};

class ThreadOutUnit : public Unit
{
private:
    std::unique_ptr<boost::thread> proc_;
    bool hasFinished_;

public:
    ThreadOutUnit()
        : hasFinished_(false)
    {}
    virtual ~ThreadOutUnit(){}

    void startImpl();
    void stopImpl();
    virtual void construct(){}
    virtual PCMWave update() = 0;
    virtual void destruct() noexcept {}
};

#endif
