
#ifndef __MEDIATARGET_H__
#define __MEDIATARGET_H__

#include "fwk/SmartBuffer.h"
#include "fwk/SmartPtr.h"
#include "CodecInfo.h"

namespace mixcodec
{

const u64 INVALID_TS = 0xFFFFFFFFFFFFFFFF;

class AccessUnit : SmartPtrInterface<AccessUnit> {
 public:
    u64 pts, dts;
    SmartPtr<Buffer> payload;
    StreamType st;
    int  ct; //codecType, either audio or video
    int  sp; //special properties, for avc, it's either sps/pps or nalu, for aac, it's sequenceheader or nalu, for other codecs, it's raw data
    bool isKey;
};

class MediaTarget {
 private:
    virtual void newAccessUnit( SmartPtr<AccessUnit> ) {};

    virtual void newAVCSeqHeader( SmartPtr<AccessUnit> ) {}
    virtual void newAudioHeader( SmartPtr<AccessUnit> ) {}
    //virtual void flush() {};
};

};
#endif //__MEDIATARGET_H__
