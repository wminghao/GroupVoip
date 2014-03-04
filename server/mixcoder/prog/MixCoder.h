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

class MixCoder: public MediaTarget
{
 public:
    MixCoder(int vBitrate, int width, 
             int aBitrate, int frequency) : vBitrate_(vBitrate),
        vWidth_(width),
        aBitrate_(aBitrate),
        aFrequency_(frequency),
        flvSegParser_(NULL) 
        flvOutput_(NULL),
        audioMixer_(NULL),
        videoMixer_(NULL),
        audioEncoder_(NULL),
        videoEncoder_(NULL)
            {
                memset(audioDecoder_, 0, sizeof(AudioDecoder*) * MAX_XCODING_INSTANCES); 
                memset(videoDecoder_, 0, sizeof(AudioDecoder*) * MAX_XCODING_INSTANCES); 
            }
    ~MixCoder() {
        delete flvSegParser_
        delete flvOutput_;
        delete [] audioDecoder_;
        delete [] videoDecoder_;
        delete audioEncoder_;
        delete videoEncoder_;
        delete audioMixer_;
        delete videoMixer_;
    }
    
    bool initialize();

    /* returns false if we hit some badness, true if OK */
    bool newInput( Ptr<SmartBuffer> );

    //read output from the system
    Ptr<SmartBuffer> getOutput();
    
    //at the end. flush the input
    void flush();

 private:
    virtual void newAccessUnit( SmartPtr<AccessUnit> ) {};

    virtual void newAVCSeqHeader( SmartPtr<SmartBuffer> ) {}
    virtual void newAudioHeader( SmartPtr<SmartBuffer> ) {}

 private:

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

    //output settings
    int vBitrate_;
    int vWidth_;
    
    int aFrequency_;
    int aBitrate_;
};

#endif
