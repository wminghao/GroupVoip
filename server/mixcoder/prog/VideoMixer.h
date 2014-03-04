#ifndef __VIDEOMIXER_H__
#define __VIDEOMIXER_H__

extern "C" {
#include <libavcodec/avcodec.h>    // required headers
#include <libavformat/avformat.h>
}

#include "fwk/SmartBuffer.h"
#include "MediaTarget.h"
#include "CodecInfo.h"

class VideoMixer
{
 public:
    VideoMixer() {}
    //do the mixing, for now, always mix n* 640*480 buffers into 1 640*480 buffer
    Ptr<SmartBuffer> mixStreams(Ptr<SmartBuffer> buffer[], 
                                VideoStreamSetting settings[], 
                                int totalStreams, 
                                int targetWidth,
                                int targetHeight);
};


#endif
