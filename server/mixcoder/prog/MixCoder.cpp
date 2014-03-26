//
//  MixCoder.cpp
//  
//
//  Created by Howard Wang on 2/28/14.
//
//
#include "MixCoder.h"
#include "FLVSegmentParser.h"
#include "FLVSegmentOutput.h"
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
                                                  aFrequency_(frequency)
{
    flvSegParser_ = new FLVSegmentParser( 30 ); //end result 30 fps
    
    VideoStreamSetting vOutputSetting = { kVP8VideoPacket, vWidth_, vHeight_ }; 
    AudioStreamSetting aOutputSetting = { kMP3, kSndMono, getAudioRate(aBitrate_), kSnd16Bit, 0 };
    AudioStreamSetting aInputSetting = { kSpeex, kSndMono, getAudioRate(16000), kSnd16Bit, 0 };
    videoEncoder_ = new VideoEncoder( &vOutputSetting, vBitrate_ );
    videoMixer_ = new VideoMixer(&vOutputSetting);

    for( u32 i = 0; i < MAX_XCODING_INSTANCES+1; i++ ) {
        audioEncoder_[i] = new AudioEncoder( &aInputSetting, &aOutputSetting, aBitrate_ );
        audioMixer_[i] = new AudioMixer();
    }
    for( u32 i = 0; i < MAX_XCODING_INSTANCES; i++ ) {
        audioDecoder_[i] = new AudioDecoder();
        videoDecoder_[i] = new VideoDecoder();
    }
    flvSegOutput_ = new FLVSegmentOutput( &vOutputSetting, &aOutputSetting );
}

MixCoder::~MixCoder() {
    delete flvSegParser_;
    delete flvSegOutput_;
    for( u32 i = 0; i < MAX_XCODING_INSTANCES; i ++ ) {
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
    
        int totalStreams = 0;
        int totalMobileStreams = 0;
        //fprintf( stderr, "------begin iteration\n" );
        if ( curStreamType == kVideoStreamType ) {
            for( u32 i = 0; i < MAX_XCODING_INSTANCES; i ++ ) {
                bool bIsStreamStarted = flvSegParser_->isStreamOnlineStarted(curStreamType, i );
                bool bIsValidFrame = false;
                if( bIsStreamStarted ) {
                    SmartPtr<AccessUnit> au = flvSegParser_->getNextFLVFrame(i, curStreamType);
                    if ( au ) {
                        bIsValidFrame = videoDecoder_[i]->newAccessUnit(au, rawVideoPlanes_[i], rawVideoStrides_[i], &rawVideoSettings_[i]); 
                        //fprintf( stderr, "------i = %d. bIsValidFrame=%d\n", i, bIsValidFrame );
                    } else {
                        //if no frame generated, and never has any frames generated before, do nothing, 
                        //else use the cached video frame 
                        if( videoDecoder_[i]->hasFirstFrameDecoded()) {
                            bIsValidFrame = true; //use the cached frame
                        }
                    }
                }
                rawVideoSettings_[i].ss= flvSegParser_->getStreamSource(i);
                rawVideoSettings_[i].bIsValid = bIsStreamStarted && bIsValidFrame;
                if( rawVideoSettings_[i].bIsValid ) {
                    totalStreams++;
                    if( kMobileStreamSource == rawVideoSettings_[i].ss ) {
                        totalMobileStreams++;
                    }
                }
            }

            if ( totalStreams > 0 ) {
                bool bIsKeyFrame = false;
                fprintf( stderr, "------totalVideoStreams = %d\n", totalStreams );
                VideoRect videoRect[MAX_XCODING_INSTANCES];
                SmartPtr<SmartBuffer> rawFrameMixed = videoMixer_->mixStreams(rawVideoPlanes_, rawVideoStrides_, rawVideoSettings_, totalStreams, videoRect);
                SmartPtr<SmartBuffer> encodedFrame = videoEncoder_->encodeAFrame(rawFrameMixed, &bIsKeyFrame);
                if ( encodedFrame ) {
                    //for each individual mobile stream
                    if ( totalMobileStreams ) { //for non-mobile stream, there is nothing to mix
                        for( u32 i = 0; i < MAX_XCODING_INSTANCES; i ++ ) {
                            if( rawVideoSettings_[i].bIsValid && kMobileStreamSource == rawVideoSettings_[i].ss) {
                                flvSegOutput_->packageVideoFrame(encodedFrame, videoPts, bIsKeyFrame, i, &videoRect[i]);
                            }
                        }
                    }
                    //for the all-in stream
                    flvSegOutput_->packageVideoFrame(encodedFrame, videoPts, bIsKeyFrame, MAX_XCODING_INSTANCES, NULL);
                    resultFlvPacket = flvSegOutput_->getOneFrameForAllStreams();
                }
            } 
        } else {
            for( u32 i = 0; i < MAX_XCODING_INSTANCES; i ++ ) {
                bool bIsStreamStarted = flvSegParser_->isStreamOnlineStarted(curStreamType, i );
                bool bIsValidFrame = false;
                if( bIsStreamStarted ) {
                    SmartPtr<AccessUnit> au = flvSegParser_->getNextFLVFrame(i, curStreamType);
                    if ( au ) {
                        rawAudioFrame_[i] = audioDecoder_[i]->newAccessUnit(au, &rawAudioSettings_[i]);
                        bIsValidFrame = true;
                    } else {
                        //if no frame generated, and never has any frames generated before, do nothing, 
                        //else use the cached audio frame 
                        if( audioDecoder_[i]->hasFirstFrameDecoded()) {
                            bIsValidFrame = true; //use the cached frame
                        } 
                    }
                }
                rawAudioSettings_[i].ss = flvSegParser_->getStreamSource(i);
                rawAudioSettings_[i].bIsValid = bIsStreamStarted && bIsValidFrame;
                if( rawAudioSettings_[i].bIsValid ) {
                    totalStreams++;
                    if( kMobileStreamSource == rawVideoSettings_[i].ss ) {
                        totalMobileStreams++;
                    }
                }
            }

            if ( totalStreams > 0 ) {
                fprintf( stderr, "------totalAudioStreams = %d\n", totalStreams );
                //for each individual mobile stream
                if ( totalMobileStreams ) { //for non-mobile stream, there is nothing to mix
                    for( u32 i = 0; i < MAX_XCODING_INSTANCES; i ++ ) {
                        if( rawVideoSettings_[i].bIsValid && kMobileStreamSource == rawVideoSettings_[i].ss) {
                            SmartPtr<SmartBuffer> rawFrameMixed = audioMixer_[i]->mixStreams(rawAudioFrame_, rawAudioSettings_, totalStreams, i);
                            SmartPtr<SmartBuffer> encodedFrame = audioEncoder_[i]->encodeAFrame(rawFrameMixed);
                            if ( encodedFrame ) {
                                flvSegOutput_->packageAudioFrame(encodedFrame, audioPts, i);
                            }
                        }
                    }
                }
                //for all in stream
                SmartPtr<SmartBuffer> rawFrameMixed = audioMixer_[MAX_XCODING_INSTANCES]->mixStreams(rawAudioFrame_, rawAudioSettings_, totalStreams, 0xffffffff);
                SmartPtr<SmartBuffer> encodedFrame = audioEncoder_[MAX_XCODING_INSTANCES]->encodeAFrame(rawFrameMixed);
                if ( encodedFrame ) {
                    flvSegOutput_->packageAudioFrame(encodedFrame, audioPts, MAX_XCODING_INSTANCES);
                }
                resultFlvPacket = flvSegOutput_->getOneFrameForAllStreams();
            }
        }
    }
    return resultFlvPacket;
}

//at the end. flush the input
void MixCoder::flush()
{
    //TODO
}
