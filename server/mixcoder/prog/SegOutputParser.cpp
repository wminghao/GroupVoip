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

FileWriter writer_[MAX_XCODING_INSTANCES+1];
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
                if ( curBuf_.size() < 3 ) {
                    size_t cpLen = MIN(len, 3-curBuf_.size());
                    curBuf_ += string((const char*)data, cpLen); //concatenate the string                                                                                                                 
                    len -= cpLen;
                    data += cpLen;
                }

                if ( curBuf_.size() >= 3 ) {
                    assert(curBuf_[0] == 'S' && curBuf_[1] == 'G' && curBuf_[2] == 'O');
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
                    numStreams_ = count_bits(streamMask)+1;
                    assert(numStreams_ < (u32)MAX_XCODING_INSTANCES);
                    
                    //LOG( "---streamMask=%d numStreams_=%d\r\n", streamMask, numStreams_);
                    int index = 0;
                    while( streamMask ) {
                        u32 value = ((streamMask<<31)>>31); //mask off all other bits
                        if( value ) {
                            //LOG( "---streamMask index=%d is valid\r\n", index);
                        }
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
                if ( curBuf_.size() < 5 ) {
                    size_t cpLen = MIN(len, 5-curBuf_.size());
                    curBuf_ += string((const char*)data, cpLen); //concatenate the string
                    len -= cpLen;
                    data += cpLen;
                }
                if ( curBuf_.size() >= 5 ) {
                    curStreamId_ = curBuf_[0];
                    //LOG( "---curBuf_[0]=%d, curStreamId_=%d\r\n", curBuf_[0], curStreamId_);
                    assert(curStreamId_ <= (u32)MAX_XCODING_INSTANCES);                    

                    memcpy(&curStreamLen_, curBuf_.data()+1, 4); //read the len
                    curBuf_.clear();
                    curSegTagSize_ = 0;
                    parsingState_ = SEARCHING_STREAM_DATA;
                }
                break;
            }
        case SEARCHING_STREAM_DATA:
            {
                if ( curBuf_.size() < curStreamLen_ ) {
                    size_t cpLen = MIN(len, curStreamLen_-curBuf_.size());
                    curBuf_ += string((const char*)data, cpLen); //concatenate the string     
                                  
                    len -= cpLen;
                    data += cpLen;
                }
                if ( curBuf_.size() >= curStreamLen_ ) {
                    //LOG( "---curStreamId_=%d curStreamLen_=%d\r\n", curStreamId_, curStreamLen_);
                    //read the actual buffer
                    if( curStreamLen_ ) {
                        writer_[curStreamId_].writeData(curBuf_.data(), curStreamLen_); 
                    }
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

//big enough buffer
const int MAX_BUF_SIZE = 40960;

int main(int argc, char** argv)
{
    Logger::initLog("SegOutputParser", kSyslog);
    if( argc!=2 ) {
        LOG("usage: %s fileName\r\n", argv[0]);
        return 0;
    }
    
    string fileNamePrefix(argv[1]);
    for( u32 i=0; i<MAX_XCODING_INSTANCES+1; i++) {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%d", i);
        string concatStr = fileNamePrefix+"_"+string(buffer) +".flv";
        writer_[i].setFileName(concatStr);
    }
    
    u8 data[MAX_BUF_SIZE];
    u32 totalBytesRead = 0;
    while ( (totalBytesRead = read( 0, data, MAX_BUF_SIZE)) > 0 ) {
        readData(data, totalBytesRead);
    }    
}
