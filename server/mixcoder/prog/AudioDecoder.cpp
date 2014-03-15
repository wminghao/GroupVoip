#include "AudioDecoder.h"
#include <stdio.h>

AudioDecoder::~AudioDecoder()
{
    speex_bits_destroy(&bits_);
    speex_decoder_destroy(decoder_); 
    free(outputFrame_);
}

SmartPtr<SmartBuffer> AudioDecoder::newAccessUnit( SmartPtr<AccessUnit> au )
{
    assert(au->st == kAudioStreamType);
    SmartPtr<SmartBuffer> result;;
    
    if( au && au->payload ) { 
        speex_bits_read_from(&bits_, (char*)au->payload->data(), au->payload->dataLength());
        speex_decode_int(decoder_, &bits_, outputFrame_);
        result = new SmartBuffer( frameSize_ * sizeof(u16), (u8*)outputFrame_);
        fprintf( stderr, "audio got pkt size=%ld frame size=%d\n", au->payload->dataLength(), frameSize_);
    }
    return result;
}

