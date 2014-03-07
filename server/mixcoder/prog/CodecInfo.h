
#ifndef __CODECINFOCOMMON_H__
#define __CODECINFOCOMMON_H__

#define MAX_XCODING_INSTANCES = 4;

enum specialProperty {
    kRawData, //nalu
    kSpsPps, //avc header
    kEndSeq, //avc end of sequence
    kSeqHeader, //aac header
};

typedef enum StreamType {
    kUnknownStreamType = 0,
    kVideoStreamType = 8,
    kAudioStreamType = 9,
    kDataStreamType = 18
}StreamType;

//video types
typedef enum
{
    kKeyFrame,
    kInterFrame,
    kDisposableFrame,
    kGenKeyFrame,
    kVideoInfoFrame
}VideoFrameType;

typedef enum 
{
    kJpegVideopacket = 1,
    kH263VideoPacket = 2,
    kScreenVideoPacket = 3,
    kVp6VideoPacket = 4,
    kVp6AlphaVideoPacket = 5,
    kScreenV2VideoPacket = 6,
    kAVCVideoPacket = 7,
    kVp8VideoPacket = 15
}VideoCodecId;

typedef enum
{
    kAVCSeqHeader,
    kAVCNalu,
    kAVCEndOfSeq
}AVCPacketType;

//audio types
typedef enum
{
    kLinearPCM = 0,
    kADPCM = 1,
    kMP3 = 2,
    kLinearPCMLittleEndian = 3,// little endian
    kNellymoser16 = 4,// 16-kHz mono
    kNellymoser8 = 5, // 8-kHz mono
    kNellymoser = 6,
    kG711ALaw = 7,// A-law logarithmic PCM
    kG711muLaw= 8,// mu-law logarithmic PCM
    kreserved = 9,
    kAAC = 10,
    kSpeex = 11,
    kMP38kHz = 14,
    kDeviceSpecific = 15 //Device-specific sound
}AudioCodecId;

typedef enum
{
    k5Dot5kHz = 0,
    k11kHz = 1,
    k22kHz = 2,
    k44kHz = 3
}AudioRate;

typedef enum
{
    kSnd8Bit,
    kSnd16Bit
}AudioSize;

typedef enum
{
    kSndMono,
    kSndStereo
}AudioType;

typedef enum
{
    kAACSeqHeader,
    kAACRaw
}AACPacketType;

typedef struct AudioStreamSetting
{
    AudioCodecId acid;
    AudioType at;
    AudioRate ar;
    AudioSize as;
    int ap; //aac or something else, audio property
}AudioStreamSetting;

typedef struct VideoStreamSetting
{
    VideoCodecId vcid;
    //always yv12 format, y plane + u plane + v plane in one buffer
    int width;
    int height;
}VideoStreamSetting;

#endif
