//
//  MixCoder.h
//  
//
//  Created by Howard Wang on 2/28/2014.
//
//

#ifndef __MIXCODERCOMMON_H__
#define __MIXCODERCOMMON_H__

#include "fwk/SmartBuffer.h"
#include "CodecInfo.h"
#include "RawData.h"

using namespace std;

class FLVSegmentParser;
class FLVSegmentOutput;
class AudioEncoder;
class VideoEncoder;
class AudioDecoder;
class VideoDecoder;
class AudioMixer;
class VideoMixer;

class MixCoder
{
 public:
    MixCoder(int vBitrate, int width, int height,
             int aBitrate, int frequency);
    ~MixCoder();
    
    /* returns false if we hit some badness, true if OK */
    bool newInput( SmartPtr<SmartBuffer> );

    //read output from the system
    SmartPtr<SmartBuffer> getOutput();
    
    //at the end. flush the input
    void flush();

 private:
    //audio uses speex or mp3
    bool bUseSpeex_;

    //output settings
    int vBitrate_;
    int vWidth_;
    int vHeight_;
    
    int aBitrate_;
    int aFrequency_;

    //input
    FLVSegmentParser* flvSegParser_;
    //output
    FLVSegmentOutput* flvSegOutput_;
    
    //encoders
    AudioEncoder* audioEncoder_[ MAX_XCODING_INSTANCES+1 ];;
    VideoEncoder* videoEncoder_;

    //mixer
    AudioMixer* audioMixer_[ MAX_XCODING_INSTANCES+1 ]; //need max+1 audio mixer since the mobile viewer should not hear his own voice from the mixing
    VideoMixer* videoMixer_;

    //TODO needs to reset those buffers when a stream ends

    //raw video frame in case it does not exist, yuv 3 planes
    SmartPtr<VideoRawData> rawVideoData_[MAX_XCODING_INSTANCES];

    //raw audio frame in case data is late to arrive
    SmartPtr<AudioRawData> rawAudioData_[MAX_XCODING_INSTANCES];
};

#endif
