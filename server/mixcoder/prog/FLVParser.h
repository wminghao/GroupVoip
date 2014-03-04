
#ifndef __FLVPARSER_H__
#define __FLVPARSER_H__

#include "SmartBuffer.h"
#include "CodecInfo.h"

class FLVParser
{
 public:
    FLVParser() {}
    u32 readData(SmartPtr<SmartBuffer> input);
    SmartPtr<SmartBuffer> getNextFLVFrame();
 private:
    u8* remainingData_;
    u32 remainingDataLen_;
}

#endif
