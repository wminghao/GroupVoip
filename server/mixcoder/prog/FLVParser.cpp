#include "FLVParser.h"
#include "CodecInfo.h"
#include "fwk/BitStreamParser.h"
#include <stdio.h>

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
                    curBuf_ = curBuf_.substr(4); //skip 4 bytes
                    curFlvTagSize_ += 7+4; //add remaining of the header + previousTagLen
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
    accessUnit->pts = accessUnit->dts = tsUnion.timestamp;
    
    //skip 3 byte
    bsParser.readBytes(3);
    dataSize -= 7;


    fprintf(stderr, "---streamType=%d, flvTagSize=%d, pts=%d\r\n", curStreamType_, curFlvTagSize_, (u32)accessUnit->pts );
    
    if ( dataSize > 0 ) {
        switch ( accessUnit->st ) {
        case kVideoStreamType:
            {
                u32 frameType = bsParser.readBits(4);
                accessUnit->isKey = (frameType == 1);
                u32 codecId = bsParser.readBits(4);
                accessUnit->ct = codecId;
                dataSize -= 1;

                std::string inputData;
                int inputDataSize = 0;
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
                            inputDataSize = 4 + spsLen + 4 + ppsLen;
                            fprintf(stderr, "---spsLen = %d, ppsLen = %d, inputDataLen=%ld\r\n", spsLen, ppsLen, inputData.size());
                            break;
                        }
                    case kAVCNalu:
                        {
                            accessUnit->sp = kRawData;
                            
                            //avcodec_decode_video2 & avcodec_decode_audio4 documentation requires an extra of FF_INPUT_BUFFER_PADDING_SIZE padding for each video buffer
                            const string stBytesPadding ("\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16);// FF_INPUT_BUFFER_PADDING_SIZE = 16
                            string lenStr = bsParser.readBytes(4);//skip the length
                            union DataSizeUnion{
                                u32 dataSize;
                                u8 dataSizeStr[4];
                            }dsUnion;
                            dsUnion.dataSizeStr[0] = lenStr[3];
                            dsUnion.dataSizeStr[1] = lenStr[2];
                            dsUnion.dataSizeStr[2] = lenStr[1];
                            dsUnion.dataSizeStr[3] = lenStr[0];

                            dataSize -= 4;
                            inputData = naluStarterCode + bsParser.readBytes(dataSize) + stBytesPadding;
                            inputDataSize = 4 + dataSize + 16;
                            fprintf(stderr, "---inputData size=%ld, len=%d  %x_%x_%x_%x_0x%x\r\n", dataSize, dsUnion.dataSize, inputData[0], inputData[1], inputData[2], inputData[3], inputData[4]);
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
                if ( inputDataSize > 0 ) {
                    //read payload. 
                    accessUnit->payload = new SmartBuffer( inputDataSize, inputData.data());
                    delegate_->onFLVFrameParsed( accessUnit, index_ );
                }
                fprintf(stderr, "---video accessUnit, isKey=%d, codecType=%d, specialProperty=%d, naluSize=%d\r\n", accessUnit->isKey, accessUnit->ct, accessUnit->sp, inputDataSize);
                            
                break;
            }
        case kAudioStreamType:
            {
                u32 soundFormat = bsParser.readBits(4);
                u32 soundRate = bsParser.readBits(2);
                u32 soundSize = bsParser.readBits(1);
                u32 soundType = bsParser.readBits(1);
                dataSize -= 1;

                accessUnit->ct = soundFormat;
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
                    delegate_->onFLVFrameParsed( accessUnit, index_ );
                }
                fprintf(stderr, "---audio accessUnit, isKey=%d, codecType=%d, specialProperty=%d\r\n", accessUnit->isKey, accessUnit->ct, accessUnit->sp);
                break;
            }
        case kDataStreamType:
            {
                //TODO
                fprintf(stderr, "---data accessUnit.\r\n");
            }
        default:
            {
                break;
            }
        }
    }
}
