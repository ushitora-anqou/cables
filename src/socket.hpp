#pragma once
#ifndef ___SOCKET_HPP___
#define ___SOCKET_HPP___

#include "pcmwave.hpp"
#include <boost/thread.hpp>
#include <cstdint>
#include <map>
#include <memory>
#include <queue>

class Unit;

using UnitPtr = std::shared_ptr<Unit>;

template<class T, class... Args>
UnitPtr makeUnit(Args&&... args)
{
    return UnitPtr(new T(std::forward<Args>(args)...));
}
void connect(const std::vector<UnitPtr>& from, const std::vector<UnitPtr>& to);


class Unit : public std::enable_shared_from_this<Unit>
{
    friend void connect(const std::vector<UnitPtr>&, const std::vector<UnitPtr>&);

private:
    class Socket;
    using SocketPtr = std::shared_ptr<Unit::Socket>;

    class Socket : public std::enable_shared_from_this<Socket>  // thread safe
    {
        friend void connect(const std::vector<UnitPtr>&, const std::vector<UnitPtr>&);

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

private:
    bool isAlive_;
    boost::shared_mutex mtx_;
    SocketPtr socket_;

protected:
    void send(const PCMWave& wave);
    void setSocketStatus(bool isOpen);

public:
    Unit();
    virtual ~Unit();

    void connectTo(const UnitPtr& next);

    bool isAlive();
    bool canSocketSendToNext() const { return socket_->canSendToNext(); }

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
    virtual void destruct(){}
};

class WaitThreadOutUnit : public Unit
{
private:
    std::unique_ptr<boost::thread> proc_;
    bool hasFinished_;

public:
    WaitThreadOutUnit()
        : hasFinished_(false)
    {}
    virtual ~WaitThreadOutUnit(){}

    void startImpl();
    void stopImpl();
    virtual void construct(){}
    virtual PCMWave update() = 0;
    virtual void destruct(){}
};

#endif
