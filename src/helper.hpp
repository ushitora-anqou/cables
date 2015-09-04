#pragma once
#ifndef ___HELPER_HPP___
#define ___HELPER_HPP___

#include "audio.hpp"
#include <memory>
#include <sstream>
#include <vector>

#define SCOPED_LOCK(mtx) \
    boost::mutex::scoped_lock lock((mtx));

template <class T, class... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template<class T, class Func>
void indexedForeach(std::vector<T>& container, Func func)
{
    for(size_t i = 0;i < container.size();i++){
        func(i, container.at(i));
    }
}

template<class T, class Func>
void indexedForeach(const std::vector<T>& container, Func func)
{
    for(size_t i = 0;i < container.size();i++){
        func(i, container.at(i));
    }
}

inline std::string toString(int src)
{
    std::stringstream ss;
    ss << src;
    return ss.str();
}

inline void writeDeviceInfo(std::ostream& os, const std::vector<AudioDevicePtr>& devices)
{
    indexedForeach(devices, [&os](int i, const AudioDevicePtr& dev){
        os <<
            i << ": " << dev->name() <<
            " (i:" << dev->inputChannel() <<
            " ,o:" << dev->outputChannel() <<
            ")" << std::endl;
    });
}

#endif
