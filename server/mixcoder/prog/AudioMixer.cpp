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
    for ( int i = 0; i < bufSize; i+=2 ) {
        int idx = randInRange(bufSize);
        buffer[idx] = 2;
        idx = randInRange(bufSize);
        buffer[idx] = -2;
    }
}

void AudioMixer::mixTwoStreams(SmartPtr<SmartBuffer> buffer[], 
                   int* twoIndex,
                   short* valShort,
                   int sampleSize)
{
    int a = twoIndex[0];
    int b = twoIndex[1];
    short* aData = (short*)buffer[a]->data();
    short* bData = (short*)buffer[b]->data();
    //    fprintf(stderr, "--------two index=%d %d\r\n", twoIndex[0], twoIndex[1]);
    for ( int i = sampleSize-1; i>=0; i-- ) {
        int val = aData[i] + bData[i];
        valShort[i] = CLIP(val, 32767, -32768);
    }
}

void AudioMixer::mixThreeStreams(SmartPtr<SmartBuffer> buffer[], 
                   int* threeIndex,
                   short* valShort,
                   int sampleSize)
{
    int a = threeIndex[0];
    int b = threeIndex[1];
    int c = threeIndex[2];
    short* aData = (short*)buffer[a]->data();
    short* bData = (short*)buffer[b]->data();
    short* cData = (short*)buffer[c]->data();
    for ( int i = sampleSize-1 ; i>=0; i-- ) {
        int val = aData[i] + bData[i] + cData[i];
        valShort[i] = CLIP(val, 32767, -32768);
    }
}

void AudioMixer::mixFourStreams(SmartPtr<SmartBuffer> buffer[], 
                                int* fourIndex,
                                short* valShort,
                                int sampleSize)
{
    int a = fourIndex[0];
    int b = fourIndex[1];
    int c = fourIndex[2];
    int d = fourIndex[3];
    short* aData = (short*)buffer[a]->data();
    short* bData = (short*)buffer[b]->data();
    short* cData = (short*)buffer[c]->data();
    short* dData = (short*)buffer[d]->data();
    for ( int i = sampleSize-1; i>=0; i-- ) {
        int val = aData[i] + bData[i] + cData[i] + dData[i];
        valShort[i] = CLIP(val, 32767, -32768);
    }
}
void AudioMixer::findIndexes(AudioStreamSetting settings[],
                     u32 excludeStreamId,
                     int* indexArr)
{
    int i = 0;
    for(u32 j=0; j<MAX_XCODING_INSTANCES; j++) {
        if( settings[j].bIsValid && j != excludeStreamId) {
            indexArr[i++] = j;
        }
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
        switch ( totalStreams ) {
            case 1: {
                if( excludeStreamId != 0xffffffff ) {
                    //if nothing is mixed, i.e., one stream only voip, use white noise
                    genWhiteNoise(valShort, sampleSize);
                } else {
                    int a = 0;
                    for(u32 j=0; j<MAX_XCODING_INSTANCES; j++) {
                        if( settings[j].bIsValid && j != excludeStreamId ) { 
                            a = j;
                            break;
                        }
                    }
                    memcpy((u8*)valShort, buffer[a]->data(), sampleSize*sizeof(short));
                }
                break;
            }
            case 2: {
                if( excludeStreamId != 0xffffffff ) {
                    int a = 0;
                    for(u32 j=0; j<MAX_XCODING_INSTANCES; j++) {
                        if( settings[j].bIsValid && j != excludeStreamId ) { 
                            a = j;
                            break;
                        }
                    }
                    memcpy((u8*)valShort, buffer[a]->data(), sampleSize*sizeof(short));
                } else {
                    int twoIndex[2];
                    findIndexes(settings, excludeStreamId, twoIndex);
                    mixTwoStreams(buffer, twoIndex, valShort, sampleSize);
                }
                break;
            }
            case 3: {
                if( excludeStreamId != 0xffffffff ) {
                    int twoIndex[2];
                    findIndexes(settings, excludeStreamId, twoIndex);
                    mixTwoStreams(buffer, twoIndex, valShort, sampleSize);
                } else {
                    int threeIndex[3];
                    findIndexes(settings, excludeStreamId, threeIndex);
                    mixThreeStreams(buffer, threeIndex, valShort, sampleSize);
                }
                break;
            }
            case 4: {
                if( excludeStreamId != 0xffffffff ) {
                    int threeIndex[3];
                    findIndexes(settings, excludeStreamId, threeIndex);
                    mixThreeStreams(buffer, threeIndex, valShort, sampleSize);
                } else {
                    int fourIndex[2];
                    findIndexes(settings, excludeStreamId, fourIndex);
                    mixFourStreams(buffer, fourIndex, valShort, sampleSize);
                }
                break;
            }
            /* Brutal force algorithm
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
            */
        }

        result = new SmartBuffer( frameTotalLen, (u8*)valShort);
    }
    return result;
}
