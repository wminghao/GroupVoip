#include "AudioDecoder.h"
#include <assert.h>
#include <stdio.h>

AudioDecoder::~AudioDecoder()
{
    speex_bits_destroy(&bits_);
    speex_decoder_destroy(decoder_); 
    free(outputFrame_);
}

SmartPtr<SmartBuffer> AudioDecoder::newAccessUnit( SmartPtr<AccessUnit> au , AudioStreamSetting* aInputSetting)
{
    assert(au->st == kAudioStreamType);
    SmartPtr<SmartBuffer> result;;
    
    if( au && au->payload ) { 
        speex_bits_read_from(&bits_, (char*)au->payload->data(), au->payload->dataLength());
        speex_decode_int(decoder_, &bits_, outputFrame_);
        result = new SmartBuffer( frameSize_ * sizeof(u16), (u8*)outputFrame_);
        fprintf( stderr, "audio decoded pkt size=%ld frame size=%d\n", au->payload->dataLength(), frameSize_);

        aInputSetting->acid = kSpeex;
        aInputSetting->at = kSndMono;
        aInputSetting->ar = k16kHz;
        aInputSetting->as = kSnd16Bit;
        aInputSetting->ap = 0;
    }
    return result;
}

