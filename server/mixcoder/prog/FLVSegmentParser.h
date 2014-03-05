#ifndef __FLVSEGMENTPARSER_H
#define __FLVSEGMENTPARSER_H

#include "SmartBuffer.h"
#include "FLVParser.h"
#include "CodecInfo.h"

///////////////////////////////////
//Segment format as follows
// Header:
//  TotalLength = 4 bytes 
//  StreamType  = 1 byte
//  NoOfStreams = 2 byte
// Content * NoOfStreams:
//  streamId = 2 byte
//  LengthOfStream = 4 bytes
//  StreamData = n bytes
///////////////////////////////////
class FLVSegmentParser
{
 public:
 FLVSegmentParser(): streamType_(0), numStreams_(0) {}
    
    u32 readData(SmartPtr<SmartBuffer> input);
    
    int getNumOfStreams(StreamType& streamType;) { streamType = streamType_; return numStreams_; }
    SmartPtr<SmartBuffer> getNextFLVFrame(int index);
    
 private:
    FLVParser parser_[MAX_XCODING_INSTANCES];
    StreamType streamType_;
    u16 numStreams_;
};
#endif
