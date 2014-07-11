#include "fwk/Units.h"
#include "CodecInfo.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string>
#include "fwk/log.h"

using namespace std;

u32 count_bits(u32 n) {     
    unsigned int c; // c accumulates the total bits set in v
    for (c = 0; n; c++) { 
        n &= n - 1; // clear the least significant bit set
    }
    return c;
}
typedef enum FLVSegmentParsingState
{
    SEARCHING_SEGHEADER,
    SEARCHING_STREAM_MASK,
    SEARCHING_STREAM_HEADER,
    SEARCHING_STREAM_DATA
}FLVSegmentParsingState;

class FileWriter
{
public:
    FileWriter() {
        fp_ = NULL;
    }
    ~FileWriter() {
        if(fp_) {
            fclose(fp_);
        }
    }
    void setFileName(string& fileName) {
        fileName_= fileName;
        
    }
    void writeData(const char* buffer, u32 dataLength) 
    {
        if(!fp_) {
            LOG( "----opening file %s\r\n", fileName_.c_str());
            fp_ = fopen(fileName_.c_str(), "wb");
        }
        assert(fp_);
        fwrite( buffer, 1, dataLength, fp_);
    }

private:
    FILE * fp_;
    string fileName_;
};

FileWriter writer_;
FLVSegmentParsingState parsingState_;
string curBuf_;
u32 curSegTagSize_;
u32 curStreamId_;
u32 curStreamLen_;
u32 curStreamCnt_;
u32 numStreams_;

bool readData(u8* data, u32 len)
{
    while( len ) {
        switch( parsingState_ ) {
        case SEARCHING_SEGHEADER:
            {
                if ( curBuf_.size() < 4 ) {
                    size_t cpLen = MIN(len, 4-curBuf_.size());
                    curBuf_ += string((const char*)data, cpLen); //concatenate the string                                                                                                                 
                    len -= cpLen;
                    data += cpLen;
                }

                if ( curBuf_.size() >= 4 ) {
                    assert(curBuf_[0] == 'S' && curBuf_[1] == 'G' && curBuf_[2] == 'I');
                    assert(curBuf_[3] == 0); //even layout for now
                    curBuf_.clear();
                    curSegTagSize_ = 0;
                    parsingState_ = SEARCHING_STREAM_MASK;
                }
                break;
            }
        case SEARCHING_STREAM_MASK:
            {
                if ( curBuf_.size() < 4 ) {
                    size_t cpLen = MIN(len, 4-curBuf_.size());
                    curBuf_ += string((const char*)data, cpLen); //concatenate the string                                                                                                          

                    len -= cpLen;
                    data += cpLen;
                }

                if ( curBuf_.size() >= 4 ) {
                    u32 streamMask;
                    memcpy(&streamMask, curBuf_.data(), sizeof(u32));
                    
                    //handle mask here 
                    numStreams_ = count_bits(streamMask);
                    printf( "---streamMask=0x%x\r\n", streamMask);
                    printf( "---numStreams_=%d\r\n", numStreams_);

                    int index = 0;
                    while( index < (int)MAX_XCODING_INSTANCES ) {
                        u32 value = ((streamMask<<31)>>31); //mask off all other bits
                        streamMask >>= 1; //shift 1 bit
                        index++;
                    }

                    curStreamCnt_ = numStreams_;
                    curBuf_.clear();
                    curSegTagSize_ = 0;
                    parsingState_ = SEARCHING_STREAM_HEADER;
                }
                break;
            }
        case SEARCHING_STREAM_HEADER:
            {
                if ( curBuf_.size() < 6 ) {
                    size_t cpLen = MIN(len, 6-curBuf_.size());
                    curBuf_ += string((const char*)data, cpLen); //concatenate the string

                    len -= cpLen;
                    data += cpLen;
                }
                if ( curBuf_.size() >= 6 ) {
                    curStreamId_ = (curBuf_[0]&0xf8)>>3; //first 5 bits
                    assert(curStreamId_ < (u32)MAX_XCODING_INSTANCES);

                    u32 curStreamSource = (curBuf_[0]&0x7); //last 3 bits
                    assert( curStreamSource < kTotalStreamSource);

                    assert(curBuf_[1] == 0x0); //ignore the special property

                    memcpy(&curStreamLen_, curBuf_.data()+2, 4); //read the len
                    printf( "---curStreamCnt_=%d, curBuf_[0]=0x%x, curStreamId_=%d curStreamSource=%d, curStreamLen_=%d\r\n", curStreamCnt_, curBuf_[0], curStreamId_, curStreamSource, curStreamLen_);

                    curBuf_.clear();
                    curSegTagSize_ = 0;
                    parsingState_ = SEARCHING_STREAM_DATA;
                }
                break;
            }
        case SEARCHING_STREAM_DATA:
            {
                bool bIsFinished = false;
                if ( curStreamLen_ ) {
                    if ( curBuf_.size() < curStreamLen_ ) {
                        size_t cpLen = MIN(len, curStreamLen_-curBuf_.size());
                        curBuf_ += string((const char*)data, cpLen); //concatenate the string     
                        
                        len -= cpLen;
                        data += cpLen;
                    }
                    if ( curBuf_.size() >= curStreamLen_ ) {
                        printf( "---curStreamId_=%d curStreamLen_=%d\r\n", curStreamId_, curStreamLen_);
                        //TODO only save the first video
                        if( curStreamCnt_ == numStreams_ ) {
                            writer_.writeData( curBuf_.data(), curStreamLen_ );
                        }
                        bIsFinished = true;
                    }
                } else {
                    //0 bytes means no data received for this channel even though it's active
                    bIsFinished = true;
                }
                if( bIsFinished ) {
                    curBuf_.clear();
                    curSegTagSize_ = 0;
                    curStreamCnt_--;
                    if ( curStreamCnt_ ) {
                        parsingState_ = SEARCHING_STREAM_HEADER;
                    } else {
                        parsingState_ = SEARCHING_SEGHEADER;
                    }
                }

                break;
            }
        }
    }
    return true;
}

//Only works with nellymoser
const char flvHeader[] = {
    0x46,0x4c,0x56,0x01,0x05, //5 bytes flv + type
    0x00,0x00,0x00,0x09, //length = 9
    0x00,0x00,0x00,0x00, //prev len = 0
    0x12,0x00,0x01,0x23,0x00,0x00,0x00,0x00,0x00,0x00,0x00, //11 bytes data tag, len=0x12E-0x0b(11 bytes)
    0x02,
    0x00,0x0a,0x6f,0x6e,0x4d,0x65,0x74,0x61,0x44,0x61,0x74,0x61, //onMetaData
    0x08,0x00,0x00,0x00,0x0d, //array of 13 elements
    0x00,0x08,0x64,0x75,0x72,0x61,0x74,0x69,0x6f,0x6e, //duration
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, //0.00
    0x00,0x0C,0x61,0x75,0x64,0x69,0x6F,0x63,0x6F,0x64,0x65,0x63,0x69,0x64, //audiocodecid
    0x00,0x40,0x18,0x00,0x00,0x00,0x00,0x00,0x00, //speex = 11(0x4026000000000000), 16kmp3 = 15(0x402e000000000000), nellymoser=6(4018000000000000)
    0x00,0x0D,0x61,0x75,0x64,0x69,0x6F,0x64,0x61,0x74,0x61,0x72,0x61,0x74,0x65,//audiodatarate
    0x00,0x40,0x3B,0xCC,0xCC,0xCC,0xCC,0xCC,0xCD,
    0x00,0x0F,0x61,0x75,0x64,0x69,0x6F,0x73,0x61,0x6D,0x70,0x6C,0x65,0x72,0x61,0x74,0x65, //audiosamplerate
    0x00,0x40,0xCF,0x40,0x00,0x00,0x00,0x00,0x00, //16k
    0x00,0x0F,0x61,0x75,0x64,0x69,0x6F,0x73,0x61,0x6D,0x70,0x6C,0x65,0x73,0x69,0x7A,0x65, //audiosamplesize
    0x00,0x40,0x30,0x00,0x00,0x00,0x00,0x00,0x00, //16bits
    0x00,0x06,0x73,0x74,0x65,0x72,0x65,0x6F, //stereo
    0x01,0x00, //bool, mono
    0x00,0x0c,0x76,0x69,0x64,0x65,0x6f,0x63,0x6f,0x64,0x65,0x63,0x69,0x64, //videocodecid
    0x00,0x40,0x1c,0x00,0x00,0x00,0x00,0x00,0x00,//0x4020000000000000 stands for codecid=8 = vp8, 7=avc= 401c000000000000
    0x00,0x05,0x77,0x69,0x64,0x74,0x68, //width
    0x00,0x40,0x84,0x00,0x00,0x00,0x00,0x00,0x00,// = 640.00
    0x00,0x06,0x68,0x65,0x69,0x67,0x68,0x74, //height
    0x00,0x40,0x7e,0x00,0x00,0x00,0x00,0x00,0x00, // = 480.00
    0x00,0x09,0x66,0x72,0x61,0x6d,0x65,0x72,0x61,0x74,0x65, //framerate
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,//0
    0x00,0x0d,0x76,0x69,0x64,0x65,0x6f,0x64,0x61,0x74,0x61,0x72,0x61,0x74,0x65, //videodatarate ???
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,//0.00
    0x00,0x07,0x65,0x6e,0x63,0x6f,0x64,0x65,0x72, //encoder
    0x02,0x00,0x0b,0x4c,0x61,0x76,0x66,0x35,0x32,0x2e,0x38,0x37,0x2e,0x31,
    0x00,0x08,0x66,0x69,0x6c,0x65,0x73,0x69,0x7a,0x65, //filesize
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, //0.00
    0x00,0x00,0x09, //ends with 9
    0x00,0x00,0x01,0x2E //previous tag len, len=0x12E
};

//big enough buffer
const int MAX_BUF_SIZE = 40960;

//TODO ONLY parse the first video
int main(int argc, char** argv)
{
    Logger::initLog("SegInputParser", kStderr);
    if( argc!=2 ) {
        printf("usage: %s outputFLVFile\r\n", argv[0]);
        return 0;
    }
    string fileName = argv[1];

    writer_.setFileName(fileName);
    //writes the flv header first
    writer_.writeData(flvHeader, sizeof(flvHeader));
    printf("Write header to: %s, len=%ld\r\n", argv[1], sizeof(flvHeader));

    u8 data[MAX_BUF_SIZE];
    u32 totalBytesRead = 0;
    while ( (totalBytesRead = read( 0, data, MAX_BUF_SIZE)) > 0 ) {
        readData(data, totalBytesRead);
    }    
}
