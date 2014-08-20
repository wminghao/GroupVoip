#include "FLVParser.h"
#include "CodecInfo.h"
#include "fwk/BitStreamParser.h"
#include <stdio.h>
#include <assert.h>

//parsing the raw data to get a complete FLV frame
void FLVParser::readData(SmartPtr<SmartBuffer> input) {
    u8* data = input->data();
    u32 len = input->dataLength();
    
    while( len ) {
        switch( scanState_ ) {
        case SCAN_HEADER_TYPE_LEN:
            {
                if ( curBuf_.size() < 4 ) {
                    size_t cpLen = MIN(len, 4-curBuf_.size());
                    curBuf_ += string((const char*)data, cpLen); //concatenate the string
                    len -= cpLen;
                    data += cpLen;
                }
                if ( curBuf_.size() >= 4 ) {
                    curStreamType_ = (StreamType)curBuf_[0];
                    std::string tempStr = curBuf_.substr(1, 3);
                    union DataSizeUnion{
                        u32 dataSize;
                        u8 dataSizeStr[4];
                    }dsUnion;
                    dsUnion.dataSizeStr[0] = tempStr[2];
                    dsUnion.dataSizeStr[1] = tempStr[1];
                    dsUnion.dataSizeStr[2] = tempStr[0];
                    dsUnion.dataSizeStr[3] = 0;
                    curFlvTagSize_ = dsUnion.dataSize;
                    curFlvTagSize_ += 7+4; //add remaining of the header + previousTagLen
                    curBuf_ = curBuf_.substr(4, curFlvTagSize_ ); //skip 4 bytes
                    scanState_ = SCAN_REMAINING_TAG;
                }
                break;
            }
        case SCAN_REMAINING_TAG:
            {
                if ( curBuf_.size() < curFlvTagSize_ ) {
                    size_t cpLen = MIN(len, curFlvTagSize_-curBuf_.size());
                    curBuf_ += string((const char*)data, cpLen); //concatenate the string                                                                                                                                                                       
                    len -= cpLen;
                    data += cpLen;
                }
                if ( curBuf_.size() >= curFlvTagSize_ ) {
                    parseNextFLVFrame( curBuf_ );
                    curBuf_.clear();
                    curStreamType_ = kUnknownStreamType;
                    curFlvTagSize_  = 0; //reset and go to the first state
                    scanState_ = SCAN_HEADER_TYPE_LEN;
                }
                break;
            }
        }
    }
}

void FLVParser::parseNextFLVFrame( string& strFlvTag )
{
    SmartPtr<AccessUnit> accessUnit = new AccessUnit();

    BitStreamParser bsParser(strFlvTag);
    //parsing logic for FLV frames, the first 4 bytes are already parsed
    accessUnit->st = curStreamType_;
    size_t dataSize = curFlvTagSize_ - 4; //previous header
    
    //read 4 bytes of ts
    string tempStr = bsParser.readBytes(3);
    u8 tempByte = bsParser.readByte();
    union TimestampUnion{
        u32 timestamp;
        u8 timestampStr[4];
    }tsUnion;
    tsUnion.timestampStr[0] = tempStr[2];
    tsUnion.timestampStr[1] = tempStr[1];
    tsUnion.timestampStr[2] = tempStr[0];
    tsUnion.timestampStr[3] = tempByte;

    //skip 3 byte
    bsParser.readBytes(3);
    dataSize -= 7;

    if ( dataSize > 0 ) {
        bool frameReady = false;

        switch ( accessUnit->st ) {
        case kVideoStreamType:
            {
                u32 frameType = bsParser.readBits(4);
                accessUnit->isKey = (frameType == 1);
                u32 codecId = bsParser.readBits(4);
                accessUnit->ctype = codecId;
                dataSize -= 1;

                std::string inputData;
                if ( codecId == kAVCVideoPacket ) {
                    u8 avcPacketType = bsParser.readByte();
                    //skip 3 bytes
                    bsParser.readBytes(3);
                    dataSize -= 4;
                    
                    const std::string naluStarterCode ("\000\000\000\001", 4);
                    
                    switch( avcPacketType ) {
                    case kAVCSeqHeader:
                        {
                            accessUnit->sp = kSpsPps;
                            
                            //parse sps/pps properly from flash format to nalu format
                            /* Flash format
                             *  0x01 ( 8 bits )
                             *  second byte of SPS (profile, 8 bits) //repeat
                             *  third byte of SPS (profile_compatibility, 8 bits) //repeat
                             *  fourth byte of SPS (level indication, 8 bits) //repeat
                             *  111111 ( 6 bits )
                             *  length size minus 1 ( 2 bits, should always be 11 )
                             *  111b ( 3 bits )
                             *  numSPS ( 5 bits, likely should always be 1 )
                             *  SPS length ( 16 bits )
                             *  SPS
                             *  numPPS ( 8 bits, likely should always be 1 )
                             *  PPS length ( 16 bits )
                             *  PPS
                             * NALU format
                             *  0001+sps+0001+pps
                             */
                            bsParser.readBytes(6);
                            string spsLenStr = bsParser.readBytes(2);
                            u16 spsLen = ((u16)spsLenStr[0]<<8)|spsLenStr[1];
                            string sps = bsParser.readBytes(spsLen);
                            bsParser.readBytes(1);
                            string ppsLenStr = bsParser.readBytes(2);
                            u16 ppsLen = ((u16)ppsLenStr[0]<<8)|ppsLenStr[1];
                            string pps = bsParser.readBytes(ppsLen);                            
                            inputData = naluStarterCode + sps + naluStarterCode + pps;
                            //LOG( "---spsLen = %d, ppsLen = %d, inputDataLen=%ld\r\n", spsLen, ppsLen, inputData.size());
                            break;
                        }
                    case kAVCNalu:
                        {
                            accessUnit->sp = kRawData;
                            
                            //avcodec_decode_video2 & avcodec_decode_audio4 documentation requires an extra of FF_INPUT_BUFFER_PADDING_SIZE padding for each video buffer
                            const string stBytesPadding ("\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16);// FF_INPUT_BUFFER_PADDING_SIZE = 16
                            inputData.clear();
                            while ( dataSize > 0 ) {
                                string lenStr = bsParser.readBytes(4);//skip the length
                                union DataSizeUnion{
                                    u32 dataSize;
                                    u8 dataSizeStr[4];
                                }dsUnion;
                                dsUnion.dataSizeStr[0] = lenStr[3];
                                dsUnion.dataSizeStr[1] = lenStr[2];
                                dsUnion.dataSizeStr[2] = lenStr[1];
                                dsUnion.dataSizeStr[3] = lenStr[0];
                                
                                string slice = bsParser.readBytes(dsUnion.dataSize);
                                if( dsUnion.dataSize > 0) {
                                    inputData += (naluStarterCode + slice);
                                }
                                dataSize -= (4+dsUnion.dataSize);

                                //LOG( "---slice size=%d, first byte=0x%x\r\n", dsUnion.dataSize, slice[0]);
                            }
                            inputData += stBytesPadding;
                            //LOG( "---inputData %x_%x_%x_%x_0x%x__0x%x\r\n", inputData[0], inputData[1], inputData[2], inputData[3], inputData[4], inputData[inputData.size()-1-16]);
                            break;
                        }
                    case kAVCEndOfSeq:
                        {
                            accessUnit->sp = kEndSeq;
                            break;
                        }
                    }
                } else {
                    assert(0);
                    accessUnit->sp = kRawData;
                }
                if ( inputData.size() > 0 ) {
                    
                    bool bIsFrameReady = true;
                    if( accessUnit->sp == kSpsPps ) {
                        //same spspps, don't enqueue
                        if( !inputData.compare(curSpsPps_) ) {
                            bIsFrameReady = false;
                        } else {
                            curSpsPps_ = inputData;
                        }
                    }
                    
                    if( bIsFrameReady ) {
                        //read payload. 
                        accessUnit->payload = new SmartBuffer( inputData.size(), inputData.data());
                        frameReady = true;
                    } else {
                        frameReady = false;
                    }
                }
                //LOG( "---video accessUnit, isKey=%d, codecType=%d, specialProperty=%d, naluSize=%ld\r\n", accessUnit->isKey, accessUnit->ct, accessUnit->sp, inputData.size());
                            
                break;
            }
        case kAudioStreamType:
            {
                u32 soundFormat = bsParser.readBits(4);
                u32 soundRate = bsParser.readBits(2);
                u32 soundSize = bsParser.readBits(1);
                u32 soundType = bsParser.readBits(1);
                dataSize -= 1;

                accessUnit->ctype = (soundFormat<<4)|(soundRate<<2)|(soundSize<<1)|soundType;
                accessUnit->isKey = true;
                if ( soundFormat == kAAC ) {
                    u8 aacPacketType = bsParser.readByte();
                    switch( aacPacketType ) {
                    case kAACSeqHeader:
                        {
                            accessUnit->sp = kSeqHeader;
                            break;
                        }
                    case kAACRaw:
                        {
                            accessUnit->sp = kRawData;
                            break;
                        }
                    }
                    dataSize -= 1;
                } else {
                    accessUnit->sp = kRawData;
                }
                if ( dataSize > 0 ) {
                    //read payload. 
                    accessUnit->payload = new SmartBuffer( dataSize, bsParser.readBytes(dataSize).data());
                    frameReady = true;
                }
                //LOG( "---audio accessUnit, isKey=%d, codecType=%d, specialProperty=%d\r\n", accessUnit->isKey, accessUnit->ct, accessUnit->sp);
                break;
            }
        case kDataStreamType:
            {
                //TODO
                //LOG( "---data accessUnit.\r\n");
            }
        default:
            {
                break;
            }
        }

        //if there is NO global timestamp, we use a relative timestamp to re-adjust the clock
        //based on the very 1st audio frame or very 1st video frame(not sps)
        if( MAX_S32 == relTimeStampOffset_ && accessUnit->st != kDataStreamType && accessUnit->sp != kSpsPps) {
            //u64 curEpocTime = getEpocTime();
            //assert( curEpocTime > startEpocTime_ );
            //relTimeStampOffset_ = ( curEpocTime - startEpocTime_ ) - tsUnion.timestamp;
            relTimeStampOffset_ = delegate_->getGlobalAudioTimestamp() - tsUnion.timestamp;
        } else {
            //if the drift is bigger than 100 ms, that means the current stream is catching up to the current time by re-adjusting its own clock.
            //that's the case when 2 publishers, a second publisher initially sends a frame with low ts, and jumps to a high ts immediately afterwards
            if( accessUnit->st == kAudioStreamType && tsUnion.timestamp > prevAudioOrigPts_ + 100 ) {
                relTimeStampOffset_ = delegate_->getGlobalAudioTimestamp() - tsUnion.timestamp;
                LOG( "==========================Adjusted relTimestampOffset_=%d===========\r\n", relTimeStampOffset_);
            }
        }
        if ( accessUnit->sp == kSpsPps ) {
            //reset the spspps timestamp to be next ts
            accessUnit->pts = prevVideoAdjPts_+1;
        } else {
            s32 relTimestampOffset = (relTimeStampOffset_ == MAX_S32)?0:relTimeStampOffset_; 
            if( relTimestampOffset >= 0 || (s32)(tsUnion.timestamp + relTimestampOffset) >= 0) {
                accessUnit->pts = tsUnion.timestamp + relTimestampOffset;
            } else {
                //cannot be negative
                if( accessUnit->st == kVideoStreamType ) {
                    accessUnit->pts = prevVideoAdjPts_ + 1;
                } else {
                    accessUnit->pts = prevAudioAdjPts_ + 1;
                }
            }
        }

        if( accessUnit->st == kVideoStreamType ) {
            prevVideoAdjPts_ = accessUnit->pts;
        } else if( accessUnit->st == kAudioStreamType ) {
            prevAudioAdjPts_ = accessUnit->pts;
            prevAudioOrigPts_ = tsUnion.timestamp;
        }
        LOG( "---index=%d, ready=%d, streamType=%d, flvTagSize=%d, oPts=%d,  relTsOffset_=%d, npts=%d\r\n", index_, frameReady, curStreamType_, curFlvTagSize_, tsUnion.timestamp, relTimeStampOffset_, (u32)accessUnit->pts );

        if( frameReady ) {
            delegate_->onFLVFrameParsed( accessUnit, index_ );
        }
    }
}
