#pragma once
#ifndef ___WAVEFILE_HPP___
#define ___WAVEFILE_HPP___

#include "pcmwave.hpp"
#include <cstdint>
#include <fstream>
#include <string>


// リニアPCM専用
struct PCMWaveFileHeader
{
    std::uint8_t  riffID[4];
    std::uint32_t size;
    std::uint8_t  waveID[4];
    std::uint8_t  fmtID[4];
    std::uint32_t fmtSize;
    std::uint16_t format;
    std::uint16_t channel;
    std::uint32_t sampleRate;
    std::uint32_t bytePerSec;
    std::uint16_t blockSize;
    std::uint16_t bitPerSample;
    std::uint8_t dataID[4];
    std::uint32_t dataSize;

    PCMWaveFileHeader(){}
    PCMWaveFileHeader(int) :
        riffID{'R', 'I', 'F', 'F'},
        waveID{'W', 'A', 'V', 'E'},
        fmtID{'f', 'm', 't', ' '},
        fmtSize(16), format(1),
        dataID{'d', 'a', 't', 'a'}
    {}
};

class WaveInFile
{
private:
    std::ifstream ifs_;

public:
    WaveInFile(const std::string& filename);
    ~WaveInFile(){}

    bool isEOF() const;
    PCMWave read();
};

class WaveOutFile
{
private:
    std::ofstream ofs_;

public:
    WaveOutFile(const std::string& filename);
    ~WaveOutFile();

    void write(const PCMWave& wave);
};

#endif
