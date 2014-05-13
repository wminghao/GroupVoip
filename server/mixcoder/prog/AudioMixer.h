#ifndef __AUDIOMIXER_H__
#define __AUDIOMIXER_H__

extern "C" {
#include <libavcodec/avcodec.h>    // required headers
#include <libavformat/avformat.h>
}

#include "fwk/SmartBuffer.h"
#include "MediaTarget.h"
#include "CodecInfo.h"

//speex 16khz mono is used
class AudioMixer
{
 public:
    AudioMixer() {}
    //do the mixing, for now, always mix n speex streams into 1 speex stream
    SmartPtr<SmartBuffer> mixStreams(SmartPtr<SmartBuffer> buffer[], 
                                     AudioStreamSetting settings[], 
                                     int sampleSize,
                                     int totalStreams,
                                     u32 excludeStreamId);

};


#endif
