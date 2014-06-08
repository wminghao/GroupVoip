#include "AudioMixer.h"
#include <stdlib.h>     /* srand, rand */
#include <time.h>

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

int randInRange(int max)
{
    double frac = ((double)rand() / (double) ((double)RAND_MAX + 1));
    return (int) ( frac * (max + 1));
}

//TODO a better whitenoise generation algorithm
void genWhiteNoise(short* buffer, int bufSize)
{
    srand (time(NULL));
    memset(buffer, 0, bufSize*sizeof(short));
    for ( int i = 0; i < bufSize; i++ ) {
        int idx = randInRange(bufSize);
        buffer[idx] = 2;
        idx = randInRange(bufSize);
        buffer[idx] = -2;
    }
}

//do the mixing, for now, always mix n speex streams into 1 speex stream
SmartPtr<SmartBuffer> AudioMixer::mixStreams(SmartPtr<SmartBuffer> buffer[], 
                                             AudioStreamSetting settings[], 
                                             int sampleSize,
                                             int totalStreams,
                                             u32 excludeStreamId)
{
    SmartPtr<SmartBuffer> result;
    if ( totalStreams > 0 ) {
        u32 frameTotalLen = sampleSize*sizeof(u16);
        short valShort[sampleSize];
        bool isMixed = false;
        for ( u32 i = 0; i < frameTotalLen; i+=2 ) {
            int val = 0;
            for(u32 j=0; j<MAX_XCODING_INSTANCES; j++) {
                if( settings[j].bIsValid && j != excludeStreamId ) { 
                    short* data = (short*)buffer[j]->data();
                    val += data[i/2];
                    isMixed = true;
                    //LOG("i=%d j=%d audio mixed val = %d, flen=%d, excludeStreamId=%d\n", i, j, val, frameTotalLen, excludeStreamId);       
                }
            }
            valShort[i/2] = CLIP(val, 32767, -32768);
        }
        //if nothing is mixed, i.e., one stream only voip, use white noise
        if( !isMixed ) {
            genWhiteNoise(valShort, sampleSize);
        }

        result = new SmartBuffer( frameTotalLen, (u8*)valShort);
    }
    return result;
}
