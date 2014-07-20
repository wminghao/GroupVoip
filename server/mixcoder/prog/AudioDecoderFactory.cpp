#include "AudioDecoderFactory.h"    
#include "fwk/SmartPtr.h"
#include "fwk/SmartBuffer.h"
#include "fwk/Units.h"
#include "AudioSpeexDecoder.h"
#include "AudioFfmpegDecoder.h"
#include "fwk/log.h"

AudioDecoder* AudioDecoderFactory::CreateAudioDecoder(SmartPtr<AccessUnit> au, int streamId)
{
    AudioDecoder* decoder = NULL;
    if( au ) {
        unsigned char firstByte = au->ctype;
        AudioCodecId codecType = (AudioCodecId)((firstByte & 0xf0)>>4);
        AudioRate audioRate = (AudioRate)((firstByte & 0x0c)>>2);
        AudioSize audioSize = (AudioSize)((firstByte & 0x02)>>1);
        AudioType audioType = (AudioType)(firstByte & 0x01);
 
        LOG("AudioDecoderFactory::CreateAudioDecoder: firstByte=0x%x, codecType=%d, audioRate=%d, audioSize=%d, audioType=%d\r\n", firstByte, codecType, audioRate, audioSize, audioType);
       
        if( codecType == kSpeex ) {
            decoder = new AudioSpeexDecoder(streamId, 
                                            kSpeex,
                                            k16kHz,
                                            kSnd16Bit,
                                            kSndMono);
        } else {
            decoder = new AudioFfmpegDecoder(streamId, 
                                            codecType,
                                            audioRate,
                                            audioSize,
                                            audioType);
        }
    }
    return decoder;
}
