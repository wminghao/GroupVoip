#ifndef __FLVOUTPUT_H__
#define __FLVOUTPUT_H__

#include "SmartBuffer.h"
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
    
    void newHeader(SmartPtr<SmartBuffer> videoHeader, SmartPtr<SmartBuffer> audioHeader);
    
    SmartPtr<SmartBuffer> packageVideoFrame(SmartPtr<SmartBuffer> videoPacket, u64 ts);
    SmartPtr<SmartBuffer> packageAudioFrame(SmartPtr<SmartBuffer> audioPacket, u64 ts);

 private:
    VideoStreamSetting videoSetting_;
    AudioStreamSetting audioSetting_;
};

#endif
