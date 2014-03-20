#ifndef __FLVOUTPUT_H__
#define __FLVOUTPUT_H__

#include "fwk/SmartBuffer.h"
#include "CodecInfo.h"

class FLVOutput
{
 public:
    //vp8 video + mp3 audio
    FLVOutput(VideoStreamSetting* videoSetting, AudioStreamSetting* audioSetting)
        {
            memcpy(&videoSetting_, videoSetting, sizeof(VideoStreamSetting));
            memcpy(&audioSetting_, audioSetting, sizeof(AudioStreamSetting));
        }
    
    SmartPtr<SmartBuffer> newHeader();
    
    SmartPtr<SmartBuffer> packageVideoFrame(SmartPtr<SmartBuffer> videoPacket, u32 ts);
    SmartPtr<SmartBuffer> packageAudioFrame(SmartPtr<SmartBuffer> audioPacket, u32 ts);

 private:
    VideoStreamSetting videoSetting_;
    AudioStreamSetting audioSetting_;

    SmartPtr<SmartBuffer> videoHeader_;
};

#endif
