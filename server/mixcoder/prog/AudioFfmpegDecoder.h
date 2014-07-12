#ifndef __AUDIOFFMPEGDECODER_H
#define __AUDIOFFMPEGDECODER_H

extern "C" {
#include <libavcodec/avcodec.h>    // required headers
#include <libavformat/avformat.h>
#include <samplerate.h>
}

#include "AudioDecoder.h"

//audio speex decoder implementation
class AudioFfmpegDecoder:public AudioDecoder
{
 public:    
    AudioFfmpegDecoder(int streamId, AudioCodecId codecType, AudioRate audioRate, AudioSize audioSize, AudioType audioType):AudioDecoder(streamId, codecType, audioRate, audioSize, audioType) {}
    virtual ~AudioFfmpegDecoder();
    //send it to the decoder
    virtual SmartPtr<SmartBuffer>  newAccessUnit( SmartPtr<AccessUnit> au, AudioStreamSetting* aInputSetting);

 private:
    short* outputFrame_;

    /* decode */
    AVCodec* decoderInst_;
    AVCodecContext* decoderCtx_;
    AVFrame* decoderFrame_;
};
#endif
