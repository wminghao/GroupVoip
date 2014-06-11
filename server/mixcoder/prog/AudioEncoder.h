#ifndef __AUDIOENCODER_H__
#define __AUDIOENCODER_H__

#include "fwk/SmartBuffer.h"
#include <queue>
#include "CodecInfo.h"

#define MAX_WB_BYTES 1000

//audio encoder implementation
class AudioEncoder
{
 public:
    //always encode in speex or mp3
    AudioEncoder(AudioStreamSetting* inputSetting, AudioStreamSetting* outputSetting, int aBitrate):aBitrate_(aBitrate) {
        memcpy(&inputSetting_, inputSetting, sizeof(AudioStreamSetting));
        memcpy(&outputSetting_, outputSetting, sizeof(AudioStreamSetting));
    }
    virtual ~AudioEncoder(){}
    virtual SmartPtr<SmartBuffer> encodeAFrame(SmartPtr<SmartBuffer> input) = 0;
 protected:
    //input settings
    AudioStreamSetting inputSetting_;

    //output settings
    AudioStreamSetting outputSetting_;
    int aBitrate_;
    
    //audio encoder
    void* encoder_;
    int frameSize_;
    
    char encodedBits_[MAX_WB_BYTES];
};

#endif
