#include "FLVSegmentParser.h"

const size_t SEGMENT_HEADER_LEN = sizeof(u8)+sizeof(u16);
const size_t STREAM_HEADER_LEN = sizeof(u16)+sizeof(u32);

u32 FLVSegmentParser::readData(SmartPtr<SmartBuffer> input)
{
    int totalRead = 0;
    if ( input->dataLength() >= SEGMENT_HEADER_LEN ) {
        u8* data = input->data();
        memcpy(&streamType_, data, sizeof(u8));
        data+=sizeof(u8);
        memcpy(&numStreams_, data, sizeof(u16)); 
        data+=sizeof(u16);
        
        totalRead += SEGMENT_HEADER_LEN;
        for( int i = 0; i < numStreams_; i++ ) {
            if ( input->dataLengt() >= (totalRead+STREAM_HEADER_LEN)  ) {
                u16 streamId = 0;
                memcpy(&streamId_, data, sizeof(u16)); 
                data+=sizeof(u16);
                
                u32 curStreamLen = 0;
                memcpy(&curStreamLen, data, sizeof(u32)); 
                data+=sizeof(u32);
                
                totalRead += STREAM_HEADER_LEN;
                
                if ( input->dataLengt() >= (totalRead+curStreamLen)  ) {
                    SmartPtr<SmartBuffer> curStream = new SmartBuffer( curStreamLen, data);
                    parser_[i].readData(curStream);
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

SmartPtr<SmartBuffer> FLVSegmentParser::getNextFLVFrame(int index)
{
    assert ( index < numStreams_ );
    return parser_[i].getNextFLVFrame();
}
