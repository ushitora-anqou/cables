#pragma once
#ifndef ___HELPER_HPP___
#define ___HELPER_HPP___

#include <boost/thread.hpp>
#include <boost/preprocessor.hpp>
#include <memory>

#define TYPE_SIZE_ASSERT(type, size) \
    static_assert(sizeof((type)) == size, \
        "The size of \"" BOOST_PP_STRINGIZE((type)) "\" isn't " BOOST_PP_STRINGIZE((size)) " byte(s).");

template <class T, class... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

using te = int;
void sleepMilli(int delay)
{
    boost::this_thread::sleep(boost::posix_time::milliseconds(delay));
}

#endif
