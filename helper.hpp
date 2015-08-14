#pragma once
#ifndef ___HELPER_HPP___
#define ___HELPER_HPP___

#include <boost/preprocessor.hpp>
#include <boost/thread.hpp>
#include <cmath>
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

inline double calcDB(double src)
{
    return 20 * std::log10(std::abs(src));
}

inline double divd(int n0, int n1)
{
    return static_cast<double>(n0) / n1;
}

inline std::array<float, PCMWave::BUFFER_SIZE * 2> wave2float(const PCMWave& wave)
{
    std::array<float, PCMWave::BUFFER_SIZE * 2> ret;
    auto it = ret.begin();
    for(auto& s : wave){
        *(it++) = s.left;
        *(it++) = s.right;
    }
    return std::move(ret);
}

inline void sleepms(int delay)
{
    if(delay <= 0)  return;
    boost::this_thread::sleep(boost::posix_time::milliseconds(delay));
}

inline std::chrono::system_clock::time_point getNowTime()
{
    return std::chrono::system_clock::now();
}

inline int getInterval(const std::chrono::system_clock::time_point& beg, const std::chrono::system_clock::time_point& fin)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(fin - beg).count();
}

struct Rect
{
	double left, top, right, bottom;

	Rect(){}
	Rect(double left_, double top_, double right_, double bottom_)
		: left(left_), top(top_), right(right_), bottom(bottom_)
	{}
};

struct Color
{
	int r, g, b;

	Color(){}
    Color(int r_, int g_, int b_)
        : r(r_), g(g_), b(b_)
    {}

    static Color black()   { return Color(  0,   0,   0); }
    static Color white()   { return Color(255, 255, 255); }
    static Color red()     { return Color(255,   0,   0); }
    static Color green()   { return Color(  0, 255,   0); }
    static Color blue()    { return Color(  0,   0, 255); }
    static Color yellow()  { return Color(255, 255,   0); }
    static Color cyan()    { return Color(  0, 255, 255); }
    static Color magenta() { return Color(255,   0, 255); }
    static Color gray()    { return Color(128, 128, 128); }
};

#endif
