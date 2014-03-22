#ifndef __VIDEOENCODER_H__
#define __VIDEOENCODER_H__

extern "C" {
#include <libavcodec/avcodec.h>    // required headers                                                                                                                                                      
#include <libavformat/avformat.h>
#include <samplerate.h>
}

#include "fwk/SmartBuffer.h"
#include <queue>
#include <stdio.h>
#include "CodecInfo.h"

//ensure strict compliance with the latest SDK by disabling some backwards compatibility features.
#define VPX_CODEC_DISABLE_COMPAT
#include "vpx/vpx_encoder.h"
#include "vpx/vp8cx.h"

#define VPX_TS_MAX_PERIODICITY 16

#define DEBUG_SAVE_IVF

//video encoder implementation, vp8 encoder   
class VideoEncoder
{
 public:
    VideoEncoder( VideoStreamSetting* setting, int vBaseLayerBitrate );
    ~VideoEncoder();
    SmartPtr<SmartBuffer> encodeAFrame(SmartPtr<SmartBuffer> input, bool* bIsKeyFrame);

 private:
    //input settings and output setting are the same
    VideoStreamSetting vSetting_;

    //output bitrate
    int vBaseLayerBitrate_;

    vpx_codec_ctx_t      codec_;
    vpx_codec_enc_cfg_t  cfg_;

    vpx_image_t          raw_;
    int                  layerFlags_[VPX_TS_MAX_PERIODICITY];

    int                  frameInputCnt_;
    int                  frameOutputCnt_;
    u32                  timestampTick_;
        
#ifdef DEBUG_SAVE_IVF
    FILE* outFile_;
#endif
};
#endif
