#include "flvrealtimeparser.h"
#include <stdio.h>
#include <assert.h>

//parsing the raw data to get a complete FLV frame
u8* FLVRealTimeParser::readData(FILE* fd, int* len) {
    u32 offset = 0;

    SCAN_STATE scanState = SCAN_HEADER_TYPE_LEN;
    string curBuf;
    u32 curFlvTagSize = 0;

    while( true ) {
        switch( scanState ) {
        case SCAN_HEADER_TYPE_LEN:
            {
                //read 4 bytes
                fread(buffer_+offset, 1, 4, fd);
                curBuf = string((const char*)buffer_+offset, 4);
                offset += 4;

                if ( curBuf.size() == 4 ) {
                    std::string tempStr = curBuf.substr(1, 3);
                    union DataSizeUnion{
                        u32 dataSize;
                        u8 dataSizeStr[4];
                    }dsUnion;
                    dsUnion.dataSizeStr[0] = tempStr[2];
                    dsUnion.dataSizeStr[1] = tempStr[1];
                    dsUnion.dataSizeStr[2] = tempStr[0];
                    dsUnion.dataSizeStr[3] = 0;
                    curFlvTagSize = dsUnion.dataSize;
                    curFlvTagSize += 7+4; //add remaining of the header + previousTagLen
                    curBuf.clear();
                    scanState = SCAN_REMAINING_TAG;
                } else {
                    //EOF
                    fprintf(stderr, "===EOF===\r\n");
                    return NULL;
                }
                break;
            }
        case SCAN_REMAINING_TAG:
            {                
                //read the current tag
                fread(buffer_+offset, 1, curFlvTagSize, fd);
                curBuf = string((const char*)buffer_+offset, curFlvTagSize);
                offset += curFlvTagSize;

                if ( curBuf.size() >= curFlvTagSize ) {
                    //read 4 bytes of ts
                    union TimestampUnion{
                        u32 timestamp;
                        u8 timestampStr[4];
                    }tsUnion;
                    tsUnion.timestampStr[0] = curBuf[2];
                    tsUnion.timestampStr[1] = curBuf[1];
                    tsUnion.timestampStr[2] = curBuf[0];
                    tsUnion.timestampStr[3] = curBuf[3];
                    u32 curTimestamp = tsUnion.timestamp;

                    curBuf.clear();
                    curFlvTagSize  = 0; //reset and go to the first state
                    scanState = SCAN_HEADER_TYPE_LEN;

                    //if current timestamp is more than 20ms of previous read timestamp, return here
                    if( curTimestamp >= lastReadTimestamp_+20) {
                        //fprintf(stderr, "===Read %d bytes, curTimestamp=%d, lastReadTimestamp_=%d===\r\n", offset, curTimestamp, lastReadTimestamp_);
                        lastReadTimestamp_ = curTimestamp;
                        *len = offset;
                        return buffer_;
                    }
                } else {
                    //EOF
                    fprintf(stderr, "===EOF===\r\n");
                    return NULL;
                }
                break;
            }
        }
    }
}
