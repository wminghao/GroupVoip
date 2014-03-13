//
//  MixCoder.cpp
//  
//
//  Created by Howard Wang on 2/28/14.
//
//
#include "MixCoder.h"
#include "FLVSegmentParser.h"
#include "FLVOutput.h"
#include "AudioEncoder.h"
#include "AudioDecoder.h"
#include "VideoEncoder.h"
#include "VideoDecoder.h"
#include "AudioMixer.h"
#include "VideoMixer.h"

MixCoder::MixCoder(int vBitrate, int width, int height, 
                   int aBitrate, int frequency) : vBitrate_(vBitrate),
                                                  vWidth_(width),
                                                  vHeight_(height),
                                                  aBitrate_(aBitrate),
                                                  aFrequency_(frequency),
                                                  flvSegParser_(NULL),
                                                  flvOutput_(NULL),
                                                  audioEncoder_(NULL),
                                                  videoEncoder_(NULL),
                                                  audioMixer_(NULL),
                                                  videoMixer_(NULL)
{
    memset(audioDecoder_, 0, sizeof(AudioDecoder*) * MAX_XCODING_INSTANCES); 
    memset(videoDecoder_, 0, sizeof(AudioDecoder*) * MAX_XCODING_INSTANCES); 
    flvSegParser_ = new FLVSegmentParser( 30 ); //end result 30 fps
    
    VideoStreamSetting vOutputSetting = { kVP8VideoPacket, vWidth_, vHeight_ }; 
    AudioStreamSetting aOutputSetting = { kMP3, kSndMono, getAudioRate(aBitrate_), kSnd16Bit, 0 };
    flvOutput_ = new FLVOutput( &vOutputSetting, &aOutputSetting );
    
    AudioStreamSetting aInputSetting = { kSpeex, kSndMono, getAudioRate(16000), kSnd16Bit, 0 };
    audioEncoder_ = new AudioEncoder( &aInputSetting, &aOutputSetting, aBitrate_ );
    videoEncoder_ = new VideoEncoder( &vOutputSetting, vBitrate_ );

    audioMixer_ = new AudioMixer();
    videoMixer_ = new VideoMixer();

    //decoders are instantiated on demand
}

MixCoder::~MixCoder() {
    delete flvSegParser_;
    delete flvOutput_;
    for( int i = 0; i < MAX_XCODING_INSTANCES; i ++ ) {
        delete audioDecoder_[i];
        delete videoDecoder_[i];
    }
    delete audioEncoder_;
    delete videoEncoder_;
    delete audioMixer_;
    delete videoMixer_;
}

/* returns false if we hit some bad data, true if OK */
bool MixCoder::newInput( SmartPtr<SmartBuffer> inputBuf )
{
    return (flvSegParser_->readData( inputBuf ) > 0);
}

//read output from the system
SmartPtr<SmartBuffer> MixCoder::getOutput()
{
    //TODO
    return NULL;
}
    
//at the end. flush the input
void MixCoder::flush()
{
    //TODO
}
