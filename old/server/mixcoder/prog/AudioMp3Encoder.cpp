#include "AudioMp3Encoder.h"
#include <assert.h>
#include <stdio.h>
#include <lame/lame.h>
#include "fwk/log.h"

void error_handler_function(const char *format, va_list ap)
{
    LOG( format, ap);
    //Not critical
    //assert(0);
}
AudioMp3Encoder::AudioMp3Encoder(AudioStreamSetting* outputSetting, int aBitrate):AudioEncoder(outputSetting, aBitrate)
{
    //speex encoder
    /*Create a new encoder state*/
    lgf_ = lame_init();
    if(lgf_) {
        lame_set_errorf(lgf_,error_handler_function);

        lame_set_num_channels(lgf_, getNumChannels(outputSetting->at));
        lame_set_in_samplerate(lgf_, getFreq(outputSetting->ar)); //always 44.1k
        lame_set_out_samplerate(lgf_, getFreq(outputSetting->ar)); //always 44.1k
        lame_set_brate(lgf_, aBitrate);
        lame_set_VBR(lgf_, vbr_off);
        lame_set_mode(lgf_, (outputSetting->at==kSndMono) ? MONO:STEREO);
        lame_set_quality(lgf_, 2);   /* 2=high  5 = medium  7=low 9=worst */
        lame_set_no_short_blocks(lgf_, 0); //no short block
        //lame_set_preset( m_gfp, 56); //voice quality
        //lame_set_lowpassfreq(m_gfp, -1);
        //lame_set_highpassfreq(m_gfp, -1);

        int ret_code = lame_init_params(lgf_);
        //LOG( "AudioMp3Encoder lame encoder initialized=%d, bitRate=%dkbps\n", ret_code, aBitrate);
        //ret_code < 0 means failed
    }
}
AudioMp3Encoder::~AudioMp3Encoder()
{
    lame_close(lgf_);
    lgf_ = NULL;
}

SmartPtr<SmartBuffer> AudioMp3Encoder::encodeAFrame(SmartPtr<SmartBuffer> input)
{
    SmartPtr<SmartBuffer> result = NULL;
    if ( input && input->dataLength() ) {
        int numChannels = getNumChannels(outputSetting_.at);
        int sampleSize = input->dataLength()/(sizeof(short)*numChannels);

        short in[sampleSize * numChannels];
        memcpy(in, input->data(), input->dataLength());

        int encodedSize = lame_encode_buffer_interleaved(lgf_,
                                                         in, sampleSize,
                                                         (u8*)encodedBits_, MAX_ENCODED_BYTES);
        
        if( encodedSize ) {
            //LOG("AudioMp3Encoder lame =====sample size=%d, channels=%d, encodedSize=%d, sampling rate=%d\n", sampleSize, numChannels, encodedSize, getFreq(outputSetting_.ar)); 
            int paddingSize = lame_encode_flush_nogap(lgf_, (u8*)encodedBits_+encodedSize, MAX_ENCODED_BYTES-encodedSize);
            encodedSize += paddingSize;
           
            //LOG("AudioMp3Encoder lame encoded pkt size=%d sample size=%d\n", encodedSize, sampleSize); 
            result = new SmartBuffer( encodedSize, encodedBits_);
        } else {
            //LOG("*** nothing encoded AudioMp3Encoder lame encoded pkt size=%d sample size=%d\n", encodedSize, sampleSize); 
        }
    }
    return result;
}
