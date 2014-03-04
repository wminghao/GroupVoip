
#ifndef __MEDIATARGET_H__
#define __MEDIATARGET_H__

#include "fwk/SmartBuffer.h"
#include "fwk/SmartPtr.h"
#include "CodecInfo.h"

namespace mixcodec
{

enum StreamType {
    unknownST = 0,
    VideoST,
    AudioST,
    PrivateST
};

const u64 INVALID_TS = 0xFFFFFFFFFFFFFFFF;

struct AccessUnit : SmartPtrInterface<AccessUnit> {
    u64 pts, dts;
    SmartPtr<Buffer> payload;
    StreamType st;
    int  ct; //codecType, either audio or video
    bool isKey;
};

class MediaTarget {
 private:
    virtual void newAccessUnit( SmartPtr<AccessUnit> ) {};

    virtual void newAVCSeqHeader( SmartPtr<SmartBuffer> ) {}
    virtual void newAudioHeader( SmartPtr<SmartBuffer> ) {}
    //virtual void flush() {};
};

};
#endif //__MEDIATARGET_H__
