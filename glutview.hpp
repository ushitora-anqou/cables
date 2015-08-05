#pragma once
#ifndef ___GLUTVIEW_HPP___
#define ___GLUTVIEW_HPP___

#include "view.hpp"
#include "glut.hpp"
#include <boost/thread.hpp>

class GlutView : public View, glut::Window
{
private:
    boost::thread mainLooper_;
    boost::mutex mtx_;

    struct GroupData
    {
        std::string name;
        PCMWave::Sample sample;
    };
    std::vector<GroupData> data_;

public:
    GlutView(){}
    ~GlutView(){}

    UID issueGroup(const std::string& name);
    void updateLevelMeter(UID id, const PCMWave::Sample& sample);
};

#endif
