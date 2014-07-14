#ifndef __AUDIO_MP3_ENCODER_H__
#define __AUDIO_MP3_ENCODER_H__

#include "AudioEncoder.h"

struct lame_global_struct;
typedef struct lame_global_struct lame_global_flags;

//Speex encoder implementation
class AudioMp3Encoder:public AudioEncoder
{
 public:
    //always encode in speex or mp3
    AudioMp3Encoder(AudioStreamSetting* outputSetting, int aBitrate);
    virtual ~AudioMp3Encoder();
    virtual SmartPtr<SmartBuffer> encodeAFrame(SmartPtr<SmartBuffer> input) ;
 private:
    lame_global_flags * lgf_;
};

#endif
