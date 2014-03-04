#ifndef __AUDIOENCODER_H
#define __AUDIOENCODER_H

extern "C" {
#include <libavcodec/avcodec.h>    // required headers
#include <libavformat/avformat.h>
#include <samplerate.h>
}

#include <fwk/SmartBuffer.h>
#include <queue>
#include "CodecInfo.h"

//audio encoder implementation
class AudioEncoder
{
 public:
    AudioEncoder(int aBitrate, int frequency) :
        aBitrate_(aBitrate),
        aFrequency_(frequency)
        {
            //mp3 encoder
        }
 private:
    //output settings
    int aFrequency_;
    int aBitrate_;
};


#endif
