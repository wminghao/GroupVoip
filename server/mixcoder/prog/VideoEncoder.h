#ifndef __VIDEOENCODER_H__
#define __VIDEOENCODER_H__

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
 VideoEncoder( VideoStreamSetting* setting, int vBitrate ) : vBitrate_(vBitrate)
    {
        //vp8 encoder
        memcpy(&vSetting_, setting, sizeof(VideoStreamSetting));
    }

    SmartPtr<SmartBuffer> encodeAFrame(SmartPtr<SmartBuffer> input);

 private:
    //input settings and output setting are the same
    VideoStreamSetting vSetting_;

    //output bitrate
    int vBitrate_;
};
#endif
