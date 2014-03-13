#include "FLVOutput.h"

void FLVOutput::newHeader(SmartPtr<SmartBuffer> videoHeader, SmartPtr<SmartBuffer> audioHeader)
{
}

SmartPtr<SmartBuffer> FLVOutput::packageVideoFrame(SmartPtr<SmartBuffer> videoPacket, u32 ts)
{
    return new SmartBuffer(4, "TODO");
}

SmartPtr<SmartBuffer> FLVOutput::packageAudioFrame(SmartPtr<SmartBuffer> audioPacket, u32 ts)
{
    return new SmartBuffer(4, "TODO");
}
