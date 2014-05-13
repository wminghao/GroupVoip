#ifndef __VIDEOMIXER_H__
#define __VIDEOMIXER_H__

extern "C" {
#include <libswscale/swscale.h>
}

#include "fwk/SmartBuffer.h"
#include "MediaTarget.h"
#include "CodecInfo.h"

class VideoMixer
{
 public:
    VideoMixer(VideoStreamSetting* outputSetting):totalStreams_(0) {
        memcpy(&outputSetting_, outputSetting, sizeof(VideoStreamSetting));
        memset(swsCtx_, 0, sizeof(SwsContext*)*MAX_XCODING_INSTANCES);
    }
    ~VideoMixer();
    
    //do the mixing, for now, always mix n* 640*480 buffers into 1 640*480 buffer
    SmartPtr<SmartBuffer> mixStreams(SmartPtr<SmartBuffer> planes[][3], 
                                     int strides[][3], 
                                     VideoStreamSetting* settings, 
                                     int totalStreams,
                                     VideoRect* videoRect);
 private:
    bool tryToInitSws(VideoStreamSetting* settings, int totalStreams);
    void releaseSws();

 private:
    VideoStreamSetting outputSetting_;
    SwsContext* swsCtx_[MAX_XCODING_INSTANCES];
    int totalStreams_;
};


#endif
