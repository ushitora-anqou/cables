#include "helper.hpp"
#include <boost/thread.hpp>

void sleepms(int delay)
{
    if(delay <= 0)  return;
    boost::this_thread::sleep(boost::posix_time::milliseconds(delay));
}

std::chrono::system_clock::time_point getNowTime()
{
    return std::chrono::system_clock::now();
}

int getInterval(const std::chrono::system_clock::time_point& beg, const std::chrono::system_clock::time_point& fin)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(fin - beg).count();
}
