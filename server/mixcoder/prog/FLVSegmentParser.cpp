#include "FLVSegmentParser.h"
#include <assert.h>
#include <stdio.h>

bool FLVSegmentParser::isNextStreamAvailable(StreamType streamType, u32& timestamp)
{
    bool isAvailable = true;
    int totalStreams = 0;

    timestamp = 0xffffffff;

    //TODO assume all video streams frame rate is the same and no frame drop, which is wrong assumption
    //For now, Wait until all streams are available at that moment
    //algorithm here to detect whether it's avaiable
    if( streamType == kVideoStreamType ) {
        for(u32 i = 0; i < MAX_XCODING_INSTANCES; i++ ) {
            if ( videoStreamStatus_[i] == kStreamOnlineStarted ) {
                if( videoQueue_[i].size() > 0) {
                    timestamp = MIN(videoQueue_[i].front()->pts, timestamp);
                } else {
                    isAvailable = false;
                    break;
                }
                totalStreams++;
            } 
        }
    } else if( streamType == kAudioStreamType ) {
        //all audio frame rate is the same
        for(u32 i = 0; i < MAX_XCODING_INSTANCES; i++ ) {
            if ( audioStreamStatus_[i] == kStreamOnlineStarted ) { 
                if( audioQueue_[i].size() > 0) {
                    timestamp = MIN(audioQueue_[i].front()->pts, timestamp);
                } else {
                    isAvailable = false;
                    break;
                }   
                totalStreams++;
            }
        }
    }
    return totalStreams?isAvailable:false;
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
                    //fprintf(stderr, "---curBuf_[0]=%d, curStreamId_=%d curStreamSource=%d\r\n", curBuf_[0], curStreamId_, curStreamSource);

                    assert( curStreamSource < kTotalStreamSource);
                    streamSource[curStreamId_] = (StreamSource)curStreamSource;

                    assert(curBuf_[1] == 0x0); //ignore the special property

                    memcpy(&curStreamLen_, curBuf_.data()+2, 4); //read the len
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
