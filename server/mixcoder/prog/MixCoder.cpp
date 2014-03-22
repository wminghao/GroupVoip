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
    flvSegParser_ = new FLVSegmentParser( 30 ); //end result 30 fps
    
    VideoStreamSetting vOutputSetting = { kVP8VideoPacket, vWidth_, vHeight_ }; 
    AudioStreamSetting aOutputSetting = { kMP3, kSndMono, getAudioRate(aBitrate_), kSnd16Bit, 0 };
    
    AudioStreamSetting aInputSetting = { kSpeex, kSndMono, getAudioRate(16000), kSnd16Bit, 0 };
    audioEncoder_ = new AudioEncoder( &aInputSetting, &aOutputSetting, aBitrate_ );
    videoEncoder_ = new VideoEncoder( &vOutputSetting, vBitrate_ );

    audioMixer_ = new AudioMixer();
    videoMixer_ = new VideoMixer(&vOutputSetting);

    for( int i = 0; i < MAX_XCODING_INSTANCES; i++ ) {
        audioDecoder_[i] = new AudioDecoder();
        videoDecoder_[i] = new VideoDecoder();
    }
    flvOutput_ = new FLVOutput( &vOutputSetting, &aOutputSetting );
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
    StreamType curStreamType = kUnknownStreamType;
    u32 videoPts = 0;
    bool bIsVideoAvailable = flvSegParser_->isNextStreamAvailable( kVideoStreamType, videoPts );

    u32 audioPts = 0;
    bool bIsAudioAvailable = flvSegParser_->isNextStreamAvailable( kAudioStreamType, audioPts );

    if( bIsVideoAvailable && bIsAudioAvailable) {
        if ( audioPts < videoPts ) {
            curStreamType = kAudioStreamType;
        } else {
            curStreamType = kVideoStreamType;
        }
    } else if ( bIsAudioAvailable ) {
        curStreamType = kAudioStreamType;
    } else if( bIsVideoAvailable ) {
        curStreamType = kVideoStreamType;
    }

    SmartPtr<SmartBuffer> resultFlvPacket = NULL;
    if ( curStreamType != kUnknownStreamType ) {
        //fprintf( stderr, "------curStreamType=%d, audioPts=%d, videoPts=%d\n", curStreamType, audioPts, videoPts );
    
        //TODO what's the difference here
        int totalStreams = 0;
        int totalNewStreams = 0;
        for( int i = 0; i < MAX_XCODING_INSTANCES; i ++ ) {
            bool bIsStreamStarted = flvSegParser_->isStreamOnlineStarted(curStreamType, i );
            bool bIsSpsPps = false;
            if( bIsStreamStarted ) {
                SmartPtr<AccessUnit> au = flvSegParser_->getNextFLVFrame(i, curStreamType);
                if ( au ) {
                    if ( curStreamType == kVideoStreamType ) {
                        bIsSpsPps = videoDecoder_[i]->newAccessUnit(au, rawVideoPlanes_[i], rawVideoStrides_[i], &rawVideoSettings_[i]); 
                    } else {
                        rawAudioFrame_[i] = audioDecoder_[i]->newAccessUnit(au, &rawAudioSettings_[i]);
                    }
                    totalNewStreams++;
                } else {
                    //TODO not ready?
                }
                totalStreams++;
            }
            if ( curStreamType == kVideoStreamType ) {
                rawVideoSettings_[i].bIsValid = bIsStreamStarted && !bIsSpsPps;
            } else {
                rawAudioSettings_[i].bIsValid = bIsStreamStarted && !bIsSpsPps;
            }
        }

        if ( totalNewStreams > 0 ) {
            if ( curStreamType == kVideoStreamType ) {
                bool bIsKeyFrame = false;
                SmartPtr<SmartBuffer> rawFrameMixed = videoMixer_->mixStreams(rawVideoPlanes_, rawVideoStrides_, rawVideoSettings_, totalStreams);
                SmartPtr<SmartBuffer> encodedFrame = videoEncoder_->encodeAFrame(rawFrameMixed, &bIsKeyFrame);
                if ( encodedFrame ) {
                    resultFlvPacket = flvOutput_->packageVideoFrame(encodedFrame, videoPts, bIsKeyFrame);
                }
            } else {
                SmartPtr<SmartBuffer> rawFrameMixed = audioMixer_->mixStreams(rawAudioFrame_, rawAudioSettings_, totalStreams);
                SmartPtr<SmartBuffer> encodedFrame = audioEncoder_->encodeAFrame(rawFrameMixed);
                if ( encodedFrame ) {
                    resultFlvPacket = flvOutput_->packageAudioFrame(encodedFrame, audioPts);
                }
            }
        }
    }
    return resultFlvPacket;
}

SmartPtr<SmartBuffer> MixCoder::newHeader()
{
    return flvOutput_->newHeader();
}    
//at the end. flush the input
void MixCoder::flush()
{
    //TODO
}
