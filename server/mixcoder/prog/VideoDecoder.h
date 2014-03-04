#ifndef __VIDEODECODER_H
#define __VIDEODECODER_H

extern "C" {
#include <libavcodec/avcodec.h>    // required headers
#include <libavformat/avformat.h>
#include <samplerate.h>
}

#include <fwk/SmartBuffer.h>
#include <queue>
#include "CodecInfo.h"

//video decoder implementation
class VideoDecoder
{
 public:
    VideoDecoder()
        {
            //avc decoder
            
        }
 private:
};


#endif
