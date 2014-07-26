#include "FLVOutput.h"
#include <stdio.h>
#include "fwk/log.h"

//5th byte 0x01 video, 0x04 audio, 0x05 video+audio
const u8 flvHeader[] = {
    0x46,0x4c,0x56,0x01,0x05, //5 bytes flv + type
    0x00,0x00,0x00,0x09, //length = 9
    0x00,0x00,0x00,0x00, //prev len = 0
    0x12,0x00,0x01,0x23,0x00,0x00,0x00,0x00,0x00,0x00,0x00, //11 bytes data tag, len=0x12E-0x0b(11 bytes)
    0x02,
    0x00,0x0a,0x6f,0x6e,0x4d,0x65,0x74,0x61,0x44,0x61,0x74,0x61, //onMetaData
    0x08,0x00,0x00,0x00,0x0d, //array of 13 elements
    0x00,0x08,0x64,0x75,0x72,0x61,0x74,0x69,0x6f,0x6e, //duration
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, //0.00
    0x00,0x0C,0x61,0x75,0x64,0x69,0x6F,0x63,0x6F,0x64,0x65,0x63,0x69,0x64, //audiocodecid
    0x00,0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00, //speex = 11(0x4026000000000000), 16kmp3 = 15(0x402e000000000000), mp3 = 2(0x4000000000000000)
    0x00,0x0D,0x61,0x75,0x64,0x69,0x6F,0x64,0x61,0x74,0x61,0x72,0x61,0x74,0x65,//audiodatarate
    0x00,0x40,0x3B,0xCC,0xCC,0xCC,0xCC,0xCC,0xCD,
    0x00,0x0F,0x61,0x75,0x64,0x69,0x6F,0x73,0x61,0x6D,0x70,0x6C,0x65,0x72,0x61,0x74,0x65, //audiosamplerate
    0x00,0x40,0xE5,0x88,0x80,0x00,0x00,0x00,0x00, //16k = 0x40CF400000000000, 44.1k = 40e5888000000000
    0x00,0x0F,0x61,0x75,0x64,0x69,0x6F,0x73,0x61,0x6D,0x70,0x6C,0x65,0x73,0x69,0x7A,0x65, //audiosamplesize
    0x00,0x40,0x30,0x00,0x00,0x00,0x00,0x00,0x00, //16bits
    0x00,0x06,0x73,0x74,0x65,0x72,0x65,0x6F, //stereo
    0x01,0x01, //bool, stereo
    0x00,0x0c,0x76,0x69,0x64,0x65,0x6f,0x63,0x6f,0x64,0x65,0x63,0x69,0x64, //videocodecid
    0x00,0x40,0x20,0x00,0x00,0x00,0x00,0x00,0x00,//0x4020000000000000 stands for codecid=8, vp8
    0x00,0x05,0x77,0x69,0x64,0x74,0x68, //width
    0x00,0x40,0x84,0x00,0x00,0x00,0x00,0x00,0x00,// = 640.00
    0x00,0x06,0x68,0x65,0x69,0x67,0x68,0x74, //height
    0x00,0x40,0x7e,0x00,0x00,0x00,0x00,0x00,0x00, // = 480.00
    0x00,0x09,0x66,0x72,0x61,0x6d,0x65,0x72,0x61,0x74,0x65, //framerate
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,//0
    0x00,0x0d,0x76,0x69,0x64,0x65,0x6f,0x64,0x61,0x74,0x61,0x72,0x61,0x74,0x65, //videodatarate ???
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,//0.00
    0x00,0x07,0x65,0x6e,0x63,0x6f,0x64,0x65,0x72, //encoder
    0x02,0x00,0x0b,0x4c,0x61,0x76,0x66,0x35,0x32,0x2e,0x38,0x37,0x2e,0x31,
    0x00,0x08,0x66,0x69,0x6c,0x65,0x73,0x69,0x7a,0x65, //filesize
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, //0.00
    0x00,0x00,0x09, //ends with 9
    0x00,0x00,0x01,0x2E //previous tag len, len=0x12E
};
    

const u8 audioSpeexTagByte = 0xbe; //speex
const u8 audioMp316kTagByte = 0xfe; //16khzmp3
const u8 audioMp3TagByte = 0x2f; //44.1khzmp3

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

    //LOG("====>video frame len=%d, videoDataLen=%d ts=%d\n", tl, videoDataLen, ts);

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
    if( audioSetting_.acid == kMP316kHz ) {
        data[11] = audioMp316kTagByte;
    } else if( audioSetting_.acid == kMP3 ) {
        data[11] = audioMp3TagByte;
    } else {
        data[11] = audioSpeexTagByte;
    }

    if( audioDataLen > 1 ) {
        memcpy(&data[12], audioPacket->data(), audioPacket->dataLength());
    }
    //prev tag size
    int tl = 11 + audioDataLen;
    data[tl] = (u8)((tl>>24)&0xff);
    data[tl+1] = (u8)((tl>>16)&0xff);  
    data[tl+2] = (u8)((tl>>8)&0xff);    
    data[tl+3] = (u8)(tl&0xff);

    //LOG("====>audio frame len=%d, audioDataLen=%d ts=%d\n", tl, audioDataLen, ts);
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
