#ifndef __FLVREALTIMEPARSER_H__
#define __FLVREALTIMEPARSER_H__

#include "../prog/fwk/Units.h"
#include <string>
#include <stdio.h>

#define MAX_BUFFER_SIZE 500000 //500k per 20ms, big enough

//Used to simulate the case where video streams comes in a consistent rate.
//FLV realtime parser, reads 20ms of data every time readData() is called.
using namespace std;
class FLVRealTimeParser
{
 public:
 FLVRealTimeParser():lastReadTimestamp_(0)
    {
    }
    //each read must be at least 1 frame
    u8* readData(FILE* fd, int* len);
 private:
    //each flv tag is 11 bytes + dataSize + 4 bytes of previous tag size
    enum SCAN_STATE {
        SCAN_HEADER_TYPE_LEN,
        SCAN_REMAINING_TAG,
    };

    //result buffer of 20ms of data
    u8 buffer_[MAX_BUFFER_SIZE];
    u32 lastReadTimestamp_;
};

#endif
