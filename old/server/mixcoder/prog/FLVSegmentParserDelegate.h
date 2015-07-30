#ifndef __FLVSEGMENTPARSERDELEGATE_H__
#define __FLVSEGMENTPARSERDELEGATE_H__

#include "fwk/SmartBuffer.h"
#include "CodecInfo.h"
#include "AccessUnit.h"

class FLVSegmentParserDelegate
{
 public:
    virtual u32 getGlobalAudioTimestamp() = 0;
    virtual void onFLVFrameParsed( SmartPtr<AccessUnit> au, int index ) = 0;
};
#endif
