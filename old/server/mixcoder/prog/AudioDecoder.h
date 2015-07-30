#ifndef __AUDIODECODER_H
#define __AUDIODECODER_H

#include "fwk/Units.h"
#include "CodecInfo.h"
#include "fwk/SmartBuffer.h"
#include <queue>
#include <stdlib.h>
#include "CodecInfo.h"
#include "AccessUnit.h"
#include "AudioResampler.h"

//audio decoder implementation
class AudioDecoder
{
 public:
    AudioDecoder(int streamId, AudioCodecId codecType, AudioRate audioRate, AudioSize audioSize, AudioType audioType):resampler_(NULL) {
        setting_.acid = codecType;
        setting_.ar = audioRate;
        setting_.as = audioSize;
        setting_.at = audioType;
        setting_.ap = 0;
        
        hasFirstFrameDecoded_ = false;
        streamId_ = streamId;       
    }
    virtual ~AudioDecoder() {
        delete( resampler_ );
    }
    //send it to the decoder, return the target settings for mixing
    virtual void newAccessUnit( SmartPtr<AccessUnit> au, AudioStreamSetting* rawAudioSetting) = 0;

    const AudioStreamSetting* getAudioInputSetting() const { return &setting_; }

    bool hasFirstFrameDecoded(){ return hasFirstFrameDecoded_; }
    
    //get the next batch of mp3 1152 samples
    bool isNextRawMp3FrameReady() {
        if( resampler_ ) {
            return resampler_->isNextRawMp3FrameReady();
        }
        return false;
    }

    //return a buffer, must be freed outside
    SmartPtr<SmartBuffer>  getNextRawMp3Frame(bool& bIsStereo) {
        SmartPtr<SmartBuffer> result;
        if( resampler_ ) {
            result = resampler_->getNextRawMp3Frame(bIsStereo);
        }
        return result;
    }

    /*
    void discardResamplerResidual() {
        if( resampler_ ) {
            resampler_->discardResidual();
        }
    }
    */

 protected:    
    //send it to resampler
    void resampleFrame(AudioStreamSetting* aRawSetting, int sampleSize, u8* outputFrame) {
        if( !resampler_ ) {
            resampler_ = new AudioResampler( getFreq(setting_.ar), getNumChannels(setting_.at), getFreq(aRawSetting->ar), getNumChannels(aRawSetting->at));
        }
        if( resampler_ ) {
            resampler_->resample(outputFrame, sampleSize);
        }    
    }
 protected:
    //input audio setting
    AudioStreamSetting setting_;

    bool hasFirstFrameDecoded_;

    int streamId_;
    int sampleSize_;

    AudioResampler* resampler_;    
};
#endif