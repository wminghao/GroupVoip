//
//  MixCoder.h
//  
//
//  Created by Howard Wang on 2/28/2014.
//
//

#ifndef __MIXCODERCOMMON_H__
#define __MIXCODERCOMMON_H__

extern "C" {
#include <libavcodec/avcodec.h>    // required headers
#include <libavformat/avformat.h>
}

#include "fwk/SmartBuffer.h"
#include "MediaTarget.h"
#include "CodecInfo.h"

using namespace std;

class FLVSegmentParser;
class FLVOutput;
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
    //output settings
    int vBitrate_;
    int vWidth_;
    int vHeight_;
    
    int aBitrate_;
    int aFrequency_;

    //input
    FLVSegmentParser* flvSegParser_;
    //output
    FLVOutput* flvOutput_;
    
    //decoders
    AudioDecoder* audioDecoder_[ MAX_XCODING_INSTANCES ];
    VideoDecoder* videoDecoder_[ MAX_XCODING_INSTANCES ];
    
    //encoders
    AudioEncoder* audioEncoder_;
    VideoEncoder* videoEncoder_;

    //mixer
    AudioMixer* audioMixer_;
    VideoMixer* videoMixer_;

    //raw video frame in case it does not exist
    SmartPtr<SmartBuffer> rawVideoFrame_[MAX_XCODING_INSTANCES];
};

#endif
