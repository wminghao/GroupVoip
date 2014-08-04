#ifndef __ACCESSUNIT_H__
#define __ACCESSUNIT_H__

#include "fwk/SmartPtrInterface.h"
#include "fwk/SmartBuffer.h"
#include "fwk/SmartPtr.h"
#include "CodecInfo.h"

const u64 INVALID_TS = 0xFFFFFFFFFFFFFFFF;

class AccessUnit : public SmartPtrInterface<AccessUnit> {
 public:
    u32 pts;
    SmartPtr<SmartBuffer> payload;
    StreamType st; //audio/video
    int  ctype; //codecType and other info, either audio or video
    int  sp; //special properties, for avc, it's either sps/pps or nalu, for aac, it's sequenceheader or nalu, for other codecs, it's raw data

    bool isKey;
};

#endif //__ACCESSUNIT_H__