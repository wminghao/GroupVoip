 #ifndef __FLVSEGMENTPARSER_H
#define __FLVSEGMENTPARSER_H

#include "MediaTarget.h"
#include "fwk/SmartBuffer.h"
#include "FLVParser.h"
#include "CodecInfo.h"
#include "FLVSegmentParserDelegate.h"
#include <queue>

using namespace std;

///////////////////////////////////
//Segment format as follows
// Header:
//  Meta data = 3 bytes //starting with SEG
//  StreamMask = 4 byte //max of 32 streams
// Content * NoOfStreams:
//  streamId = 1 byte
//  LengthOfStream = 4 bytes
//  StreamData = n bytes
///////////////////////////////////

///////////////////////////////////
//FLVSegmentParser does the following:
// 1) input tells it how many streams available.
// 2) it parses data and put audio/video inside a queue
// 3) each queue is sorted by timestamp
//    The top data in the queue,
//    for audio, there is no frame drop, (continuous) if all encoders are speex 16khz, 2 bytes/sample, mono, frameLen=160samples/frame (timestamp diff is no bigger than 10 ms)
//        In the beginning, if a queue is never used, ignore that queue. sync with the others.
//        After the 1st time a queue is used, all k audio data are available, always waitif all k top data is available, pop out immediately.
//    for video, there could be frame drop, if the TARGET framerate is 30 fps, (timestamp diff is no bigger than 33.33 ms)
//        Every 33.33ms, video data pops out as well, whether there is data or not in the queue, 
//        if there is no data, mixer will reuse the previous frame to mix it, if there is no previous frame(in the beginning), it will fill with blank.
//    TODO: Will the timestamp be adjusted???
///////////////////////////////////

class FLVSegmentParser:public FLVSegmentParserDelegate
{
 public:
    FLVSegmentParser(u32 targetVideoFrameRate): numStreams_(0), targetVideoFrameRate_(targetVideoFrameRate) {
        memset(audioStreamStatus_, 0, sizeof(StreamStatus)*MAX_XCODING_INSTANCES);
        memset(videoStreamStatus_, 0, sizeof(StreamStatus)*MAX_XCODING_INSTANCES);
        for(int i = 0; i < MAX_XCODING_INSTANCES; i++) {
            parser_[i] = new FLVParser(this, i);
        }
    }
    ~FLVSegmentParser() {
        for(int i = 0; i < MAX_XCODING_INSTANCES; i++) {
            delete parser_[i];
        }
    }

    u32 readData(SmartPtr<SmartBuffer> input);

    //detect whether the next stream is available or not 
    bool isNextStreamAvailable(StreamType streamType);
    //check the status of a stream to see if it's online
    bool isStreamOnlineStarted(StreamType streamType, int index);
    //get next flv frame
    SmartPtr<AccessUnit> getNextFLVFrame(int index, StreamType streamType);

 private:
    virtual void onFLVFrameParsed( SmartPtr<AccessUnit> au, int index );
    
 private:
    typedef enum StreamStatus
    {
        kStreamOffline,
        kStreamOnlineNotStarted,
        kStreamOnlineStarted
    } StreamStatus;
    
    typedef enum FLVSegmentParsingState
    {
        SEARCHING_SEGHEADER,
        SEARCHING_STREAM_MASK,
        SEARCHING_STREAM_HEADER,
        SEARCHING_STREAM_DATA
    }FLVSegmentParsingState;
    
    queue< SmartPtr<AccessUnit> > audioQueue_[MAX_XCODING_INSTANCES];
    StreamStatus audioStreamStatus_[MAX_XCODING_INSTANCES]; //tells whether a queue has benn used or not
    queue< SmartPtr<AccessUnit> > videoQueue_[MAX_XCODING_INSTANCES];
    StreamStatus videoStreamStatus_[MAX_XCODING_INSTANCES]; //tells whether a queue has been used or not

    FLVParser* parser_[MAX_XCODING_INSTANCES];
    u16 numStreams_;
    u32 targetVideoFrameRate_;
};
#endif
