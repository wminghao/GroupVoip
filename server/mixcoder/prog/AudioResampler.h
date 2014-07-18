#ifndef __AUDIO_RESAMPLER__
#define __AUDIO_RESAMPLER__

#include "fwk/Units.h"
extern "C" {
#include <samplerate.h> //resampling
}
#include <assert.h>
#include <list>
#include <stdlib.h>


//each mp3 frame, contains 1152 samples
#define MP3_FRAME_SAMPLE_SIZE 1152
#define MP3_SAMPLE_PER_SEC 44100

//an audio resampler from ffmpeg
class AudioResampler
{
 public:
 AudioResampler(int inputFreq, int inputChannels, int outputFreq, int outputChannels):
    inputFreq_(inputFreq), inputChannels_(inputChannels), outputFreq_(outputFreq), outputChannels_(outputChannels), remainingSampleCnt_(0){
        /* resample */
        alloc();

        frameSize_ = MP3_FRAME_SAMPLE_SIZE * sizeof(short) * outputChannels_;
        assert(inputChannels_==1 || inputChannels_==2);
        assert(outputChannels_==2);
    }
    ~AudioResampler(){
        reset();
    }
    
    //return success or failure
    bool resample(u8* inputData, u32 sampleSize);

    //get the next batch of mp3 1152 samples
    bool isNextRawMp3FrameReady();
    //return a buffer, must be freed outside
    u8* getNextRawMp3Frame(u32& totalBytes);

    //when a timestamp jump happens, discard the previous resampler residual
    void discardResidual();

 private:
    void alloc() {
        int error = 0;
        resamplerState_ = src_new( SRC_SINC_MEDIUM_QUALITY, inputChannels_, &error );
        assert(resamplerState_);
    }
    void reset() {
        if( resamplerState_) {            
            src_delete( resamplerState_ );
            resamplerState_ = 0;
        }
        while( mp3FrameList_.size() > 0 ) {
            u8* res = mp3FrameList_.back();
            mp3FrameList_.pop_back();
            free( res );
        }
        remainingSampleCnt_ = 0;
    }

 private:
    /* Resample */
    SRC_STATE* resamplerState_;
    
    /* temp buffers, one second at 44 khz times two channels - its PLENTY */
    float resampleFloatBufIn_[44100 * 2];
    float resampleFloatBufOut_[44100 * 2];
    short resampleShortBufOneChannel_[44100];
    short resampleShortBufOut_[44100 * 2];

    //channel info
    int inputFreq_;
    int inputChannels_;
    int outputFreq_;
    int outputChannels_;

    //linked list of mp3 raw frame of 1152 samples
    std::list<u8*> mp3FrameList_; // integer list
    short resampleShortRemaining_[MP3_FRAME_SAMPLE_SIZE * 2]; //save reamining data from the previous read
    u32 remainingSampleCnt_;

    //mp3 frame size
    u32 frameSize_;
};
#endif //
