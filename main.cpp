#include "socket.hpp"
#include "helper.hpp"
#include <iostream>

#include <cmath>
class SinOutUnit : public WaitThreadOutUnit
{
private:
    std::vector<double> sinTable_;
    int p_;

public:
    SinOutUnit(int f);
    ~SinOutUnit(){}

    PCMWave update();
};

SinOutUnit::SinOutUnit(int f)
    : sinTable_(PCMWave::SAMPLE_RATE / f, 0), p_(0)
{
    int t = sinTable_.size();
    for(int i = 0;i < t;i++){
        sinTable_.at(i) = static_cast<double>(::sin(2 * 3.14159265358979323846264338 * i / t));
    }
}

PCMWave SinOutUnit::update()
{
    auto& tbl = sinTable_;
    PCMWave ret;
    for(auto& s : ret){
        s.left = tbl.at(p_);
        s.right = tbl.at(p_);
        p_ = (p_ + 1) % tbl.size();
    }

    return std::move(ret);
}

///

#include "wavefile.hpp"
// ファイルに書き込む
class FileInUnit : public Unit
{
private:
    WaveOutFile file_;

public:
    FileInUnit(const std::string& filename)
        : file_(filename)
    {}

    void inputImpl(const PCMWave& wave)
    {
        file_.write(wave);
    }
};

int main(int argc, char **argv)
{
    std::vector<UnitPtr> units = {
        makeUnit<SinOutUnit>(1000),
        makeUnit<SinOutUnit>(1000),
        makeUnit<FileInUnit>("test0.wav"),
        makeUnit<FileInUnit>("test1.wav"),
        makeUnit<FileInUnit>("test2.wav")
    };
    connect({units.at(0)}, {units.at(2), units.at(4)});
    connect({units.at(1)}, {units.at(3), units.at(4)});

    for(auto& unit : units) unit->start();
    sleepms(5000);
    for(auto& unit : units) unit->stop();


    return 0;
}
