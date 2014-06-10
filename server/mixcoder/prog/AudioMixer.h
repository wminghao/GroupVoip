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

 private:
    void mixTwoStreams(SmartPtr<SmartBuffer> buffer[], 
                       int* twoIndex,
                       short* valShort,
                       int sampleSize);

    void mixThreeStreams(SmartPtr<SmartBuffer> buffer[], 
                       int* threeIndex,
                       short* valShort,
                       int sampleSize);

    void mixFourStreams(SmartPtr<SmartBuffer> buffer[], 
                       int* fourIndex,
                       short* valShort,
                       int sampleSize);

    void findIndexes(AudioStreamSetting settings[],
                     u32 excludeStreamId,
                     int* indexArr);

};


#endif
