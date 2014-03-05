#include "AudioDecoder.h"

void AudioDecoder::newAccessUnit( SmartPtr<AccessUnit> )
{
}

SmartPtr<SmartBuffer> AudioDecoder::getDecodedResult()
{
    return new SmartBuffer(4, "TODO");
}
