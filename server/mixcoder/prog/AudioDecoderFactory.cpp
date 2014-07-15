#include "AudioDecoderFactory.h"    
#include "fwk/SmartPtr.h"
#include "fwk/SmartBuffer.h"
#include "fwk/Units.h"
#include "AudioSpeexDecoder.h"
#include "AudioFfmpegDecoder.h"

AudioDecoder* AudioDecoderFactory::CreateAudioDecoder(SmartPtr<AccessUnit> au, int streamId)
{
    AudioDecoder* decoder = NULL;
    if( au ) {
        SmartPtr<SmartBuffer> payload = au->payload;
        unsigned char firstByte = payload->data()[0];
        AudioCodecId codecType = (AudioCodecId)((firstByte & 0xf0)>>4);
        AudioRate audioRate = (AudioRate)((firstByte & 0xc0)>>2);
        AudioSize audioSize = (AudioSize)((firstByte & 0x02)>>1);
        AudioType audioType = (AudioType)(firstByte & 0x01);
        
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
