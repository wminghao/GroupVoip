
#ifndef __FLVPARSER_H__
#define __FLVPARSER_H__

#include "fwk/SmartBuffer.h"
#include "CodecInfo.h"
#include "FLVSegmentParserDelegate.h"
#include <string>

using namespace std;
class FLVParser
{
 public:
    FLVParser(FLVSegmentParserDelegate* delegate, int index):delegate_(delegate), index_(index) {}
    //each read must be at least 1 frame
    void readData(SmartPtr<SmartBuffer> input); 
 private:
    SmartPtr<AccessUnit> getNextFLVFrame();
 private:
    string curBuffer_;
    FLVSegmentParserDelegate* delegate_;
    int index_;
};

#endif
