#ifndef __AUDIODECODER_H
#define __AUDIODECODER_H

#include "fwk/Units.h"
#include "CodecInfo.h"
#include "fwk/SmartBuffer.h"
#include <queue>
#include <stdlib.h>
#include "CodecInfo.h"
#include "MediaTarget.h"
#include "AudioResampler.h"

//audio decoder implementation
class AudioDecoder
{
 public:
    AudioDecoder(int streamId, AudioCodecId codecType, AudioRate audioRate, AudioSize audioSize, AudioType audioType) {
        setting_.acid = codecType;
        setting_.ar = audioRate;
        setting_.as = audioSize;
        setting_.at = audioType;
        setting_.ap = 0;
        
        hasFirstFrameDecoded_ = false;
        streamId_ = streamId;       
    }
    virtual ~AudioDecoder() {}
    //send it to the decoder, return the target settings for mixing
    virtual SmartPtr<SmartBuffer>  newAccessUnit( SmartPtr<AccessUnit> au, AudioStreamSetting* rawAudioSetting) = 0;

    bool hasFirstFrameDecoded(){ return hasFirstFrameDecoded_; }
    
    int getSampleSize() { return sampleSize_; }
    
 protected:
    //input audio setting
    AudioStreamSetting setting_;

    bool hasFirstFrameDecoded_;

    int streamId_;
    int sampleSize_;

    AudioResampler* resampler_;    
};
#endif
