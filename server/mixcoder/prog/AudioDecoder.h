#ifndef __AUDIODECODER_H
#define __AUDIODECODER_H

extern "C" {
#include <libavcodec/avcodec.h>    // required headers
#include <libavformat/avformat.h>
#include <samplerate.h>
}

#include "fwk/SmartBuffer.h"
#include <queue>
#include "CodecInfo.h"
#include "MediaTarget.h"

//audio decoder implementation
class AudioDecoder:public MediaTarget
{
 public:
    //speex settings is always, 16khz, mono, 16bits audio
    AudioDecoder(){
        setting_.acid = kSpeex;
        setting_.at = kSndMono;
        setting_.ar = k16kHz;
        setting_.as = kSnd16Bit;
        setting_.ap = 0;
    }
    //send it to the decoder
    virtual SmartPtr<SmartBuffer>  newAccessUnit( SmartPtr<AccessUnit> );
    
 private:
    AudioStreamSetting setting_;
};
#endif
