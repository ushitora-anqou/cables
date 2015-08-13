#pragma once
#ifndef ___HELPER_HPP___
#define ___HELPER_HPP___

#include <boost/preprocessor.hpp>
#include <cassert>
#include <chrono>
#include <memory>
#include <vector>

#define ZARU_ASSERT(res) \
    assert((res));

#define TYPE_SIZE_ASSERT(type, size) \
    static_assert(sizeof((type)) == size, \
        "The size of \"" BOOST_PP_STRINGIZE((type)) "\" isn't " BOOST_PP_STRINGIZE((size)) " byte(s).");

template <class T, class... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

void sleepms(int delay);
std::chrono::system_clock::time_point getNowTime();
int getInterval(const std::chrono::system_clock::time_point& beg, const std::chrono::system_clock::time_point& fin);

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
#endif
