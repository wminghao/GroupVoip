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
#include "AudioSpeexEncoder.h"
#include "AudioMp3Encoder.h"
#include "AudioDecoder.h"
#include "AudioDecoderFactory.h"
#include "VideoEncoder.h"
#include "VideoDecoder.h"
#include "AudioMixer.h"
#include "VideoMixer.h"
#include "fwk/log.h"
#include <assert.h>

MixCoder::MixCoder(int vBitrate, int width, int height, 
                   int aBitrate, int frequency) : bUseSpeex_(false),
                                                  vBitrate_(vBitrate),
                                                  vWidth_(width),
                                                  vHeight_(height),
                                                  aBitrate_(aBitrate),
                                                  aFrequency_(frequency)
{
    flvSegParser_ = new FLVSegmentParser( 30 ); //end result 30 fps
    
    VideoStreamSetting vOutputSetting = { kVP8VideoPacket, vWidth_, vHeight_ }; 
    AudioStreamSetting aOutputSetting = { kMP316kHz, getAudioRate(16000), kSndMono, kSnd16Bit, 0 };
                                         
    videoEncoder_ = new VideoEncoder( &vOutputSetting, vBitrate_ );
    videoMixer_ = new VideoMixer(&vOutputSetting);

    if( bUseSpeex_ ) {
        aOutputSetting.acid = kSpeex;
    } //otherwise, it's 16kHz mp3 audio(customized format)

    for( u32 i = 0; i < MAX_XCODING_INSTANCES+1; i++ ) {
        if( bUseSpeex_ ) {
            audioEncoder_[i] = new AudioSpeexEncoder( &aOutputSetting, aBitrate_ );
        } else {
            //TODO aOutputSettings can be changed
            audioEncoder_[i] = new AudioMp3Encoder( &aOutputSetting, aBitrate_ );
        }
        //save the raw audio Settings_
        memcpy( &rawAudioSettings_[i], &aOutputSetting, sizeof(AudioStreamSetting) );
        audioMixer_[i] = new AudioMixer();
    }
    for( u32 i = 0; i < MAX_XCODING_INSTANCES; i++ ) {
        audioDecoder_[i] = NULL; //initialize it later
        videoDecoder_[i] = new VideoDecoder(i);
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
    for( u32 i = 0; i < MAX_XCODING_INSTANCES+1; i ++ ) {
        delete audioEncoder_[i];
        delete audioMixer_[i];
    }
    delete videoEncoder_;
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
    u32 audioPts = 0;
    bool bIsAudioReady = flvSegParser_->isNextAudioStreamReady( audioPts );

    u32 videoPts = 0;
    bool bIsVideoReady = bIsAudioReady?flvSegParser_->isNextVideoStreamReady( videoPts, audioPts ):false;

    if( bIsVideoReady && bIsAudioReady) {
        //audio and video are both ready.
        if ( audioPts < videoPts ) {
            curStreamType = kAudioStreamType;
        } else {
            curStreamType = kVideoStreamType;
        }
    } else if ( bIsAudioReady ) {
        //audio ready, video not ready, continue
        curStreamType = kAudioStreamType;
    } else if( bIsVideoReady ) {
        //for video ready, audio not ready case, do not pursue, since audio is always continuous.
        //curStreamType = kVideoStreamType;
        assert(0);
    }

    SmartPtr<SmartBuffer> resultFlvPacket = NULL;
    if ( curStreamType != kUnknownStreamType ) {
        //LOG("------curStreamType=%d, audioPts=%d, videoPts=%d, bIsVideoReady=%d, bIsAudioReady=%d\n", curStreamType, audioPts, videoPts, bIsVideoReady, bIsAudioReady );
        int totalStreams = 0;
        int totalMobileStreams = 0;
        if ( curStreamType == kVideoStreamType ) {
            for( u32 i = 0; i < MAX_XCODING_INSTANCES; i ++ ) {
                bool bIsStreamStarted = flvSegParser_->isStreamOnlineStarted(curStreamType, i );
                bool bIsValidFrame = false;
                if( bIsStreamStarted ) {
                    SmartPtr<AccessUnit> au;
                    do {
                        au = flvSegParser_->getNextVideoFrame(i, videoPts);
                        if ( au ) {
                            videoDecoder_[i]->newAccessUnit(au, rawVideoPlanes_[i], rawVideoStrides_[i], &rawVideoSettings_[i]); 
                        } 
                    } while (au); //pop a few frames with timestamp smaller than videoPts.

                    //if no frame generated, and never has any frames generated before, do nothing, 
                    //else use the cached video frame 
                    if( videoDecoder_[i]->hasFirstFrameDecoded()) {
                        bIsValidFrame = true; //use the cached frame
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
                VideoRect videoRect[MAX_XCODING_INSTANCES];
                SmartPtr<SmartBuffer> rawFrameMixed = videoMixer_->mixStreams(rawVideoPlanes_, rawVideoStrides_, rawVideoSettings_, totalStreams, videoRect);
                SmartPtr<SmartBuffer> encodedFrame = videoEncoder_->encodeAFrame(rawFrameMixed, &bIsKeyFrame);
                if ( encodedFrame ) {
                    //for each individual mobile stream
                    if ( totalMobileStreams ) { 
                        //for non-mobile stream, there is nothing to mix
                        for( u32 i = 0; i < MAX_XCODING_INSTANCES; i ++ ) {
                            if( rawVideoSettings_[i].bIsValid && kMobileStreamSource == rawVideoSettings_[i].ss) {
                                //LOG("------totalVideoStreams = %d, totalMobileStreams=%d\n", totalStreams, totalMobileStreams );
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
                    SmartPtr<AccessUnit> au = flvSegParser_->getNextAudioFrame(i);
                    if ( au ) {
                        if(!audioDecoder_[i]) {
                            audioDecoder_[i] = AudioDecoderFactory::CreateAudioDecoder(au, i);
                        }
                        rawAudioFrame_[i] = audioDecoder_[i]->newAccessUnit(au, &rawAudioSettings_[i]);
                        bIsValidFrame = true;
                    } else {
                        //if no frame generated, and never has any frames generated before, do nothing, 
                        //else use the cached audio frame 
                        if( audioDecoder_[i] && audioDecoder_[i]->hasFirstFrameDecoded()) {
                            bIsValidFrame = true; //use the cached frame
                        } 
                    }
                }
                rawAudioSettings_[i].ss = flvSegParser_->getStreamSource(i);
                rawAudioSettings_[i].bIsValid = bIsStreamStarted && bIsValidFrame;
                if( rawAudioSettings_[i].bIsValid ) {
                    totalStreams++;
                    if( kMobileStreamSource == rawAudioSettings_[i].ss ) {
                        totalMobileStreams++;
                    }
                }
            }

            if ( totalStreams > 0 ) {
                //LOG("------totalAudioStreams = %d, totalMobileStreams=%d\n", totalStreams, totalMobileStreams );
                //for each individual mobile stream
                if ( totalMobileStreams ) { 
                    //for non-mobile stream, there is nothing to mix
                    for( u32 i = 0; i < MAX_XCODING_INSTANCES; i ++ ) {
                        if( rawAudioSettings_[i].bIsValid && kMobileStreamSource == rawAudioSettings_[i].ss) {
                            //LOG("----------------------------totalAudioStreams = %d, mobileStream index=%d\n", totalStreams, i );
                            SmartPtr<SmartBuffer> rawFrameMixed = audioMixer_[i]->mixStreams(rawAudioFrame_, rawAudioSettings_, MP3_FRAME_SAMPLE_SIZE, totalStreams, i);
                            SmartPtr<SmartBuffer> encodedFrame = audioEncoder_[i]->encodeAFrame(rawFrameMixed);
                            if ( encodedFrame ) {
                                flvSegOutput_->packageAudioFrame(encodedFrame, audioPts, i);
                            }
                        }
                    }
                }
                //for all in stream
                SmartPtr<SmartBuffer> rawFrameMixed = audioMixer_[MAX_XCODING_INSTANCES]->mixStreams(rawAudioFrame_, rawAudioSettings_, MP3_FRAME_SAMPLE_SIZE, totalStreams, 0xffffffff);
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
