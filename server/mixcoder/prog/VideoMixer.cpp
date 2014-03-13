#include "VideoMixer.h"

//do the mixing, for now, always mix n raw streams into 1 rawstream
SmartPtr<SmartBuffer> VideoMixer::mixStreams(SmartPtr<SmartBuffer>* buffer, 
                                             VideoStreamSetting* settings, 
                                             int totalStreams)
{
    return new SmartBuffer(4, "TODO");
}
