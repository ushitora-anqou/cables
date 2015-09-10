#include "pitchshifter.hpp"
#include "wavefile.hpp"
#include "stopwatch.hpp"
#include <iostream>

int main()
{
    WaveInFile infile("in.wav");
    WaveOutFile outfile("out.wav");
    PitchShifter::Config config;
    config.pitch = 0.66;
    config.templateMS = 0.01;
    config.pminMS = 0.005;
    config.pmaxMS = 0.02;
    PitchShifter shifter(config);

    for(int i = 0;i < 1000;i++)
        outfile.write(shifter.proc(infile.read()));
}
