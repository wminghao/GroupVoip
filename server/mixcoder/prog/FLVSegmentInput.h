#ifndef __FLV_SEGMENT_INPUT__
#define __FLV_SEGMENT_INPUT__

#include "FLVSegmentParser.h"
#include "VideoDecoder.h"
#include "AudioDecoder.h"

class FLVSegmentInput
{
 public:
    FLVSegmentInput() {
        flvSegParser_ = new FLVSegmentParser( 30 ); //end result 30 fps
        for( u32 i = 0; i < MAX_XCODING_INSTANCES; i++ ) {
            audioDecoder_[i] = NULL; //initialize it later
            videoDecoder_[i] = new VideoDecoder(i);
        }
    }
    ~FLVSegmentInput() {
        delete flvSegParser_;
        for( u32 i = 0; i < MAX_XCODING_INSTANCES; i ++ ) {
            delete audioDecoder_[i];
            delete videoDecoder_[i];
        }
    }
    
    StreamSource getStreamSource(int streamId) { return flvSegParser_->getStreamSource(streamId); }

    bool readData(SmartPtr<SmartBuffer> input) { return flvSegParser_->readData(input); }

    //check the status of a stream to see if it's online
    bool isStreamOnlineStarted(StreamType streamType, int index) { return flvSegParser_->isStreamOnlineStarted(streamType, index); }

    //detect whether the next video stream is ready or not
    bool isNextRawVideoStreamReady(u32& videoTimestamp, u32 audioTimestamp) { return flvSegParser_->isNextVideoStreamReady(videoTimestamp, audioTimestamp); }

    //detect whether the next audio stream is ready or not
    bool isNextRawAudioStreamReady(u32& audioTimestamp);

    //get next flv frame
    SmartPtr<AccessUnit> getNextRawAudioFrame(u32 index); //return at most 1 frame
    SmartPtr<AccessUnit> getNextRawVideoFrame(u32 index, u32 timestamp); // can return more than 1 frames
    
 private:
    //segment parser
    FLVSegmentParser* flvSegParser_;
    //decoders
    AudioDecoder* audioDecoder_[ MAX_XCODING_INSTANCES ];
    VideoDecoder* videoDecoder_[ MAX_XCODING_INSTANCES ];

    //queue of raw audios and raw videos
    queue< SmartPtr<AccessUnit> > rawAudioQueue_[MAX_XCODING_INSTANCES];
    queue< SmartPtr<AccessUnit> > rawVideoQueue_[MAX_XCODING_INSTANCES];
};
#endif
