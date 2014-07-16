 #ifndef __FLVSEGMENTPARSER_H
#define __FLVSEGMENTPARSER_H

#include "MediaTarget.h"
#include "fwk/SmartBuffer.h"
#include "fwk/Time.h"
#include "FLVParser.h"
#include "CodecInfo.h"
#include "FLVSegmentParserDelegate.h"
#include "RawData.h"
#include "VideoDecoder.h"
#include "AudioDecoder.h"
#include <queue>

using namespace std;

///////////////////////////////////
//Segment format as follows
// Header:
//  Meta data = 3 bytes //starting with SGI
//  LayoutMode = 1 byte //Even or Main
//  StreamMask = 4 byte //max of 32 streams
// Content * NoOfStreams:
//  streamId = 5 bits
//  streamSource = 3 bits //desktop or mobile
//  special property = 1 byte //Main layout, whether it's main
//  LengthOfStream = 4 bytes
//  StreamData = n bytes
///////////////////////////////////

///////////////////////////////////
//FLVSegmentParser does the following:
// 1) input tells it how many streams available.
// 2) it parses data and put audio/video inside a queue
// 3) each queue is sorted by timestamp
//    The top data in the queue,
//    for audio, there is no frame drop, (continuous) if all encoders are speex 16khz, 2 bytes/sample, mono, frameLen=160samples/frame (timestamp diff is no bigger than 20 ms, 50fps)
//        In the beginning, if a queue is never used, ignore that queue. sync with the others.
//        After the 1st time a queue is used, all k audio data are available, always waitif all k top data is available, pop out immediately.
//    for video, there could be frame drop, if the TARGET framerate is 30 fps, (timestamp diff is no bigger than 33.33 ms)
//        Every 33.33ms, video data pops out as well, whether there is data or not in the queue, (it's possible a stream having more than 1 video data output)
//        if there is no data, mixer will reuse the previous frame to mix it, if there is no previous frame(in the beginning), it will fill with blank.
// Timestamp must be adjusted to be the same as the 1st stream's timestamp
///////////////////////////////////

class FLVSegmentParser:public FLVSegmentParserDelegate
{
 public:
 FLVSegmentParser(u32 targetVideoFrameRate, AudioStreamSetting* aRawSetting): parsingState_(SEARCHING_SEGHEADER),
        curSegTagSize_(0), curStreamId_(0), curStreamLen_(0), curStreamCnt_(0),
        numStreams_(0), targetVideoFrameRate_(targetVideoFrameRate), 
        hasStarted_(0), lastBucketTimestamp_(0), globalAudioTimestamp_(0)
        {
            memset(audioStreamStatus_, 0, sizeof(StreamStatus)*MAX_XCODING_INSTANCES);
            memset(videoStreamStatus_, 0, sizeof(StreamStatus)*MAX_XCODING_INSTANCES);
            for(u32 i = 0; i < MAX_XCODING_INSTANCES; i++) {
                parser_[i] = new FLVParser(this, i);
                audioDecoder_[i] = NULL; //initialize it later
                videoDecoder_[i] = new VideoDecoder(i);
            }
            memcpy( &rawAudioSettings_, aRawSetting, sizeof(AudioStreamSetting) );
        }
    ~FLVSegmentParser() {
        for(u32 i = 0; i < MAX_XCODING_INSTANCES; i++) {
            delete parser_[i];
            delete audioDecoder_[i];
            delete videoDecoder_[i];
        }
    }
    StreamSource getStreamSource(int streamId) { return streamSource[streamId]; }
    
    bool readData(SmartPtr<SmartBuffer> input);

    //detect whether the next stream is ready or not 
    bool isNextVideoStreamReady(u32& videoTimestamp, u32 audioTimestamp);
    bool isNextAudioStreamReady(u32& audioTimestamp);

    //check the status of a stream to see if it's online
    bool isStreamOnlineStarted(StreamType streamType, int index);

    bool hasFirstFrameDecoded(int i, bool bIsVideo) { return bIsVideo?videoDecoder_[i]->hasFirstFrameDecoded():audioDecoder_[i]->hasFirstFrameDecoded(); }

    //get next decoded frame
    SmartPtr<AudioRawData> getNextAudioFrame(u32 index); //return at most 1 frame
    SmartPtr<VideoRawData> getNextVideoFrame(u32 index, u32 timestamp); // can return more than 1 frames

 private:
    bool isNextVideoFrameSpsPps(u32 index, u32& timestamp);

    //inherited from delegate functions
    virtual void onFLVFrameParsed( SmartPtr<AccessUnit> au, int index );
    virtual u32 getGlobalAudioTimestamp() { return globalAudioTimestamp_;}
    
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

    FLVSegmentParsingState parsingState_;
    string curBuf_;
    u32 curSegTagSize_;
    u32 curStreamId_;
    u32 curStreamLen_;
    u32 curStreamCnt_;

    //stores raw audio and video messages
    queue<SmartPtr<AudioRawData> > audioQueue_[MAX_XCODING_INSTANCES];
    StreamStatus audioStreamStatus_[MAX_XCODING_INSTANCES]; //tells whether a queue has benn used or not
    queue<SmartPtr<VideoRawData> > videoQueue_[MAX_XCODING_INSTANCES];
    StreamStatus videoStreamStatus_[MAX_XCODING_INSTANCES]; //tells whether a queue has been used or not

    FLVParser* parser_[MAX_XCODING_INSTANCES];
    u32 numStreams_;
    u32 targetVideoFrameRate_;

    StreamSource streamSource[MAX_XCODING_INSTANCES];

    //video timestamp adjustment. output is always 30fps
    u32 hasStarted_;       //1st time video stream starts
    double lastBucketTimestamp_;      //1st video frame timestamp

    u32 globalAudioTimestamp_; //global audio timestamp used for avsync between different video streams

    //immediately decodes any video/audio messages once we receive any data
    AudioDecoder* audioDecoder_[ MAX_XCODING_INSTANCES ];
    VideoDecoder* videoDecoder_[ MAX_XCODING_INSTANCES ];

    //audio settings for decoder
    AudioStreamSetting rawAudioSettings_;

    //current audio timestamp, mapping
};
#endif
