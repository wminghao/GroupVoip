//
//  MixCoder.cpp
//  
//
//  Created by Howard Wang on 2/28/14.
//
//
#include "MixCoder.h"

bool MixCoder::initialize()
{
    return false;
}

/* returns false if we hit some badness, true if OK */
bool MixCoder::newInput( SmartPtr<SmartBuffer> )
{
    return false;
}

//read output from the system
SmartPtr<SmartBuffer> MixCoder::getOutput()
{
    return new SmartBuffer("TODO", 4);
}
    
//at the end. flush the input
void MixCoder::flush()
{
}
