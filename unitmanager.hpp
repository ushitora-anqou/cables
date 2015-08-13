#pragma once
#ifndef ___UNITMANAGER_HPP___
#define ___UNITMANAGER_HPP___

#include "socket.hpp"
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>

class UnitManager
{
private:
    std::unordered_map<std::string, UnitPtr> units_;

public:
    UnitManager(){}
    ~UnitManager(){}

    template<class T, class... Args>
    void makeUnit(const std::string& name, Args&&... args)
    {
        units_.insert(std::make_pair(name, ::makeUnit<T>(std::forward<Args>(args)...)));
    }

    void connect(const std::vector<std::string>& from, const std::vector<std::string>& to)
    {
        std::vector<UnitPtr> fromUnits, toUnits;
        for(auto& str : from)   fromUnits.push_back(units_.at(str));
        for(auto& str : to)     toUnits.push_back(units_.at(str));
        ::connect(fromUnits, toUnits);
    }

    void startAll()
    {
        for(auto& unit : units_){
            if(!unit.second->isAlive())  unit.second->start();
        }
    }

    void stopAll()
    {
        for(auto& unit : units_){
            if(unit.second->isAlive())  unit.second->stop();
        }
    }

    template<class T>
    std::weak_ptr<T> getCastUnit(const std::string& name)
    {
        return std::dynamic_pointer_cast<T>(getUnit(name).lock());
    }

    std::weak_ptr<Unit> getUnit(const std::string& name)
    {
        return units_.at(name);
    }
};



#endif
