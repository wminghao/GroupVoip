#ifndef __AUDIOSPEEXDECODER_H
#define __AUDIOSPEEXDECODER_H

extern "C" {
#include <speex/speex.h>
}

#include "AudioDecoder.h"

//audio speex decoder implementation
class AudioSpeexDecoder:public AudioDecoder
{
 public:
    //speex settings is always, 16khz, mono, 16bits audio
    AudioSpeexDecoder(int streamId, AudioCodecId codecType, AudioRate audioRate, AudioSize audioSize, AudioType audioType):AudioDecoder(streamId, codecType, audioRate, audioSize, audioType) { 
        /*codec structure*/
        speex_bits_init(&bits_);
        /*Create a new decoder state in wideband mode, 16khz*/
        decoder_ = speex_decoder_init(&speex_wb_mode);
        int tmp=1;
        speex_decoder_ctl(decoder_, SPEEX_SET_ENH, &tmp);

        speex_decoder_ctl(decoder_, SPEEX_GET_FRAME_SIZE, &sampleSize_);  
        outputFrame_ = (short*)malloc(sizeof(short)*sampleSize_);
    }
    virtual ~AudioSpeexDecoder();
    //send it to the decoder
    virtual SmartPtr<SmartBuffer>  newAccessUnit( SmartPtr<AccessUnit> au, AudioStreamSetting* aInputSetting);
    
 private:
    /*Holds the state of the decoder*/
    void *decoder_;
    SpeexBits bits_;
    
    short* outputFrame_;
};
#endif
