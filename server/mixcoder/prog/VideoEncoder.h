#ifndef __VIDEOENCODER_H
#define __VIDEOENCODER_H

extern "C" {
#include <libavcodec/avcodec.h>    // required headers                                                                                                                                                      
#include <libavformat/avformat.h>
#include <samplerate.h>
}

#include "fwk/SmartBuffer.h"
#include <queue>
#include "CodecInfo.h"

//video encoder implementation, vp8 encoder   
class VideoEncoder
{
 public:
    VideoEncoder(int vBitrate, int width, VideoStreamSetting* inputSetting) : vBitrate_(vBitrate), vWidth_(width)
    {
        //vp8 encoder
        memcpy(&inputSetting_, inputSetting, sizeof(VideoStreamSetting));
    }
    SmartPtr<SmartBuffer> encodeOneFrame(SmartPtr<SmartBuffer> input);
 private:
    //input settings
    VideoStreamSetting inputSetting_;

    //output settings                                                                                                                                                                                     
    int vBitrate_;
    int vWidth_;
};
#endif
