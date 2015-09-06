#include "group.hpp"
#include "calc.hpp"
#include "helper.hpp"

void GroupBase::updateWaveLevel(const PCMWave::Sample& sample)
{
	auto db = std::make_pair(
        sample.left  == 0 ? minfinity() : calcDB(sample.left),
        sample.right == 0 ? minfinity() : calcDB(sample.right));

    SCOPED_LOCK(mtx_);
    waveLevel_ = db;
}

std::pair<double, double> GroupBase::getWaveLevel()
{
    SCOPED_LOCK(mtx_);
    return waveLevel_;
}

