
#ifndef __FLVPARSER_H__
#define __FLVPARSER_H__

#include "fwk/SmartBuffer.h"
#include "fwk/Time.h"
#include "fwk/Units.h"
#include "CodecInfo.h"
#include "FLVSegmentParserDelegate.h"
#include <string>

using namespace std;
class FLVParser
{
 public:
 FLVParser(FLVSegmentParserDelegate* delegate, int index):delegate_(delegate), index_(index), scanState_ (SCAN_HEADER_TYPE_LEN), curFlvTagSize_(0), curStreamType_(kUnknownStreamType), relTimeStampOffset_(MAX_U32)
    {
        startEpocTime_ = getEpocTime();
    }
    //each read must be at least 1 frame
    void readData(SmartPtr<SmartBuffer> input); 
 private:
    void parseNextFLVFrame(string & flvTag);
 private:
    //each flv tag is 11 bytes + dataSize + 4 bytes of previous tag size
    enum SCAN_STATE {
        SCAN_HEADER_TYPE_LEN,
        SCAN_REMAINING_TAG,
    };
    FLVSegmentParserDelegate* delegate_;
    int index_;
    SCAN_STATE scanState_;
    string curBuf_;
    u32 curFlvTagSize_;
    StreamType curStreamType_;
    
    //calculate the relative time of 1st frame
    u64 startEpocTime_;
    u32 relTimeStampOffset_;
};

#endif
