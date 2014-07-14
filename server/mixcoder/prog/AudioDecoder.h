#ifndef __AUDIODECODER_H
#define __AUDIODECODER_H

#include "fwk/Units.h"
#include "CodecInfo.h"
#include "fwk/SmartBuffer.h"
#include <queue>
#include <stdlib.h>
#include "CodecInfo.h"
#include "MediaTarget.h"

extern "C" {
#include <samplerate.h> //resampling
}

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
    
    /* Resample */
    SRC_STATE* resamplerState_;
    /* one second at 44 khz times two channels - its PLENTY */
    float resampleFloatBufIn_[44100 * 2];
    float resampleFloatBufOut_[44100 * 2];
    short resampleShortBuf_[44100 * 2];

    /* Here, we accumulate one-frame's worth of samples to send to avcodec */
    short resampleShortBufFrame_[44100];
    int frameLen_;
};
#endif
