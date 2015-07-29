#pragma once
#ifndef ___PCMWAVE_HPP___
#define ___PCMWAVE_HPP___

#include <cstdint>
#include <array>

class PCMWave
{
public:
    static const int
        SAMPLE_RATE = 44100,
        BUFFER_SIZE = SAMPLE_RATE * 0.1,
        CHANNEL_COUNT = 2,
        BIT_COUNT = 16,
        BYTE_PER_SEC = SAMPLE_RATE * CHANNEL_COUNT * BIT_COUNT / 8,
        BLOCK_SIZE = BIT_COUNT / 8 * CHANNEL_COUNT;

    struct Sample
    {
        float left, right;

        Sample(){}
        Sample(float l, float r)
            : left(l), right(r)
        {}

        Sample& operator+=(const Sample& rhs)
        {
            left += rhs.left;
            right += rhs.right;
            return *this;
        }

        Sample operator+(const Sample& rhs) const
        {
            Sample ret(*this);
            ret += rhs;
            return ret;
        }
    };

private:
    using Buffer = std::array<Sample, BUFFER_SIZE>;
    Buffer buffer_;

public:
    PCMWave(){}
    ~PCMWave(){}

    Sample& at(size_t index) { return buffer_.at(index); }
    const Sample& at(size_t index) const { return buffer_.at(index); }

    void fill(const Sample& value) { buffer_.fill(value); }

    Buffer::iterator begin() { return buffer_.begin(); }
    Buffer::const_iterator begin() const { return buffer_.cbegin(); }
    Buffer::iterator end() { return buffer_.end(); }
    Buffer::const_iterator end() const { return buffer_.cend(); }
};


#endif
