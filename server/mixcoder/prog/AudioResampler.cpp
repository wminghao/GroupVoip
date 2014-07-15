#include "AudioResampler.h"
#include "fwk/log.h"
#include <stdio.h>
#include <string.h>

//return the many samples
u32 AudioResampler::resample(u8* inputData, u32 totalInputBytes)
{
    //TODO different channel count
    int sampleCount = 0;
    if ( inputFreq_ == outputFreq_ ) {
        assert(inputChannels_ == 2); //Only take care of 2 channels case
        //direct copy w/o resampling
        memcpy(resampleShortBufOut_, inputData, totalInputBytes);
        sampleCount = totalInputBytes/(sizeof(short) * inputChannels_);
    } else {
        //convert to float
        src_short_to_float_array( (const short* )inputData,
                                  resampleFloatBufIn_,
                                  totalInputBytes/sizeof(short) );
        SRC_DATA srcData;
        srcData.data_in = resampleFloatBufIn_;
        srcData.data_out = resampleFloatBufOut_;
        srcData.input_frames = totalInputBytes/(sizeof(short) * inputChannels_);
        srcData.output_frames = 44100; /* 44100 big enough */
        srcData.src_ratio = ((double)outputFreq_)/((double)inputFreq_);
        srcData.end_of_input = 0;
        
        if( 0 != src_process( resamplerState_, &srcData ) ) {
            LOG( "src_proces FAILED\n");
            return 0;
        }
        sampleCount = srcData.output_frames_gen;
        //convert back to short
        if( inputChannels_ == 1 ) {
            src_float_to_short_array( (const float*) resampleFloatBufOut_,
                                      resampleShortBufOneChannel_,
                                      srcData.output_frames_gen * inputChannels_ );
            //duplicate the channels from left to right
            int j = sampleCount*2-1; 
            int i = sampleCount-1;
            while( i>=0 ) {
                short singleSample = resampleShortBufOneChannel_[i];
                resampleShortBufOut_[j--] = singleSample;
                resampleShortBufOut_[j--] = singleSample;
                i--;
            }
        } else {            
            src_float_to_short_array( (const float*) resampleFloatBufOut_,
                                      resampleShortBufOut_,
                                      srcData.output_frames_gen * inputChannels_ );
        }

        LOG( "resampling from %d to %d, srcData.input_frames=%ld, srcData.output_frames_gen=%ld\n", inputFreq_, outputFreq_, srcData.input_frames, srcData.output_frames_gen);
    }
    return sampleCount;
}
