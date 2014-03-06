#include "VideoMixer.h"

//do the mixing, for now, always mix n speex streams into 1 speex stream
SmartPtr<SmartBuffer> mixStream(SmartPtr<SmartBuffer> buffer[], 
                                VideoStreamSetting settings[], 
                                int totalStreams)
{
    return new SmartBuffer(4, "TODO");
}
