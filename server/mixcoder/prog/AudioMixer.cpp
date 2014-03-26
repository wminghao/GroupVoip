#include "AudioMixer.h"

inline int CLIP(int val, int max, int min)
{
    int ret;
    if( val >  max) {
        ret = max;
    } else if ( val < min ) {
        ret = min;
    } else {
        ret = val;
    }
    return ret;
}

//do the mixing, for now, always mix n speex streams into 1 speex stream
SmartPtr<SmartBuffer> AudioMixer::mixStreams(SmartPtr<SmartBuffer> buffer[], 
                                             AudioStreamSetting settings[], 
                                             int totalStreams,
                                             u32 excludeStreamId)
{
    SmartPtr<SmartBuffer> result;
    if ( totalStreams > 0 ) {
        u32 frameTotalLen = buffer[0]->dataLength();
        short valShort[frameTotalLen/2];
        for ( u32 i = 0; i < frameTotalLen; i+=2 ) {
            int val = 0;
            for(u32 j=0; j<MAX_XCODING_INSTANCES; j++) {
                if( settings[j].bIsValid && j != excludeStreamId ) { 
                    short* data = (short*)buffer[j]->data();
                    val += data[i/2];
                }
            }
            //fprintf( stderr, "audio mixed val = %d\n", val);       
            valShort[i/2] = CLIP(val, 32767, -32768);
        }
        result = new SmartBuffer( frameTotalLen, (u8*)valShort);
    }
    return result;
}
