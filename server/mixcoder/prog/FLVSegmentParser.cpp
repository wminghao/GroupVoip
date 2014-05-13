#include "FLVSegmentParser.h"
#include <assert.h>
#include <stdio.h>

#define OUTPUT_VIDEO_FRAME_RATE 30

////////////////////////////////////////////////////////////////
// Audio is always continuous. Video can not be faster than audio.
// There are 3 timestamps to look at.
// 1) frameTimestamp, max timestamp of the current frames, no bigger than limitTimestamp(MAX of nextBucketTimestamp and audioBucketTimestamp)
//                         for any frameTimestamp bigger, treat it the next bucket.
// 2) nextBucketTimestamp, in normal case, everytime, bucket advance by 33ms, 
//                         in cases where there is video within the nextBucket, it's used.
// 3) audioBucketTimestamp, in abnormal case, bucket can advance by multiple of 33ms
//                         in cases where there is no video within the nextBucket, it's used.
////////////////////////////////////////////////////////////////
bool FLVSegmentParser::isNextVideoStreamReady(u32& videoTimestamp, u32 audioTimestamp)
{
    //isReady means every 33ms, there is a stream ready
    bool isReady = false; //different meaning for audio and video

    double frameInterval = (double)1000 /(double)OUTPUT_VIDEO_FRAME_RATE;

    //nextBucketTimestamp is every 33ms since the beginning of video stream
    double nextBucketTimestamp = lastBucketTimestamp_ + frameInterval;

    //audioBuckeTimestamp is the bucket under which the audio packet falls into. (strictly folow 33ms rule)
    double audioBucketTimestamp = lastBucketTimestamp_ + frameInterval * ((int)(((double)audioTimestamp - lastBucketTimestamp_)/frameInterval)); //strictly follow 33ms rule

    //nextLimitTimestamp is useful if audio is ahead of video bucket.
    u32 nextLimitTimestamp = MAX( nextBucketTimestamp, audioBucketTimestamp );

    //frame timestamp is the max video timestamp before the limit
    u32 frameTimestamp = 0xffffffff;

    bool hasAnyStreamStartedAndReady = false;
    bool hasSpsPps = false;
    for(u32 i = 0; i < MAX_XCODING_INSTANCES; i++ ) {
        if ( videoStreamStatus_[i] == kStreamOnlineStarted ) {
            if( videoQueue_[i].size() > 0 ) {
                if (isNextVideoFrameSpsPps(i)) {
                    hasSpsPps = true;
                } else {
                    hasAnyStreamStartedAndReady = true;
                    bool recordFrameTimestamp = true;
                    if( hasStarted_ ) {
                        //if it's over the limit, don't record
                        if( videoQueue_[i].front()->pts > nextLimitTimestamp ) {
                            recordFrameTimestamp = false;
                        }
                    }
                    if( recordFrameTimestamp ) {
                        if( frameTimestamp ==  0xffffffff ) {
                            frameTimestamp = videoQueue_[i].front()->pts;
                        } else {
                            frameTimestamp = MAX(videoQueue_[i].front()->pts, frameTimestamp);
                        }
                    }
                }
            } else {
                //fprintf(stderr, "---streamMask online unavailable index=%d, numStreams=%d\r\n", i, numStreams_);
            }
        } 
    }
        
    //after the first frame. every 33ms, considers it's ready, regardless whether there is a frame or not
    if( hasStarted_ ) {
        if ( frameTimestamp != 0xffffffff ) {
            if( frameTimestamp <= (u32)nextBucketTimestamp ) { 
                if( audioBucketTimestamp >= nextBucketTimestamp) {
                    //video has accumulated some data and audio has already catch up
                    videoTimestamp = lastBucketTimestamp_ = nextBucketTimestamp; //strictly follow
                    isReady = true;
                    /*
                    fprintf(stderr, "===follow up video timstamp=%d, hasAnyStreamStartedAndReady=%d, audioBucketTimestamp=%d nextBucketTimestamp=%d, lastBucketTimestamp_=%d\r\n", 
                            videoTimestamp, hasAnyStreamStartedAndReady, (u32)audioBucketTimestamp, (u32)nextBucketTimestamp, (u32)lastBucketTimestamp_);
                    */
                } else {
                    //wait for the nextBucketTimestamp, since audio is not ready yet
                    //not ready yet
                    isReady = false;
                }
            } else { 
                assert( audioBucketTimestamp >= frameTimestamp );
                assert( audioBucketTimestamp > nextBucketTimestamp );
                //if audio is already ahead, pop that frame out
                videoTimestamp = lastBucketTimestamp_ = audioBucketTimestamp;
                /*
                  fprintf(stderr, "===follow up 2 video timstamp=%d, hasAnyStreamStartedAndReady=%d, audioBucketTimestamp=%d nextBucketTimestamp=%d, lastBucketTimestamp_=%d\r\n", 
                        videoTimestamp, hasAnyStreamStartedAndReady, (u32)audioBucketTimestamp, (u32)nextBucketTimestamp, (u32)lastBucketTimestamp_);
                */
                isReady = true;
            }
        } else {
            //no data available
            //wait for the nextBucketTimestamp
            isReady = false;
        }
    } else {
        //first time there is a stream available, always pop out the frame(s)            
        if( hasAnyStreamStartedAndReady ) {
            hasStarted_ = true;
            lastBucketTimestamp_ = frameTimestamp;
            videoTimestamp = frameTimestamp;
            fprintf(stderr, "===first video timstamp=%d\r\n", (u32)lastBucketTimestamp_);
            isReady = true;
        } else if( hasSpsPps ) {
            //if there is no frame ready, only sps/pps pop out it immediately
            fprintf(stderr, "---found sps pps. but no other frames\r\n");
            isReady = true;
        }
    }
    
    return isReady;
}

bool FLVSegmentParser::isNextAudioStreamReady(u32& audioTimestamp) {
    audioTimestamp = 0xffffffff;
    int totalStreams = 0;
    bool isReady = true; //all streams ready means ready
    //all audio frame rate is the same
    for(u32 i = 0; i < MAX_XCODING_INSTANCES; i++ ) {
        if ( audioStreamStatus_[i] == kStreamOnlineStarted ) { 
            if( audioQueue_[i].size() > 0) {
                audioTimestamp = MIN(audioQueue_[i].front()->pts, audioTimestamp);
            } else {
                isReady = false;
                //fprintf(stderr, "---streamMask online unavailable index=%d, numStreams=%d\r\n", i, numStreams_);
                break;
            }   
            totalStreams++;
        }
    }
    return totalStreams?isReady:false;
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
                    //fprintf(stderr, "---streamMask=0x%x\r\n", streamMask);
                    int index = 0;
                    while( index < (int)MAX_XCODING_INSTANCES ) {
                        u32 value = ((streamMask<<31)>>31); //mask off all other bits
                        if( value ) {
                            if ( videoStreamStatus_[index] == kStreamOffline ) {
                                videoStreamStatus_[index] = kStreamOnlineNotStarted;
                            }
                            if ( audioStreamStatus_[index] == kStreamOffline ) {
                                audioStreamStatus_[index] = kStreamOnlineNotStarted;
                            }
                            //fprintf(stderr, "---streamMask online  index=%d, numStreams=%d\r\n", index, numStreams_);
                        } else {
                            videoStreamStatus_[index] = kStreamOffline;
                            audioStreamStatus_[index] = kStreamOffline;
                            //fprintf(stderr, "---streamMask offline index=%d, numStreams=%d\r\n", index, numStreams_);
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
                    //fprintf(stderr, "---curBuf_[0]=0x%x, curStreamId_=%d curStreamSource=%d\r\n", curBuf_[0], curStreamId_, curStreamSource);
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
                bool bIsFinished = false;
                if ( curStreamLen_ ) {
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

bool FLVSegmentParser::isNextVideoFrameSpsPps(u32 index)
{
    bool bIsSpsPps = false;
    if ( videoQueue_[index].size() > 0 ) {
        SmartPtr<AccessUnit> au = videoQueue_[index].front();
        if ( au && au->sp == kSpsPps ) {
            bIsSpsPps = true;
        }
    }
    return bIsSpsPps;
}

//canr eturn more than 1 frome
SmartPtr<AccessUnit> FLVSegmentParser::getNextVideoFrame(u32 index, u32 timestamp)
{
    SmartPtr<AccessUnit> au;
    if ( videoQueue_[index].size() > 0 ) {
        au = videoQueue_[index].front();
        if ( au && au->pts <= timestamp ) {
            //fprintf( stderr, "------pop Next video frame, index=%d pts=%d\r\n", index, au->pts);
            videoQueue_[index].pop();
        } else {
            //fprintf( stderr, "------nopop Next video frame, index=%d pts=%d\r\n", index, au->pts);
            //don't pop anything that has a bigger timestamp
            au = NULL;
        }
    }
    return au;
}

SmartPtr<AccessUnit> FLVSegmentParser::getNextAudioFrame(u32 index)
{
    SmartPtr<AccessUnit> au;
    if ( audioQueue_[index].size() > 0 ) {
        au = audioQueue_[index].front();
        if ( au ) {
            audioQueue_[index].pop();
        }
    }
    return au;
}
