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
    u32 timestamp;
    u8 timestampStr[4];
    timestampStr[0] = tempStr[2];
    timestampStr[1] = tempStr[1];
    timestampStr[2] = tempStr[0];
    timestampStr[3] = tempByte;
    
    memcpy(&timestamp, timestampStr, sizeof(u32));
    accessUnit->pts = accessUnit->ds = timestamp;
    
    //skip 1 byte
    bsParser.readByte();
    //read payload
    accessUnit->payload = new SmartBuffer( dataSize, bsParser.readBytes(dataSize).data());
    
    //TODO 
    accessUnit->isKey = false;
    accessUnit->ct = 1;
    
}
