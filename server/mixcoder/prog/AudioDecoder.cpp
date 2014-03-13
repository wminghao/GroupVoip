#include "AudioDecoder.h"

SmartPtr<SmartBuffer> AudioDecoder::newAccessUnit( SmartPtr<AccessUnit> )
{
    return new SmartBuffer(4, "TODO");
}

