#ifndef __AUDIOENCODER_H
#define __AUDIOENCODER_H

extern "C" {
#include <libavcodec/avcodec.h>    // required headers
#include <libavformat/avformat.h>
#include <samplerate.h>
}

#include "fwk/SmartBuffer.h"
#include <queue>
#include "CodecInfo.h"

//audio encoder implementation
class AudioEncoder
{
 public:
    //always encode in mp3
    AudioEncoder(int aBitrate, int frequency, AudioStreamSetting* inputSetting) : aBitrate_(aBitrate), aFrequency_(frequency)
    {
        //mp3 encoder
        memcpy(&inputSetting_, inputSetting, sizeof(AudioStreamSetting));
    }
    SmartPtr<SmartBuffer> encodeOneFrame(SmartPtr<SmartBuffer> input);
 private:
    //input settings
    AudioStreamSetting inputSetting_;

    //output settings
    int aFrequency_;
    int aBitrate_;
};


#endif
