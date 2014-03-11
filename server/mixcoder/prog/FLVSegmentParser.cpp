#include "FLVSegmentParser.h"
#include <assert.h>

const size_t SEGMENT_HEADER_LEN = sizeof(u32);
const size_t STREAM_HEADER_LEN = sizeof(u8)+sizeof(u32);

bool FLVSegmentParser::isNextStreamAvailable(StreamType streamType)
{
    //TODO
    //algorithm here to detect whether it's avaiable
    return true;
}

bool FLVSegmentParser::isStreamOnlineStarted(StreamType streamType, int index)
{
    if( streamType == kVideoStreamType ) {
        return ( videoStreamStatus_[index] == kStreamOnlineStarted );
    } else if( streamType == kAudioStreamType ) {
        return ( audioStreamStatus_[index] == kStreamOnlineStarted );
    } else {
        return false;
    }
}

u32 count_bits(u32 n) {     
    unsigned int c; // c accumulates the total bits set in v
    for (c = 0; n; c++) { 
        n &= n - 1; // clear the least significant bit set
    }
    return c;
}

void FLVSegmentParser::onFLVFrameParsed( SmartPtr<AccessUnit> au, int index )
{
    if( au->st == kVideoStreamType ) {
        videoQueue_[index].push( au );
    } else if ( au->st == kAudioStreamType ) {
        audioQueue_[index].push( au );
    } else {
        //do nothing
    }
}

//TODO parse partial data
u32 FLVSegmentParser::readData(SmartPtr<SmartBuffer> input)
{
    int totalRead = 0;
    if ( input->dataLength() >= SEGMENT_HEADER_LEN ) {
        u8* data = input->data();
        u32 streamMask;
        memcpy(&streamMask, data, sizeof(u32));
        //handle mask here
        numStreams_ = count_bits(streamMask);

        //TODO take care of mask here
 
        data+=sizeof(u32);
        totalRead += SEGMENT_HEADER_LEN;

        for( int i = 0; i < numStreams_; i++ ) {
            if ( input->dataLength() >= (totalRead+STREAM_HEADER_LEN)  ) {
                u8 streamId = 0;
                memcpy(&streamId, data, sizeof(u8)); 
                data+=sizeof(u8);
                
                u32 curStreamLen = 0;
                memcpy(&curStreamLen, data, sizeof(u32)); 
                data+=sizeof(u32);
                
                totalRead += STREAM_HEADER_LEN;
                
                if ( input->dataLength() >= (totalRead+curStreamLen)  ) {
                    SmartPtr<SmartBuffer> curStream = new SmartBuffer( curStreamLen, data);
                    parser_[i]->readData(curStream);
                    totalRead += curStreamLen;
                } else {
                    assert(0);
                }
            } else {
                assert(0);
            }
        }
    }
    return totalRead;
}

SmartPtr<AccessUnit> FLVSegmentParser::getNextFLVFrame(int index, StreamType streamType)
{
    assert ( index < numStreams_ );
    SmartPtr<AccessUnit> au;
    if ( streamType == kVideoStreamType ) {
        au = videoQueue_[index].front();
        videoQueue_[index].pop();
    } else if( streamType == kAudioStreamType ){
        au = audioQueue_[index].front();
        audioQueue_[index].pop();
    }
    return au;
}
