#ifndef __RAWDATA_H__
#define __RAWDATA_H__

#include "CodecInfo.h"
#include "fwk/SmartBuffer.h"

class VideoRawData;
class VideoRawData: public SmartPtrInterface<VideoRawData> {
 public:
    SmartPtr<SmartBuffer> rawVideoPlanes_[3];
    int rawVideoStrides_[3];
    VideoStreamSetting rawVideoSettings_;
    u32 pts; //pts == dts
    int sp; //special properties, for avc, it's either sps/pps or nalu, for aac, it's sequenceheader or nalu, for other codecs, it's raw data
};

class AudioRawData;
class AudioRawData: public SmartPtrInterface<AudioRawData>{
 public:
    SmartPtr<SmartBuffer> rawAudioFrame_;
    bool bIsStereo; //if it's two channels or single channel, used in mixing
    bool bIsValid;
    StreamSource ss; //special flag, mobile or desktop stream
    u32 pts; //pts == dts
};
#endif //__RAWDATA_H__
