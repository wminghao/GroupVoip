#ifndef __VIDEODECODER_H
#define __VIDEODECODER_H

#include "fwk/SmartBuffer.h"
#include <queue>
#include "CodecInfo.h"
#include "MediaTarget.h"

//video decoder implementation, h264 only
class VideoDecoder
{
 public:
 VideoDecoder():codec_(NULL), codecCtx_(NULL), frame_(NULL), inWidth_(0), inHeight_(0)
        {
            //avc decoder
            av_register_all();
        }
    ~VideoDecoder();
    virtual bool newAccessUnit( SmartPtr<AccessUnit> au, SmartPtr<SmartBuffer> plane[], int stride[], VideoStreamSetting* vInputSetting);

 private:
    void reset();
    void initDecoder( SmartPtr<SmartBuffer> spspps );

 private:
    
    AVCodec* codec_;
    AVCodecContext* codecCtx_;
    AVFrame* frame_;

    int inWidth_;
    int inHeight_;

    SmartPtr<SmartBuffer> spspps_;
};


#endif
