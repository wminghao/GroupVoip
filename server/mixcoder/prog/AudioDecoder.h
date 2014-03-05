#ifndef __AUDIODECODER_H
#define __AUDIODECODER_H

extern "C" {
#include <libavcodec/avcodec.h>    // required headers
#include <libavformat/avformat.h>
#include <samplerate.h>
}

#include <fwk/SmartBuffer.h>
#include <queue>
#include "CodecInfo.h"
#include "MediaTarget.h"

//audio decoder implementation
class AudioDecoder:public MediaTarget
{
 public:
    AudioDecoder()
        {
            //speex decoder
            
        }
    virtual void newAccessUnit( SmartPtr<AccessUnit> );

    SmartPtr<SmartBuffer> getDecodedResult();
};


#endif
