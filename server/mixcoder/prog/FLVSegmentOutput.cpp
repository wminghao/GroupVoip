#include "FLVSegmentOutput.h"
#include "FLVOutput.h"
#include <stdio.h>

FLVSegmentOutput::FLVSegmentOutput(VideoStreamSetting* videoSetting, AudioStreamSetting* audioSetting) {
    for( u32 i = 0; i < MAX_XCODING_INSTANCES+1; i++ ) {
        output_[i] = new FLVOutput(videoSetting, audioSetting);
        outputBuffer_[i] = NULL;
    }
}
    
FLVSegmentOutput::~FLVSegmentOutput() {
    for( u32 i = 0; i < MAX_XCODING_INSTANCES+1; i++ ) {
        delete(output_[i]);
    }
}

bool FLVSegmentOutput::packageVideoFrame(SmartPtr<SmartBuffer> videoPacket, u32 ts, bool bIsKeyFrame, int streamId, VideoRect* videoRect)
{
    outputBuffer_[streamId] = output_[streamId]->packageVideoFrame(videoPacket, ts, bIsKeyFrame, videoRect);
    return true;
}
bool FLVSegmentOutput::packageAudioFrame(SmartPtr<SmartBuffer> audioPacket, u32 ts, int streamId)
{
    outputBuffer_[streamId] = output_[streamId]->packageAudioFrame(audioPacket, ts);
    return true;
}
SmartPtr<SmartBuffer> FLVSegmentOutput::getOneFrameForAllStreams()
{
    u32 streamMask = 0;
    u32 totalLen = 0;
    int totalStreams = 0;
    for( u32 i = 0; i < MAX_XCODING_INSTANCES+1; i++ ) {
        if( outputBuffer_[i] && outputBuffer_[i]->dataLength() ) {
            totalLen += outputBuffer_[i]->dataLength();
            totalStreams++;
            //streamMask excluding the all-in stream
            if( i != MAX_XCODING_INSTANCES ) {
                u32 val = 0x1<<i;
                streamMask |= val;
            }
        }
    }
    SmartPtr<SmartBuffer> result;
    if( totalLen > 0 ) {
        fprintf(stderr, "------totalStreams=%d, streamMask=0x%x\r\n", totalStreams, streamMask);
    
        //Headers
        //  Meta data = 3 bytes //starting with SGO //segment output
        //  StreamMask = 4 byte //max of 32 output streams
        // Content * (NoOfStreams+1): //MAX_ID is the all-mixed stream, 1-NoOfStreams are for each mobile stream
        //  streamId = 1 byte
        //  LengthOfStream = 4 bytes
        //  StreamData = n bytes
        result = new SmartBuffer(7 + 5*totalStreams + totalLen);
        u8* data = result->data();
        data[0] = 'S';
        data[1] = 'G';
        data[2] = 'O';
        memcpy(data+3, &streamMask, sizeof(streamMask));
        int offset = 7;
        for( u32 i = 0; i < MAX_XCODING_INSTANCES+1; i++ ) {
            if( outputBuffer_[i] && outputBuffer_[i]->dataLength() ) {
                u8 streamIdByte = i;
                memcpy(data+offset, &streamIdByte, sizeof(u8));
                offset += sizeof(u8);
                int len = outputBuffer_[i]->dataLength();
                memcpy(data+offset, &len, sizeof(u32));
                offset += sizeof(u32);
                memcpy(data+offset, outputBuffer_[i]->data(), outputBuffer_[i]->dataLength());
                offset += outputBuffer_[i]->dataLength();
                outputBuffer_[i] = NULL; //reset outputbuffer
            }
        }
    }
    return result;
}
