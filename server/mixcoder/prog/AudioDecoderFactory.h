#ifndef __AUDIODECODERFACTORY_H
#define __AUDIODECODERFACTORY_H
#include "fwk/SmartPtr.h"
#include "MediaTarget.h"

class AudioDecoder;

class AudioDecoderFactory
{
 public:
    static AudioDecoder* CreateAudioDecoder(SmartPtr<AccessUnit> au, int streamId);
};
#endif
