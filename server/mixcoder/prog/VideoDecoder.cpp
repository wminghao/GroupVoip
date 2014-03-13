#include "VideoDecoder.h"
            
SmartPtr<SmartBuffer> VideoDecoder::newAccessUnit( SmartPtr<AccessUnit> )
{
    //it can be a sps-pps header or regular nalu
    return new SmartBuffer(4, "TODO");
}
