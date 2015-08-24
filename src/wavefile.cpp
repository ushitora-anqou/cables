#include "wavefile.hpp"
#include "pcmwave.hpp"
#include <cassert>

PCMWave::Sample makeSample(std::int16_t left, std::int16_t right)
{
    PCMWave::Sample ret(
        left / static_cast<double>(0x7FFF),
        right / static_cast<double>(0x7FFF));
    return std::move(ret);
}

WaveInFile::WaveInFile(const std::string& filename)
    : ifs_(filename, std::ios::in | std::ios::binary)
{
    assert(ifs_);

    PCMWaveFileHeader header;
    ifs_.read(reinterpret_cast<char *>(&header), sizeof(header));
    assert(header.format == 1);
    assert(header.channel == PCMWave::CHANNEL_COUNT);
    assert(header.sampleRate == PCMWave::SAMPLE_RATE);
    assert(header.bytePerSec == PCMWave::BYTE_PER_SEC);
    assert(header.blockSize == PCMWave::BLOCK_SIZE);
    assert(header.bitPerSample == PCMWave::BIT_COUNT);
}

bool WaveInFile::isEOF() const
{
    return ifs_.eof();
}

PCMWave WaveInFile::read()
{
    std::array<std::int16_t, PCMWave::BUFFER_SIZE * PCMWave::CHANNEL_COUNT> buffer;
    ifs_.read(reinterpret_cast<char *>(buffer.data()), PCMWave::BUFFER_SIZE * PCMWave::CHANNEL_COUNT * 2);

    int readSize = ifs_.gcount() / (PCMWave::CHANNEL_COUNT * 2);
    PCMWave ret;
    for(int i = 0;i < readSize;i++)
        ret.at(i) = makeSample(buffer.at(i * 2), buffer.at(i * 2 + 1));
    for(int i = readSize;i < PCMWave::BUFFER_SIZE;i++)
        ret.at(i) = PCMWave::Sample(0.0, 0.0);

    return std::move(ret);
}

///

WaveOutFile::WaveOutFile(const std::string& filename)
    : ofs_(filename, std::ios::out | std::ios::binary | std::ios::trunc)
{
    assert(ofs_);
    ofs_.seekp(sizeof(PCMWaveFileHeader), std::ios::beg);
}

WaveOutFile::~WaveOutFile()
{
    int size = ofs_.tellp();
    PCMWaveFileHeader header(0);
    header.size = size - 8;
    header.channel = PCMWave::CHANNEL_COUNT;
    header.sampleRate = PCMWave::SAMPLE_RATE;
    header.bytePerSec = PCMWave::BYTE_PER_SEC;
    header.blockSize = PCMWave::BLOCK_SIZE;
    header.bitPerSample = PCMWave::BIT_COUNT;
    header.dataSize = size - 44;

    ofs_.seekp(0, std::ios::beg);
    ofs_.write(reinterpret_cast<char *>(&header), sizeof(header));
}

void WaveOutFile::write(const PCMWave& wave)
{
    for(auto s : wave){
        std::int16_t buf = 0;
        buf = s.left * 0x7FFF;
        ofs_.write(reinterpret_cast<char *>(&buf), 2);
        buf = s.right * 0x7FFF;
        ofs_.write(reinterpret_cast<char *>(&buf), 2);
    }
}

