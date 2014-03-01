
#ifndef __MEDIATARGET_H__
#define __MEDIATARGET_H__

#include "fwk/SmartBuffer.h"
#include "fwk/SmartPtr.h"

namespace mixcodec
{

enum StreamType {
    unknownST = 0,
    VideoST,
    AudioST,
    PrivateST
};

enum CodecType {
    unknownCT = 0,
    h264CT,
    aacCT,
    mp3CT,
};

const u64 INVALID_TS = 0xFFFFFFFFFFFFFFFF;

struct AccessUnit : SmartPtrInterface<AccessUnit> {
    u64 pts, dts;
    SmartPtr<Buffer> payload;
    StreamType st;
    CodecType  ct;
    bool isKey;
};

class MediaTarget {
 public:
    virtual void newAU( SmartPtr<AccessUnit> ) {};

    /* Useful for non-broken containers like MP2TS,
       not so useful for ASF */
    virtual void newSystemClock( u64 ) {};

    virtual void newSPSPPS( SmartPtr<Buffer> ) {}
    virtual void newAudioHeader( SmartPtr<Buffer> ) {}
    //virtual void flush() {};
};

};
#endif //__MEDIATARGET_H__
