#ifndef __AUDIOMIXER_H__
#define __AUDIOMIXER_H__

extern "C" {
#include <libavcodec/avcodec.h>    // required headers
#include <libavformat/avformat.h>
}

#include "fwk/SmartBuffer.h"
#include "MediaTarget.h"
#include "CodecInfo.h"
#include "RawData.h"

//speex 16khz mono is used
class AudioMixer
{
 public:
    AudioMixer() {}
    //do the mixing, for now, always mix n speex streams into 1 speex stream
    SmartPtr<SmartBuffer> mixStreams(SmartPtr<AudioRawData>* rawData,
                                     int sampleSize,
                                     int totalStreams,
                                     u32 excludeStreamId);

 private:
    void mixTwoStreams(SmartPtr<AudioRawData>* rawData, 
                       int* twoIndex,
                       short* valShort,
                       int sampleSize);

    void mixThreeStreams(SmartPtr<AudioRawData>* rawData,
                       int* threeIndex,
                       short* valShort,
                       int sampleSize);

    void mixFourStreams(SmartPtr<AudioRawData>* rawData,
                       int* fourIndex,
                       short* valShort,
                       int sampleSize);

    void findIndexes(SmartPtr<AudioRawData>* rawData,
                     u32 excludeStreamId,
                     int* indexArr);

};
#endif
