#ifndef __FLVOUTPUT_H__
#define __FLVOUTPUT_H__

#include "fwk/SmartBuffer.h"
#include "CodecInfo.h"

class FLVOutput
{
 public:
    //vp8 video + mp3 audio(16kHz)/speex(16khz)
 FLVOutput(VideoStreamSetting* videoSetting, AudioStreamSetting* audioSetting):flvHeaderSent_(false)
        {
            memcpy(&videoSetting_, videoSetting, sizeof(VideoStreamSetting));
            memcpy(&audioSetting_, audioSetting, sizeof(AudioStreamSetting));
        }
    SmartPtr<SmartBuffer> newHeader();

    //streamId and totalStream tells the flv format where is the video located inside the video output
    SmartPtr<SmartBuffer> packageVideoFrame(SmartPtr<SmartBuffer> videoPacket, u32 ts, bool bIsKeyFrame, VideoRect* videoRect);
    SmartPtr<SmartBuffer> packageAudioFrame(SmartPtr<SmartBuffer> audioPacket, u32 ts);

 private:
    VideoStreamSetting videoSetting_;
    AudioStreamSetting audioSetting_;

    bool flvHeaderSent_;
};

#endif
