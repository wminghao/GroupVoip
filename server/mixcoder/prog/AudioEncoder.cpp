#include "AudioEncoder.h"

AudioEncoder::AudioEncoder(AudioStreamSetting* inputSetting, AudioStreamSetting* outputSetting, int aBitrate):aBitrate_(aBitrate)
{
    //speex encoder
    memcpy(&inputSetting_, inputSetting, sizeof(AudioStreamSetting));
    memcpy(&outputSetting_, outputSetting, sizeof(AudioStreamSetting));
    
    /*Create a new encoder state in wideband mode*/
    encoder_ = speex_encoder_init(&speex_wb_mode);
    speex_encoder_ctl(encoder_,SPEEX_GET_FRAME_SIZE,&frameSize_);
    speex_bits_init(&bits_);

    /*Set the quality to 8 (15 kbps)*/
    int qLevel=8;
    speex_encoder_ctl(encoder_, SPEEX_SET_QUALITY, &qLevel);
}
AudioEncoder::~AudioEncoder()
{
    /*Destroy the encoder state*/
    speex_encoder_destroy(encoder_);
    /*Destroy the bit-packing struct*/
    speex_bits_destroy(&bits_);
}

SmartPtr<SmartBuffer> AudioEncoder::encodeAFrame(SmartPtr<SmartBuffer> input)
{
    SmartPtr<SmartBuffer> result;
    if ( input && input->dataLength() ) {
        /*Flush all the bits in the struct so we can encode a new frame*/
        speex_bits_reset(&bits_);
        
        int sampleSize = input->dataLength()/2;
        
        short in[sampleSize];
        memcpy(in, input->data(), sampleSize*2);
        
        /*Encode the frame*/
        speex_encode_int(encoder_, in, &bits_);

        /*Copy the bits to an array of char that can be written*/
        int nbBytes = speex_bits_write(&bits_, encodedBits_, MAX_WB_BYTES);
        
        fprintf( stderr, "audio encoded pkt size=%d sample size=%d\n", nbBytes, sampleSize); 
        result = new SmartBuffer( nbBytes,  encodedBits_);
    }
    return result;
}
