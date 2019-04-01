#ifndef WAVDATA_H
#define WAVDATA_H

#include <string>

class WavData
{
public:
    std::string filename;
    unsigned int channels;
    unsigned int samplesPerChannel;
    float *PwavData;

    static WavData *Load(std::string const &filename);
};

#endif // WAVDATA_H