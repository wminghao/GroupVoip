#include "VideoMixer.h"
#include <assert.h>

int mappingToScalingWidth(int totalStream) {
    if(totalStream == 1) {
        return 640;
    } else {
        return 320;
    }
}
int mappingToScalingHeight(int totalStream) {
    if(totalStream == 1) {
        return 480;
    } else {
        return 240;
    }
}

//do the mixing, for now, always mix n raw streams into 1 rawstream
SmartPtr<SmartBuffer> VideoMixer::mixStreams(SmartPtr<SmartBuffer> planes[][3],
                                             int strides[][3],
                                             VideoStreamSetting* settings, 
                                             int totalStreams)
{
    if( tryToInitSws(settings, totalStreams) ) {
        //TODO do mixing
    }
    return new SmartBuffer(4, "TODO");
}
VideoMixer::~VideoMixer()
{
    releaseSws();
}
void VideoMixer::releaseSws()
{
    for(int i=0; i<MAX_XCODING_INSTANCES; i++) {
        if( swsCtx_[i] ) {
            sws_freeContext( swsCtx_[i] );
            swsCtx_[i] = 0;
        }
    }
}
bool VideoMixer::tryToInitSws(VideoStreamSetting* settings, int totalStreams)
{
    bool ret = true;
    if( mappingToScalingWidth(totalStreams_) != mappingToScalingWidth(totalStreams) ) {
        releaseSws();
        int outputWidth = mappingToScalingWidth(totalStreams);
        int outputHeight = mappingToScalingHeight(totalStreams);
        for(int i=0;  i<MAX_XCODING_INSTANCES; i++) {
            swsCtx_[i] = sws_getCachedContext( swsCtx_[i], settings[i].width, settings[i].height, PIX_FMT_YUV420P,
                                            outputWidth, outputHeight, PIX_FMT_YUV420P,
                                            SWS_BICUBIC, 0, 0, 0 );
            if( !swsCtx_[i] ) {
                fprintf( stderr, "FAILED to create swscale context\n");
                assert(0);
                ret = false;
            }
        }

    }
    totalStreams_ = totalStreams;
    return ret;
}
