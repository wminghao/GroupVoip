#ifndef __AUDIOENCODER_H__
#define __AUDIOENCODER_H__

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
 AudioEncoder(AudioStreamSetting* inputSetting, AudioStreamSetting* outputSetting, int aBitrate):aBitrate_(aBitrate)
    {
        //mp3 encoder
        memcpy(&inputSetting_, inputSetting, sizeof(AudioStreamSetting));
        memcpy(&outputSetting_, outputSetting, sizeof(AudioStreamSetting));
    }
    SmartPtr<SmartBuffer> encodeAFrame(SmartPtr<SmartBuffer> input);
 private:
    //input settings
    AudioStreamSetting inputSetting_;

    //output settings
    AudioStreamSetting outputSetting_;
    int aBitrate_;
};


#endif
