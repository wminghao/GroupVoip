#ifndef __AUDIODECODERFACTORY_H
#define __AUDIODECODERFACTORY_H
#include "fwk/SmartPtr.h"
#include "AccessUnit.h"

class AudioDecoder;

class AudioDecoderFactory
{
 public:
    static AudioDecoder* CreateAudioDecoder(const SmartPtr<AccessUnit> au, int streamId);
    static bool isSameDecoder(const SmartPtr<AccessUnit> au, const AudioStreamSetting* origSetting);
};
#endif
