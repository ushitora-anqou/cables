#include "cable.hpp"
#include "helper.hpp"
#include <iostream>

class WhileOutUnit : public Unit
{
private:
    std::unique_ptr<boost::thread> proc_;
    bool hasFinished_;

public:
    WhileOutUnit()
        : hasFinished_(false)
    {}
    virtual ~WhileOutUnit(){}

    void startImpl();
    void stopImpl();
    virtual void construct(){}
    virtual PCMWave update() = 0;
    virtual void destruct(){}
};

void WhileOutUnit::startImpl()
{
    proc_ = make_unique<boost::thread>(
        [this](){
            construct();
            while(!hasFinished_){
                PCMWave wave(std::move(update()));
                if(!hasFinished_)   send(wave);
            }
            destruct();
        }
    );
}

void WhileOutUnit::stopImpl()
{
    hasFinished_ = true;
    proc_->join();
}

///

#include <cmath>
class SinOutUnit : public WhileOutUnit
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

    sleepMilli(80);

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
        makeUnit<FileInUnit>("test.wav")
    };
    std::vector<CablePtr> cables = {
        connect({units.at(0)}, {units.at(1)})
    };

    for(auto& unit : units) unit->start();
    sleepMilli(5000);
    for(auto& unit : units) unit->stop();


    return 0;
}
