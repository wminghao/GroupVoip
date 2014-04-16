#include "FLVOutput.h"
#include <stdio.h>

//5th byte 0x01 video, 0x04 audio, 0x05 video+audio
const u8 flvHeader[210] = {
    0x46,0x4c,0x56,0x01,0x05,0x00,0x00,0x00,0x09,0x00,0x00,0x00,0x00,0x12,0x00,0x00
    ,0xb6,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x0a,0x6f,0x6e,0x4d,0x65,0x74
    ,0x61,0x44,0x61,0x74,0x61,0x08,0x00,0x00,0x00,0x07,0x00,0x08,0x64,0x75,0x72,0x61
    ,0x74,0x69,0x6f,0x6e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x05,0x77
    ,0x69,0x64,0x74,0x68,0x00,0x40,0x84,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x68
    ,0x65,0x69,0x67,0x68,0x74,0x00,0x40,0x7e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x09
    ,0x66,0x72,0x61,0x6d,0x65,0x72,0x61,0x74,0x65,0x00,0x00,0x00,0x00,0x00,0x00,0x00
    ,0x00,0x00,0x00,0x0c,0x76,0x69,0x64,0x65,0x6f,0x63,0x6f,0x64,0x65,0x63,0x69,0x64
    ,0x00,0x40,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0d,0x76,0x69,0x64,0x65,0x6f //0x4020000000000000 stands for codecid=8, vp8
    ,0x64,0x61,0x74,0x61,0x72,0x61,0x74,0x65,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
    ,0x00,0x00,0x07,0x65,0x6e,0x63,0x6f,0x64,0x65,0x72,0x02,0x00,0x0b,0x4c,0x61,0x76
    ,0x66,0x35,0x32,0x2e,0x38,0x37,0x2e,0x31,0x00,0x08,0x66,0x69,0x6c,0x65,0x73,0x69
    ,0x7a,0x65,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x09,0x00,0x00
    ,0x00,0xc1
};

const u8 audioTagByte = 0xbe; //speex

SmartPtr<SmartBuffer> FLVOutput::newHeader()
{
    //ignore audioheader since it's speex
    SmartPtr<SmartBuffer> header = new SmartBuffer(sizeof(flvHeader));
    u8* data = header->data();
    u32 offset = 0;

    //first build flvheader
    memcpy( data, flvHeader, sizeof(flvHeader) );
    offset += sizeof(flvHeader);
    
    return header;
}

SmartPtr<SmartBuffer> FLVOutput::packageVideoFrame(SmartPtr<SmartBuffer> videoPacket, u32 ts, bool bIsKeyFrame, VideoRect* videoRect)
{
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;

    if( videoRect ) {
        x = videoRect->x;
        y = videoRect->y;
        width = videoRect->width;
        height = videoRect->height;
    }

    //then build video header
    u32 videoHeaderLen = 11;
    u32 videoDataLen = videoPacket->dataLength() + 9;
    SmartPtr<SmartBuffer> videoFrame = new SmartBuffer( videoHeaderLen + videoDataLen + 4 );
    u8* data = videoFrame->data();

    //frame tag
    data[0] = (u8)0x09;
    //frame data length
    data[1] = (u8)((videoDataLen>>16)&0xff);
    data[2] = (u8)((videoDataLen>>8)&0xff);
    data[3] = (u8)(videoDataLen&0xff);
    //frame time stamp
    data[4] = (u8)((ts>>16)&0xff);
    data[5] = (u8)((ts>>8)&0xff);
    data[6] = (u8)(ts&0xff);
    //frame time stamp extend
    data[7] = (u8)((ts>>24)&0xff);
    //frame reserved
    data[8] = 0;
    data[9] = 0;
    data[10] = 0;

    if ( bIsKeyFrame) {
        data[11] = (u8)0x18;
    } else {
        data[11] = (u8)0x28;
    }
    //tell the playback side where the stream is located.
    data[12] = (u8)((x>>8)&0xff);  
    data[13] = (u8)(x&0xff);  
    data[14] = (u8)((y>>8)&0xff);  
    data[15] = (u8)(y&0xff);  
    data[16] = (u8)((width>>8)&0xff);  
    data[17] = (u8)(width&0xff);  
    data[18] = (u8)((height>>8)&0xff);  
    data[19] = (u8)(height&0xff);  

    if ( videoDataLen > 1 ) {
        memcpy(&data[20], videoPacket->data(), videoPacket->dataLength());
    }
    //prev tag size
    int tl = 11 + videoDataLen;
    data[tl] = (u8)((tl>>24)&0xff);
    data[tl+1] = (u8)((tl>>16)&0xff);  
    data[tl+2] = (u8)((tl>>8)&0xff);  
    data[tl+3] = (u8)(tl&0xff);  

    //fprintf( stderr, "====>video frame len=%d, videoDataLen=%d ts=%d\n", tl, videoDataLen, ts);

    if( !flvHeaderSent_ ) {
        SmartPtr<SmartBuffer> header = newHeader();
        int totalLen = header->dataLength() + videoFrame->dataLength();
        
        SmartPtr<SmartBuffer> result = new SmartBuffer(totalLen);
        u8* data = result->data();
        memcpy(data, header->data(), header->dataLength());
        memcpy(data + header->dataLength(), videoFrame->data(), videoFrame->dataLength());

        flvHeaderSent_  = true;
        return result;
    } else {
        return videoFrame;
    }
}

SmartPtr<SmartBuffer> FLVOutput::packageAudioFrame(SmartPtr<SmartBuffer> audioPacket, u32 ts)
{
    u32 audioHeaderLen = 11;
    u32 audioDataLen = audioPacket->dataLength()+1;
    SmartPtr<SmartBuffer> audioFrame = new SmartBuffer(audioHeaderLen + audioDataLen + 4);
    u8* data = audioFrame->data();

    //FLV_TAG_TYPE_AUDIO
    //{
    data[0] = (u8)0x08;
    //frame data size
    data[1] = (u8)((audioDataLen>>16)&0xff); 
    data[2] = (u8)((audioDataLen>>8)&0xff);  
    data[3] = (u8)(audioDataLen&0xff); 
    //frame timestatmp

    data[4] = (u8)((ts>>16)&0xff);
    data[5] = (u8)((ts>>8)&0xff);
    data[6] = (u8)(ts&0xff);
    //frame time stamp extend
    data[7] = (u8)((ts>>24)&0xff);

    //StreamID
    data[8] = 0;
    data[9] = 0;
    data[10] = 0;
    //frame data begin
    //{
    //
    data[11] = audioTagByte;

    if( audioDataLen > 1 ) {
        memcpy(&data[12], audioPacket->data(), audioPacket->dataLength());
    }
    //prev tag size
    int tl = 11 + audioDataLen;
    data[tl] = (u8)((tl>>24)&0xff);
    data[tl+1] = (u8)((tl>>16)&0xff);  
    data[tl+2] = (u8)((tl>>8)&0xff);    
    data[tl+3] = (u8)(tl&0xff);

    //fprintf( stderr, "====>audio frame len=%d, audioDataLen=%d ts=%d\n", tl, audioDataLen, ts);
    if( !flvHeaderSent_ ) {
        SmartPtr<SmartBuffer> header = newHeader();
        int totalLen = header->dataLength() + audioFrame->dataLength();
        
        SmartPtr<SmartBuffer> result = new SmartBuffer(totalLen);
        u8* data = result->data();
        memcpy(data, header->data(), header->dataLength());
        memcpy(data + header->dataLength(), audioFrame->data(), audioFrame->dataLength());

        flvHeaderSent_  = true;
        return result;
    } else {
        return audioFrame;
    }
}
