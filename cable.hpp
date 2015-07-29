#pragma once
#ifndef ___CABLE_HPP___
#define ___CABLE_HPP___

#include "pcmwave.hpp"
#include <boost/thread.hpp>
#include <memory>
#include <vector>

class Unit;
class Cable;

using UnitPtr = std::shared_ptr<Unit>;
using UnitWeakPtr = std::weak_ptr<Unit>;
using CablePtr = std::shared_ptr<Cable>;
using CableWeakPtr = std::weak_ptr<Cable>;

template<class T, class... Args>
UnitPtr makeUnit(Args&&... args)
{
    return UnitPtr(new T(std::forward<Args>(args)...));
}
CablePtr connect(const std::vector<UnitPtr>& from, const std::vector<UnitPtr>& to);

class Socket
{
private:
    int index_;
    const CableWeakPtr cable_;

public:
    Socket(int index, const CableWeakPtr& cable)
        : index_(index), cable_(cable)
    {}

    void write(const PCMWave& wave) const;
};

class Unit
{
    friend CablePtr connect(const std::vector<UnitPtr>&, const std::vector<UnitPtr>&);

private:
    bool isAlive_;
    std::unique_ptr<Socket> socket_;

protected:
    void send(const PCMWave& wave)
    {
        assert(socket_);
        socket_->write(wave);
    }

public:
    Unit(){}
    virtual ~Unit();

    void start();
    void stop();
    void input(const PCMWave& wave);

    virtual void startImpl(){}
    virtual void stopImpl(){}
    virtual void inputImpl(const PCMWave& wave){}
};

class Cable
{
private:
    std::vector<UnitWeakPtr> units_;
    PCMWave pool_;
    int flagWriter_;
    boost::mutex mtx_;

public:
    Cable(std::vector<UnitWeakPtr> units);
    ~Cable(){}

    void write(int index, const PCMWave& src);
};

#endif
