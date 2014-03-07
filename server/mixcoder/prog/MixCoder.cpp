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

MixCoder::~MixCoder() {
    delete flvSegParser_;
    delete flvOutput_;
    delete [] audioDecoder_;
    delete [] videoDecoder_;
    delete audioEncoder_;
    delete videoEncoder_;
    delete audioMixer_;
    delete videoMixer_;
}

/* returns false if we hit some badness, true if OK */
bool MixCoder::newInput( SmartPtr<SmartBuffer> )
{
    return false;
}

//read output from the system
SmartPtr<SmartBuffer> MixCoder::getOutput()
{
    return new SmartBuffer(4, "TODO");
}
    
//at the end. flush the input
void MixCoder::flush()
{
}
