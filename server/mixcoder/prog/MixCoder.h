//
//  MixCoder.h
//  
//
//  Created by Howard Wang on 2/28/2014.
//
//
extern "C" {
#include <libavcodec/avcodec.h>    // required headers
#include <libavformat/avformat.h>
}

#include "fwk/SmartBuffer.h"
#include "MediaTarget.h"

#define MAX_XCODING_INSTANCES = 4;

using namespace std;

class FLVSegmentParser;
class FLVOutput;
class AudioXcoder;
class VideoXcoder;
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
        videoMixer_(NULL)
            {
                memset(audioXcoder_, 0, sizeof(AudioXcoder*) * MAX_XCODING_INSTANCES); 
                memset(videoXcoder_, 0, sizeof(AudioXcoder*) * MAX_XCODING_INSTANCES); 
            }
    ~MixCoder() {
        delete flvSegParser_
        delete flvOutput_;
        delete [] audioXcoder_;
        delete [] videoXcoder_;
        delete audioMixer_;
        delete videoMixer_;
    }

    
    /* returns false if we hit some badness, true if OK */
    bool newInput( Ptr<Buffer> );

    //read output from the system
    Ptr<Buffer> getOutput();
    
    //at the end. flush the input
    void flush();

 private:
    FLVSegmentParser* flvSegParser_;
    FLVOutput* flvOutput_;
    
    AudioXcoder* audioXcoder_[ MAX_XCODING_INSTANCES ];
    VideoXcoder* videoXcoder_[ MAX_XCODING_INSTANCES ];
    
    AudioMixer* audioMixer_;
    VideoMixer* videoMixer_;

    int vBitrate_;
    int vWidth_;
    
    int aFrequency_;
    int aBitrate_;
};

