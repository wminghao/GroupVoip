
#ifndef __MEDIATARGET_H__
#define __MEDIATARGET_H__

#include "fwk/SmartPtrInterface.h"
#include "fwk/SmartBuffer.h"
#include "fwk/SmartPtr.h"
#include "CodecInfo.h"

const u64 INVALID_TS = 0xFFFFFFFFFFFFFFFF;

class AccessUnit : public SmartPtrInterface<AccessUnit> {
 public:
    u32 pts, dts;
    SmartPtr<SmartBuffer> payload;
    StreamType st;
    int  ct; //codecType, either audio or video
    int  sp; //special properties, for avc, it's either sps/pps or nalu, for aac, it's sequenceheader or nalu, for other codecs, it's raw data
    bool isKey;
};

class MediaTarget {
 private:
    virtual SmartPtr<SmartBuffer> newAccessUnit( SmartPtr<AccessUnit> ) = 0;

    //virtual void newAVCSeqHeader( SmartPtr<AccessUnit> ) {}
    //virtual void newAudioHeader( SmartPtr<AccessUnit> ) {}
    //virtual void flush() {};
};

#endif //__MEDIATARGET_H__
