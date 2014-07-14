#include "AudioSpeexDecoder.h"
#include "fwk/log.h"
#include <assert.h>
#include <stdio.h>

AudioSpeexDecoder::~AudioSpeexDecoder()
{
    speex_bits_destroy(&bits_);
    speex_decoder_destroy(decoder_); 
    free(outputFrame_);
}

SmartPtr<SmartBuffer> AudioSpeexDecoder::newAccessUnit( SmartPtr<AccessUnit> au , AudioStreamSetting* aRawSetting)
{
    assert(au->st == kAudioStreamType);
    SmartPtr<SmartBuffer> result;;
    
    if( au && au->payload ) { 
        speex_bits_read_from(&bits_, (char*)au->payload->data(), au->payload->dataLength());
        speex_decode_int(decoder_, &bits_, outputFrame_);
        result = new SmartBuffer( sampleSize_ * sizeof(u16), (u8*)outputFrame_);
        //LOG( "audio decoded pkt size=%ld sample size=%d ts=%d, streamId=%d\n", au->payload->dataLength(), sampleSize_, au->pts, streamId_);
        hasFirstFrameDecoded_ = true;
    }
    return result;
}

