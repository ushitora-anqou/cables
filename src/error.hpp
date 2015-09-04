#pragma once
#ifndef ___ERROR_HPP___
#define ___ERROR_HPP___

#include <iostream>
#include <boost/exception/all.hpp>
#include <boost/throw_exception.hpp>

#define ZARU_CHECK(msg) \
    std::cerr << \
        "ERROR ( " << \
        __FILE__ << ", " << __LINE__ << ")\n\t" << \
        msg << std::endl;
#define ZARU_CHECK_IF(ret) \
    if((ret)) { ZARU_CHECK(#ret); }
#define ZARU_THROW_IF(ret) \
    if((ret)) { BOOST_THROW_EXCEPTION(CablesError()); }
#define ZARU_THROW_UNLESS(ret) ZARU_THROW_IF(!(ret));

class CablesError : public boost::exception, public std::exception
{
private:
    std::string detail_;

public:
    CablesError()
    {
        detail_ = boost::diagnostic_information(*this);
    }
    const char *what() const noexcept
    {
        return detail_.c_str();
    }
};

#endif
