#ifndef __AUDIO_RESAMPLER__
#define __AUDIO_RESAMPLER__

#include "fwk/Units.h"
extern "C" {
#include <samplerate.h> //resampling
}
#include <assert.h>

//an audio resampler from ffmpeg
class AudioResampler
{
 public:
 AudioResampler(int inputFreq, int inputChannels, int outputFreq, int outputChannels):
    inputFreq_(inputFreq), inputChannels_(inputChannels), outputFreq_(outputFreq), outputChannels_(outputChannels){
        /* resample */
        int error = 0;
        resamplerState_ = src_new( SRC_SINC_MEDIUM_QUALITY, inputChannels_, &error );
        assert(resamplerState_);
        assert(inputChannels_==1 || inputChannels_==2);
        assert(outputChannels_==2);
    }
    ~AudioResampler(){
        if( resamplerState_) {            
            src_delete( resamplerState_ );
            resamplerState_ = 0;
        }
    }
    
    //return the many samples
    u32 resample(u8* inputData, u32 totalInputBytes);

 private:
    /* Resample */
    SRC_STATE* resamplerState_;
    
    /* one second at 44 khz times two channels - its PLENTY */
    float resampleFloatBufIn_[44100 * 2];
    float resampleFloatBufOut_[44100 * 2];
    short resampleShortBufOneChannel_[44100];
    short resampleShortBufOut_[44100 * 2];

    /* Max of 44.1K buffer size */
    short resampleShortBufFrame_[44100];
    int frameLen_;

    //channel info
    int inputFreq_;
    int inputChannels_;
    int outputFreq_;
    int outputChannels_;
};
#endif //
