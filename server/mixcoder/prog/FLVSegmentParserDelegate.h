#ifndef __FLVSEGMENTPARSERDELEGATE_H__
#define __FLVSEGMENTPARSERDELEGATE_H__

#include "fwk/SmartBuffer.h"
#include "CodecInfo.h"
#include "MediaTarget.h"

class FLVSegmentParserDelegate
{
 public:
    virtual void onFLVFrameParsed( SmartPtr<AccessUnit> au, int index ) = 0;
};
#endif
