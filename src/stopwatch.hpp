#pragma once
#ifndef ___STOPWATCH_HPP___
#define ___STOPWATCH_HPP___

#include <chrono>

template<class Duration = std::chrono::milliseconds>
class StopWatch
{
private:
    using system_clock = std::chrono::system_clock;

private:
    system_clock::time_point start_;

public:
    StopWatch()
        : start_(system_clock::now())
    {}
    ~StopWatch(){}

    void restart() { start_ = system_clock::now(); }
    decltype(Duration().count()) elapsed()
    {
        auto end = system_clock::now();
        auto dur = end - start_;
        return std::chrono::duration_cast<Duration>(dur).count();
    }
};

#endif
