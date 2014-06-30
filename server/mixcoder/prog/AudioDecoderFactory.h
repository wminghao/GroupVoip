#ifndef __AUDIODECODERFACTORY_H
#define __AUDIODECODERFACTORY_H

#include "AudioSpeexDecoder.h"
//#include "AudioFfmpegDecoder.h"

class AudioDecoderFactory
{
 public:
    static AudioDecoder* CreateAudioDecoder(SmartPtr<AccessUnit> au, int streamId);
};
#endif
