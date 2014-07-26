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
#include "VideoEncoder.h"
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
    VideoStreamSetting vOutputSetting = { kVP8VideoPacket, vWidth_, vHeight_ }; 
    AudioStreamSetting aOutputSetting = { kMP3, getAudioRate(44100), kSndStereo, kSnd16Bit, 0 };

    flvSegParser_ = new FLVSegmentParser( 30, &aOutputSetting ); //end result 30 fps
                                         
    videoEncoder_ = new VideoEncoder( &vOutputSetting, vBitrate_ );
    videoMixer_ = new VideoMixer(&vOutputSetting);

    if( bUseSpeex_ ) {
        aOutputSetting.acid = kSpeex;
    } //otherwise, it's 44.1kHz mp3 audio

    for( u32 i = 0; i < MAX_XCODING_INSTANCES+1; i++ ) {
        if( bUseSpeex_ ) {
            audioEncoder_[i] = new AudioSpeexEncoder( &aOutputSetting, aBitrate_ );
        } else {
            audioEncoder_[i] = new AudioMp3Encoder( &aOutputSetting, aBitrate_ );
        }
        audioMixer_[i] = new AudioMixer();
    }
    flvSegOutput_ = new FLVSegmentOutput( &vOutputSetting, &aOutputSetting );
}

MixCoder::~MixCoder() {
    delete flvSegParser_;
    delete flvSegOutput_;
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
                    SmartPtr<VideoRawData> v;
                    do {
                        v = flvSegParser_->getNextVideoFrame(i, videoPts);
                        if( v ) {
                            rawVideoData_[i] = v;
                        }
                    } while (v); //pop a few frames with timestamp smaller than videoPts.

                    //if no frame generated, and never has any frames generated before, do nothing, 
                    //else use the cached video frame 
                    if( flvSegParser_->hasFirstFrameDecoded(i, true, videoPts)) {
                        bIsValidFrame = true; //use the cached frame
                    }
                }
                if( rawVideoData_[i] ) {
                    rawVideoData_[i]->rawVideoSettings_.ss= flvSegParser_->getStreamSource(i);
                    rawVideoData_[i]->rawVideoSettings_.bIsValid = bIsStreamStarted && bIsValidFrame;
                    if( rawVideoData_[i]->rawVideoSettings_.bIsValid ) {
                        //LOG("------rawVideoData_[%d] is valid\r\n", i);
                        totalStreams++;
                        if( kMobileStreamSource == rawVideoData_[i]->rawVideoSettings_.ss ) {
                            totalMobileStreams++;
                        }
                    }
                }
            }

            if ( totalStreams > 0 ) {
                bool bIsKeyFrame = false;
                VideoRect videoRect[MAX_XCODING_INSTANCES];
                SmartPtr<SmartBuffer> rawFrameMixed = videoMixer_->mixStreams(rawVideoData_, totalStreams, videoRect);
                SmartPtr<SmartBuffer> encodedFrame = videoEncoder_->encodeAFrame(rawFrameMixed, &bIsKeyFrame);
                if ( encodedFrame ) {
                    //for each individual mobile stream
                    if ( totalMobileStreams ) { 
                        //for non-mobile stream, there is nothing to mix
                        for( u32 i = 0; i < MAX_XCODING_INSTANCES; i ++ ) {
                            if( rawVideoData_[i] &&  rawVideoData_[i]->rawVideoSettings_.bIsValid && kMobileStreamSource == rawVideoData_[i]->rawVideoSettings_.ss) {
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
                    SmartPtr<AudioRawData> a = flvSegParser_->getNextAudioFrame(i);
                    if ( a ) {
                        rawAudioData_[i] = a;
                        bIsValidFrame = true;
                    } else {
                        //if no frame generated, and never has any frames generated before, do nothing, 
                        //else use the cached audio frame 
                        if( flvSegParser_->hasFirstFrameDecoded(i, false, a->pts)) {
                            bIsValidFrame = true; //use the cached frame
                        } 
                    }
                }
                if( rawAudioData_[i] ) {
                    rawAudioData_[i]->ss = flvSegParser_->getStreamSource(i);
                    rawAudioData_[i]->bIsValid = bIsStreamStarted && bIsValidFrame;
                    if( rawAudioData_[i]->bIsValid ) {
                        totalStreams++;
                        if( kMobileStreamSource == rawAudioData_[i]->ss ) {
                            totalMobileStreams++;
                        }
                    }
                }
            }

            if ( totalStreams > 0 ) {
                //LOG("------totalAudioStreams = %d, totalMobileStreams=%d\n", totalStreams, totalMobileStreams );
                //for each individual mobile stream
                if ( totalMobileStreams ) { 
                    //for non-mobile stream, there is nothing to mix
                    for( u32 i = 0; i < MAX_XCODING_INSTANCES; i ++ ) {
                        if( rawAudioData_[i] && rawAudioData_[i]->bIsValid && kMobileStreamSource == rawAudioData_[i]->ss) {
                            //LOG("----------------------------totalAudioStreams = %d, mobileStream index=%d\n", totalStreams, i );
                            SmartPtr<SmartBuffer> rawFrameMixed = audioMixer_[i]->mixStreams(rawAudioData_, MP3_FRAME_SAMPLE_SIZE*2, totalStreams, i);
                            SmartPtr<SmartBuffer> encodedFrame = audioEncoder_[i]->encodeAFrame(rawFrameMixed);
                            if ( encodedFrame ) {
                                flvSegOutput_->packageAudioFrame(encodedFrame, audioPts, i);
                            }
                        }
                    }
                }
                //for all in stream
                SmartPtr<SmartBuffer> rawFrameMixed = audioMixer_[MAX_XCODING_INSTANCES]->mixStreams(rawAudioData_, MP3_FRAME_SAMPLE_SIZE*2, totalStreams, 0xffffffff);
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
