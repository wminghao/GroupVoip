#ifndef __FLVOUTPUT_H__
#define __FLVOUTPUT_H__

#include "fwk/SmartBuffer.h"
#include "CodecInfo.h"

class FLVOutput
{
 public:
    //vp8 video + mp3 audio
 FLVOutput(VideoStreamSetting* videoSetting, AudioStreamSetting* audioSetting):isFlvHeaderSent_(false)
        {
            memcpy(&videoSetting_, videoSetting, sizeof(VideoStreamSetting));
            memcpy(&audioSetting_, audioSetting, sizeof(AudioStreamSetting));
        }
    
    SmartPtr<SmartBuffer> newHeader();
    
    //streamId and totalStream tells the flv format where is the video located inside the video output
    SmartPtr<SmartBuffer> packageVideoFrame(SmartPtr<SmartBuffer> videoPacket, u32 ts, bool bIsKeyFrame, int streamId, int totalStreams);
    SmartPtr<SmartBuffer> packageAudioFrame(SmartPtr<SmartBuffer> audioPacket, u32 ts);

 private:
    VideoStreamSetting videoSetting_;
    AudioStreamSetting audioSetting_;

    bool isFlvHeaderSent_;
};

#endif
