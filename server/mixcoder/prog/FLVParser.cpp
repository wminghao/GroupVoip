#include "FLVParser.h"
#include "fwk/BitStreamParser.h"

void FLVParser::readData(SmartPtr<SmartBuffer> input); {
    curBuffer_ = string( input->data(), input->dataLength());
}

SmartPtr<AccessUnit> FLVParser::getNextFLVFrame()
{
    SmartPtr<AccessUnit> accessUnit = new AccessUnit();

    BitStreamParser bsParser(curBuffer_);
    //parsing logic for FLV frames
    accessUnit->st = bsParser.readByte(); //streamType
    std::string tempStr = bsParser.readBytes(3); //3 bytes, length of data
    size_t dataSize = 0;
    memcpy(&dataSize, tempStr.data(), 3);
    
    //read 4 bytes of ts
    tempStr = bsParser.readBytes(3);
    u8 tempByte = bsParser.readByte();
    union TimestampUnion{
        u32 timestamp;
        u8 timestampStr[4];
    }tsUnion;
    tsUnion.timestampStr[0] = tempStr[2];
    tsUnion.timestampStr[1] = tempStr[1];
    tsUnion.timestampStr[2] = tempStr[0];
    tsUnion.timestampStr[3] = tempByte;
    accessUnit->pts = accessUnit->ds = tsUnion.timestamp;
    
    //skip 1 byte
    bsParser.readByte();
    
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
                            accessUnit->sp = kRaw;
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
                    accessUnit->sp = kRaw;
                }
                if ( dataSize > 0 ) {
                    //read payload. 
                    accessUnit->payload = new SmartBuffer( dataSize, bsParser.readBytes(dataSize).data());
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

                if ( codecId == kAAC ) {
                    u8 aacPacketType = bsParser.readByte();
                    switch( aacPacketType ) {
                    case kAACSeqHeader:
                        {
                            accessUnit->sp = kSeqHeader;
                            break;
                        }
                    case kAACRaw:
                        {
                            accessUnit->sp = kRaw;
                            break;
                        }
                    }
                    dataSize -= 1;
                } else {
                    accessUnit->sp = kRaw;
                }
                if ( dataSize > 0 ) {
                    //read payload. 
                    accessUnit->payload = new SmartBuffer( dataSize, bsParser.readBytes(dataSize).data());
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
    return accessUnit;
}
