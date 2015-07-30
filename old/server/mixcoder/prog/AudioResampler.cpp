#include "AudioResampler.h"
#include "fwk/log.h"
#include <stdio.h>
#include <string.h>

//return the many samples
bool AudioResampler::resample(u8* inputData, u32 sampleSize)
{
    u32 totalInputBytes =  sampleSize * sizeof(short) * inputChannels_;
    int sampleCount = 0;
    if ( inputFreq_ == outputFreq_ ) {
        //direct copy w/o resampling
        memcpy(resampleShortBufOut_, inputData, totalInputBytes);
        sampleCount = sampleSize;
        //LOG("======no need for resampling, copy over sampleSize=%d\r\n", sampleSize);
    } else {
        //convert to float
        src_short_to_float_array( (const short* )inputData,
                                  resampleFloatBufIn_,
                                  totalInputBytes/sizeof(short) );
        SRC_DATA srcData;
        srcData.data_in = resampleFloatBufIn_;
        srcData.data_out = resampleFloatBufOut_;
        srcData.input_frames = sampleSize;
        srcData.output_frames = 44100; /* 44100 big enough */
        srcData.src_ratio = ((double)outputFreq_)/((double)inputFreq_);
        srcData.end_of_input = 0;
        
        if( 0 != src_process( resamplerState_, &srcData ) ) {
            LOG( "src_proces FAILED\n");
            return false;
        }
        sampleCount = srcData.output_frames_gen;

        //convert back to short
        src_float_to_short_array( (const float*) resampleFloatBufOut_,
                                  resampleShortBufOut_,
                                  sampleCount * inputChannels_ );
    }
    //push the samples into an linked list
    u32 samplesToSkip = 0;

    while( sampleCount > 0 ) {
        u32 samplesToCopyFromOutBuf = 0;

        if( remainingSampleCnt_ ) {
            u32 remainingBytes = remainingSampleCnt_ * sizeof(short) * inputChannels_;
            //copy the remaining sample count first
            if( remainingSampleCnt_ + sampleCount >= MP3_FRAME_SAMPLE_SIZE ) {
                SmartPtr<SmartBuffer> rawFrame = new SmartBuffer( MP3_FRAME_SAMPLE_SIZE * sizeof(short) * inputChannels_ );
                u8* mp3RawFrame = rawFrame->data();
                memcpy(mp3RawFrame, (u8*)resampleShortRemaining_, remainingBytes);
                samplesToCopyFromOutBuf = MP3_FRAME_SAMPLE_SIZE-remainingSampleCnt_;
                memcpy(mp3RawFrame + remainingBytes, (u8*)resampleShortBufOut_, samplesToCopyFromOutBuf * sizeof(short) * inputChannels_);
                samplesToSkip = samplesToCopyFromOutBuf;
                remainingSampleCnt_ = 0;
                mp3FrameList_.push_back( rawFrame );
                //LOG("======got a part-part frame, remainingSampleCnt_=%d, samplesToCopyFromOutBuf=%d===\r\n", remainingSampleCnt_, samplesToCopyFromOutBuf);
            } else {
                memcpy((u8*)resampleShortRemaining_+remainingBytes, (u8*)resampleShortBufOut_, sampleCount * sizeof(short) * inputChannels_);
                remainingSampleCnt_ += sampleCount;
                samplesToCopyFromOutBuf = sampleCount;
                samplesToSkip = 0;
                //LOG("======got nothing, remainingSampleCnt_=%d, samplesToCopyFromOutBuf=%d===\r\n", remainingSampleCnt_, samplesToCopyFromOutBuf);
            }
        } else {
            //no residual, so create a new buffer
            if( sampleCount >= MP3_FRAME_SAMPLE_SIZE ) {
                u32 bytesToCopy = MP3_FRAME_SAMPLE_SIZE * sizeof(short) * inputChannels_;
                SmartPtr<SmartBuffer> rawFrame = new SmartBuffer( bytesToCopy );
                u8* mp3RawFrame = rawFrame->data();
                samplesToCopyFromOutBuf = MP3_FRAME_SAMPLE_SIZE;
                memcpy(mp3RawFrame, (u8*)resampleShortBufOut_, bytesToCopy);
                remainingSampleCnt_ = (sampleCount - MP3_FRAME_SAMPLE_SIZE);
                samplesToSkip = samplesToCopyFromOutBuf;
                mp3FrameList_.push_back( rawFrame );
                //LOG("======got a brand new frame, remainingSampleCnt_=%d, samplesToCopyFromOutBuf=%d===\r\n", remainingSampleCnt_, samplesToCopyFromOutBuf);
            } else {
                memcpy((u8*)resampleShortRemaining_, (u8*)(resampleShortBufOut_+samplesToSkip*inputChannels_), sampleCount * sizeof(short) * inputChannels_);
                remainingSampleCnt_ = sampleCount;
                samplesToCopyFromOutBuf = sampleCount;
                samplesToSkip = 0;
                //LOG("======No residual, not enough for a frame, remainingSampleCnt_=%d, samplesToCopyFromOutBuf=%d===\r\n", remainingSampleCnt_, samplesToCopyFromOutBuf);
            }
        }
        sampleCount -= samplesToCopyFromOutBuf;
    }
    return true;
}

bool AudioResampler::isNextRawMp3FrameReady()
{
    return (mp3FrameList_.size() > 0);
}

//return a buffer, must be freed outside
SmartPtr<SmartBuffer> AudioResampler::getNextRawMp3Frame(bool& bIsStereo)
{
    SmartPtr<SmartBuffer> res;
    if( mp3FrameList_.size() > 0 ) {
        bIsStereo = (inputChannels_ == 2);
        res = mp3FrameList_.back();
        mp3FrameList_.pop_back();
    }
    return res;
}
