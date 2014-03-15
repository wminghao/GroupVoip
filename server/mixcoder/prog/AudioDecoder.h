#ifndef __AUDIODECODER_H
#define __AUDIODECODER_H

extern "C" {
#include <speex/speex.h>
}

#include "fwk/SmartBuffer.h"
#include <queue>
#include <stdlib.h>
#include "CodecInfo.h"
#include "MediaTarget.h"

//audio decoder implementation
class AudioDecoder:public MediaTarget
{
 public:
    //speex settings is always, 16khz, mono, 16bits audio
    AudioDecoder(){
        setting_.acid = kSpeex;
        setting_.at = kSndMono;
        setting_.ar = k16kHz;
        setting_.as = kSnd16Bit;
        setting_.ap = 0;
        
        /*codec structure*/
        speex_bits_init(&bits_);
        /*Create a new decoder state in wideband mode, 16khz*/
        decoder_ = speex_decoder_init(&speex_wb_mode);
        int tmp=1;
        speex_decoder_ctl(decoder_, SPEEX_SET_ENH, &tmp);

        speex_decoder_ctl(decoder_, SPEEX_GET_FRAME_SIZE, &frameSize_);  
        outputFrame_ = (short*)malloc(sizeof(short)*frameSize_);
    }
    ~AudioDecoder();
    //send it to the decoder
    virtual SmartPtr<SmartBuffer>  newAccessUnit( SmartPtr<AccessUnit> );
    
 private:
    /*Holds the state of the decoder*/
    void *decoder_;
    SpeexBits bits_;
    
    short* outputFrame_;
    int frameSize_;

    AudioStreamSetting setting_;
};
#endif
