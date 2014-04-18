#include "AudioMp3Encoder.h"
#include <assert.h>
#include <stdio.h>
#include <lame/lame.h>

void error_handler_function(const char *format, va_list ap)
{
    assert(0);
}
AudioMp3Encoder::AudioMp3Encoder(AudioStreamSetting* inputSetting, AudioStreamSetting* outputSetting, int aBitrate):AudioEncoder(inputSetting, outputSetting, aBitrate)
{
    //speex encoder
    /*Create a new encoder state in wideband mode*/
    lgf_ = lame_init();
    if(lgf_) {
        lame_set_errorf(lgf_,error_handler_function);

        lame_set_num_channels(lgf_,1); //1 channel now
        lame_set_in_samplerate(lgf_, 16000);
        lame_set_out_samplerate(lgf_, 16000);
        lame_set_brate(lgf_, aBitrate>>10);
        lame_set_VBR(lgf_, vbr_off);
        lame_set_mode(lgf_,MONO);//mono
        lame_set_quality(lgf_,7);   /* 2=high  5 = medium  7=low 9=worst */
        lame_set_no_short_blocks(lgf_, 0); //no short block
        //lame_set_preset( m_gfp, 56); //voice quality
        //lame_set_lowpassfreq(m_gfp, -1);
        //lame_set_highpassfreq(m_gfp, -1);

        int ret_code = lame_init_params(lgf_);
        fprintf(stderr, "MP3Encoder::MP3Encoder lame encoder initialized=%d, bitRate=%d\n", ret_code, aBitrate);
    }
}
AudioMp3Encoder::~AudioMp3Encoder()
{
    lame_close(lgf_);
    lgf_ = NULL;
}

SmartPtr<SmartBuffer> AudioMp3Encoder::encodeAFrame(SmartPtr<SmartBuffer> input)
{
    SmartPtr<SmartBuffer> result;
    if ( input && input->dataLength() ) {

        int sampleSize = input->dataLength()/2;
        short in[sampleSize];
        memcpy(in, input->data(), sampleSize*2);

        int encodedSize = lame_encode_buffer(lgf_,
                                             in, in, sampleSize,
                                             (u8*)encodedBits_, MAX_WB_BYTES);
        int paddingSize = lame_encode_flush_nogap(lgf_, (u8*)encodedBits_+encodedSize, MAX_WB_BYTES-encodedSize);
        encodedSize += paddingSize;
        
        fprintf( stderr, "audio mp3 encoded pkt size=%d sample size=%d\n", encodedSize, sampleSize); 
        result = new SmartBuffer( encodedSize, encodedBits_);
    }
    return result;
}
