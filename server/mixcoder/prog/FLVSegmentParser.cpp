#include "FLVSegmentParser.h"
#include <assert.h>
#include <stdio.h>

bool FLVSegmentParser::isNextStreamAvailable(StreamType streamType, u32& timestamp)
{
    //TODO
    //algorithm here to detect whether it's avaiable
    if( streamType == kVideoStreamType ) {
        //using the frist stream timestamp as the base timestamp
        for(int i = 0; i < MAX_XCODING_INSTANCES; i++ ) {
            if ( videoStreamStatus_[i] == kStreamOnlineStarted && videoQueue_[i].size() > 0) {
                timestamp = videoQueue_[i].front()->pts;
                return true;
            }
        }
    } else if( streamType == kAudioStreamType ) {
        //using the frist stream timestamp as the base timestamp
        for(int i = 0; i < MAX_XCODING_INSTANCES; i++ ) {
            if ( audioStreamStatus_[i] == kStreamOnlineStarted && audioQueue_[i].size() > 0) {
                timestamp = audioQueue_[i].front()->pts;
                return true;
            }
        }
    }
    return false;
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
        videoStreamStatus_[index] = kStreamOnlineStarted;
    } else if ( au->st == kAudioStreamType ) {
        audioQueue_[index].push( au );
        audioStreamStatus_[index] = kStreamOnlineStarted;
    } else {
        //do nothing
    }
}


bool FLVSegmentParser::readData(SmartPtr<SmartBuffer> input)
{
    u8* data = input->data();
    u32 len = input->dataLength();

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
                    assert(curBuf_[0] == 'S' && curBuf_[1] == 'E' && curBuf_[2] == 'G');
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
                    assert(numStreams_ < (u32)MAX_XCODING_INSTANCES);
                    
                    int index = 0;
                    while( streamMask ) {
                        u32 value = ((streamMask<<31)>>31); //mask off all other bits
                        if( value ) {
                            if ( videoStreamStatus_[index] == kStreamOffline ) {
                                videoStreamStatus_[index] = kStreamOnlineNotStarted;
                            }
                            if ( audioStreamStatus_[index] == kStreamOffline ) {
                                audioStreamStatus_[index] = kStreamOnlineNotStarted;
                            }
                            //fprintf(stderr, "---streamMask index=%d, numStreams=%d\r\n", index, numStreams_);
                        } else {
                            videoStreamStatus_[index] = kStreamOffline;
                            audioStreamStatus_[index] = kStreamOffline;
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
                    curStreamId_ = curBuf_[0]; //read the id
                    assert(curStreamId_ < (u32)MAX_XCODING_INSTANCES);
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
                    //fprintf(stderr, "---curStreamId_=%d curStreamLen_=%d\r\n", curStreamId_, curStreamLen_);
                    //read the actual buffer
                    SmartPtr<SmartBuffer> curStream = new SmartBuffer( curStreamLen_, curBuf_.data());
                    parser_[curStreamId_]->readData(curStream); 
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

SmartPtr<AccessUnit> FLVSegmentParser::getNextFLVFrame(u32 index, StreamType streamType)
{
    assert ( index < numStreams_ );
    SmartPtr<AccessUnit> au;
    if ( streamType == kVideoStreamType ) {
        if ( videoQueue_[index].size() > 0 ) {
            au = videoQueue_[index].front();
            if ( au ) {
                videoQueue_[index].pop();
            }
        }
    } else if( streamType == kAudioStreamType ){
        if ( audioQueue_[index].size() > 0 ) {
            au = audioQueue_[index].front();
            if ( au ) {
                audioQueue_[index].pop();
            }
        }
    }
    return au;
}
