#include "FLVParser.h"
#include "CodecInfo.h"
#include "fwk/BitStreamParser.h"

inline u32 MIN(u32 a, u32 b) {
    return a < b? a : b;
}

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
                    memcpy(&curFlvTagSize_, tempStr.data(), 3);
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
                    curBuf_ = curBuf_.substr(curFlvTagSize_);
                    curStreamType_ = kUnknownStreamType;
                    curFlvTagSize_  = 0; //reset and go to the first state
                    scanState_ = SCAN_HEADER_TYPE_LEN;
                }
                break;
            }
        }
    }
    
    curBuf_ = string( (char*)input->data(), input->dataLength());
}

void FLVParser::parseNextFLVFrame( string & strFlvTag )
{
    SmartPtr<AccessUnit> accessUnit = new AccessUnit();

    BitStreamParser bsParser(strFlvTag);
    //parsing logic for FLV frames, the first 4 bytes are already parsed
    accessUnit->st = curStreamType_;
    size_t dataSize = curFlvTagSize_;
    
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
    
    if ( dataSize > 0 ) {
        switch ( accessUnit->st ) {
        case kVideoStreamType:
            {
                u32 frameType = bsParser.readBits(4);
                accessUnit->isKey = (frameType == 1);
                u32 codecId = bsParser.readBits(4);
                accessUnit->ct = codecId;
                dataSize -= 1;

                if ( codecId == kAVCVideoPacket ) {
                    u8 avcPacketType = bsParser.readByte();
                    switch( avcPacketType ) {
                    case kAVCSeqHeader:
                        {
                            accessUnit->sp = kSpsPps;
                            break;
                        }
                    case kAVCNalu:
                        {
                            accessUnit->sp = kRawData;
                            break;
                        }
                    case kAVCEndOfSeq:
                        {
                            accessUnit->sp = kEndSeq;
                            break;
                        }
                    }
                    bsParser.readBytes(3);
                    dataSize -= 4;
                } else {
                    accessUnit->sp = kRawData;
                }
                if ( dataSize > 0 ) {
                    //read payload. 
                    accessUnit->payload = new SmartBuffer( dataSize, bsParser.readBytes(dataSize).data());
                    delegate_->onFLVFrameParsed( accessUnit, index_ );
                }
                break;
            }
        case kAudioStreamType:
            {
                u32 soundFormat = bsParser.readBits(4);
                u32 soundRate = bsParser.readBits(2);
                u32 soundSize = bsParser.readBits(1);
                u32 soundType = bsParser.readBits(1);
                dataSize -= 1;

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
                break;
            }
        case kDataStreamType:
            {
                //TODO
            }
        default:
            {
                break;
            }
        }
    }
}
