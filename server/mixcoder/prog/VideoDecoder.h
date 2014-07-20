#ifndef __VIDEODECODER_H
#define __VIDEODECODER_H

extern "C" {
#include <libavcodec/avcodec.h>    // required headers
#include <libavformat/avformat.h>
}
#include "fwk/SmartBuffer.h"
#include <queue>
#include "CodecInfo.h"
#include "MediaTarget.h"
#include "RawData.h"

//video decoder implementation, h264 only
class VideoDecoder
{
 public:
 VideoDecoder(int streamId):codec_(NULL), codecCtx_(NULL), frame_(NULL), inWidth_(0), inHeight_(0), bHasFirstFrameStarted(false), streamId_(streamId)
        {
            //avc decoder
            av_register_all();
        }
    ~VideoDecoder();
    bool newAccessUnit( SmartPtr<AccessUnit> au, SmartPtr<VideoRawData> v);
    bool hasFirstFrameDecoded() { return bHasFirstFrameStarted; }
 private:
    void reset();
    void initDecoder( SmartPtr<SmartBuffer> spspps );

 private:
    
    AVCodec* codec_;
    AVCodecContext* codecCtx_;
    AVFrame* frame_;

    int inWidth_;
    int inHeight_;
    bool bHasFirstFrameStarted;
    int streamId_;

    SmartPtr<SmartBuffer> spspps_;
};


#endif
