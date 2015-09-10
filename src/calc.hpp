#pragma once
#ifndef ___CALC_HPP___
#define ___CALC_HPP___

#include <cmath>
#include <limits>

constexpr double infinity() noexcept
{
    return std::numeric_limits<double>::infinity();
}

constexpr double minfinity() noexcept
{
    return -std::numeric_limits<double>::infinity();
}

constexpr double pi() noexcept
{
    return 3.14159265358979323846264338;
}

inline double calcDB(double src)
{
    return 20 * std::log10(std::abs(src));
}

inline double divd(int n0, int n1)
{
    return static_cast<double>(n0) / n1;
}

inline double sinc(double x)
{
    return x == 0 ? 1.0 : std::sin(x) / x;
}

template<class T>
bool isIn(const T& lhs, const T& src, const T& rhs)
{
    return lhs < src && src < rhs;
}

template<class T>
bool isInEq(const T& lhs, const T& src, const T& rhs)
{
    return lhs <= src && src <= rhs;
}


#endif
