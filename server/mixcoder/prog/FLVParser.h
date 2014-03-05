
#ifndef __FLVPARSER_H__
#define __FLVPARSER_H__

#include "SmartBuffer.h"
#include "CodecInfo.h"
#include <string>

class FLVParser
{
 public:
    FLVParser() {}
    //each read must be at least 1 frame
    void readData(SmartPtr<SmartBuffer> input); 
    SmartPtr<AccessUnit> getNextFLVFrame();
 private:
    std::string curBuf_;
}

#endif
