#pragma once
#ifndef ___SOCKET_HPP___
#define ___SOCKET_HPP___

#include "pcmwave.hpp"
#include <boost/thread.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <cstdint>
#include <map>
#include <memory>

class Unit;

using UnitPtr = std::shared_ptr<Unit>;
using UnitWeakPtr = std::weak_ptr<Unit>;

template<class T, class... Args>
UnitPtr makeUnit(Args&&... args)
{
    return UnitPtr(new T(std::forward<Args>(args)...));
}
void connect(const std::vector<UnitPtr>& from, const std::vector<UnitPtr>& to);


class Unit
{
    friend void connect(const std::vector<UnitPtr>&, const std::vector<UnitPtr>&);

private:
    class Socket;
    using SocketPtr = std::shared_ptr<Unit::Socket>;
    using SocketWeakPtr = std::weak_ptr<Unit::Socket>;

    class Socket
    {
        friend void connect(const std::vector<UnitPtr>&, const std::vector<UnitPtr>&);

    public:
        using UID = boost::uuids::uuid;

    private:
        UID uid_;
        std::map<UID, bool> fromFlags_;
        std::vector<SocketWeakPtr> toSockets_;
        PCMWave pool_;
        boost::mutex mtx_;
        Unit *parent_;

    private:
        void emitPool();

    public:
        Socket(Unit *parent)
            : uid_(boost::uuids::random_generator()()), parent_(parent)
        {}
        ~Socket(){}

        const UID& getUID() const { return uid_; }

        void write(const PCMWave& src);
        void onRecv(const UID& uid, const PCMWave& src);
    };

private:
    bool isAlive_;
    SocketPtr socket_;

protected:
    void send(const PCMWave& wave);

public:
    Unit();
    virtual ~Unit();

    void start();
    void stop();
    void input(const PCMWave& wave);

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
