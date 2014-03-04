#ifndef __FLVOUTPUT_H__
#define __FLVOUTPUT_H__

#include "SmartBuffer.h"
#include "CodecInfo.h"

class FLVOutput
{
 public:
    //vp8 video + mp3 audio
    FLVOutput(AudioCodecId acid, VideoCodecId vcid, VideoStreamSetting videoSetting, AudioStreamSetting audioSetting);
    
    newHeader(SmartPtr<SmartBuffer> videoHeader, SmartPtr<SmartBuffer> audioHeader);
};

#endif
