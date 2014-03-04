#ifndef __FLVSEGMENTPARSER_H
#define __FLVSEGMENTPARSER_H

#include "SmartBuffer.h"
#include <queue>
#include "FLVParser.h"
#include "CodecInfo.h"

class FLVSegmentParser
{
 public:
    FLVSegmentParser() {}
    u32 readData(SmartPtr<SmartBuffer> input);
    
    int getNumOfStreams() { return numStreams_; }
    SmartPtr<SmartBuffer> getNextFLVFrame(int index);

 private:
    FLVParser parser_[MAX_XCODING_INSTANCES];
    int numStreams_;
}




#endif
