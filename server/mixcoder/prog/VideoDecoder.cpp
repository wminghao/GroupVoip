#include "VideoDecoder.h"
            
void VideoDecoder::newAccessUnit( SmartPtr<AccessUnit> )
{
}

void VideoDecoder::newAVCSeqHeader( SmartPtr<AccessUnit> )
{
}
SmartPtr<SmartBuffer> VideoDecoder::getDecodedResult()
{
    return new SmartBuffer(4, "TODO");
}
