#ifndef __AUDIOENCODER_H__
#define __AUDIOENCODER_H__

extern "C" {
#include <libavcodec/avcodec.h>    // required headers
#include <libavformat/avformat.h>
#include <samplerate.h>
}

#include "fwk/SmartBuffer.h"
#include <queue>
#include <speex/speex.h>
#include "CodecInfo.h"

#define MAX_WB_BYTES 1000

//audio encoder implementation
class AudioEncoder
{
 public:
    //always encode in speex
    AudioEncoder(AudioStreamSetting* inputSetting, AudioStreamSetting* outputSetting, int aBitrate);
    ~AudioEncoder();
    SmartPtr<SmartBuffer> encodeAFrame(SmartPtr<SmartBuffer> input);
 private:
    //input settings
    AudioStreamSetting inputSetting_;

    //output settings
    AudioStreamSetting outputSetting_;
    int aBitrate_;
    
    //audio encoder
    void* encoder_;
    SpeexBits bits_;
    int frameSize_;
    
    char encodedBits_[MAX_WB_BYTES];
};

#endif
