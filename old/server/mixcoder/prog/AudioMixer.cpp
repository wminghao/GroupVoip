#include "AudioMixer.h"
#include <stdlib.h>     /* srand, rand */
#include <time.h>

#define CLIP( val, max, min ) ( val >  max ) ? max: ( ( val < min ) ? min: val)

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

void AudioMixer::mixOneStreams(SmartPtr<AudioRawData>* rawData, 
                               int oneIndex,
                               short* valShort,
                               int sampleSize)
{
    //fprintf(stderr, "--------one index=%d, len=%ld, isStereo=%d\r\n", oneIndex, rawData[oneIndex]->rawAudioFrame_->dataLength(), rawData[oneIndex]->bIsStereo );

    if( rawData[oneIndex]->bIsStereo ) {
        memcpy((u8*)valShort, rawData[oneIndex]->rawAudioFrame_->data(), sampleSize*sizeof(short)*2);        
    } else {
        int j = sampleSize*2-1;
        int i = sampleSize-1;
        short* rawDataPtr = (short*)rawData[oneIndex]->rawAudioFrame_->data();
        while( i>=0 ) {
            short singleSample = rawDataPtr[i];
            valShort[j--] = singleSample;
            valShort[j--] = singleSample;
            i--;
        }
    }
}

void AudioMixer::mixTwoStreams(SmartPtr<AudioRawData>* rawData,
                   int* twoIndex,
                   short* valShort,
                   int sampleSize)
{
    int a = twoIndex[0];
    int b = twoIndex[1];
    short* aData = (short*)rawData[a]->rawAudioFrame_->data();
    short* bData = (short*)rawData[b]->rawAudioFrame_->data();
    
    //fprintf(stderr, "--------two index=%d %d, len=%ld %ld\r\n", twoIndex[0], twoIndex[1], rawData[a]->rawAudioFrame_->dataLength(), rawData[b]->rawAudioFrame_->dataLength());
    bool aIsStereo = rawData[a]->bIsStereo;
    bool bIsStereo = rawData[b]->bIsStereo;
    for(int i = sampleSize*2-1; i >= 0; i-- ) {
        int val = 0;
        if( aIsStereo ) {
            val += aData[i];
        } else {
            val += aData[i/2];
        }

        if( bIsStereo ) {
            val += bData[i];
        } else {
            val += bData[i/2];
        }
        valShort[i] = CLIP(val, 32767, -32768);
    }
}

void AudioMixer::mixThreeStreams(SmartPtr<AudioRawData>* rawData,
                   int* threeIndex,
                   short* valShort,
                   int sampleSize)
{
    int a = threeIndex[0];
    int b = threeIndex[1];
    int c = threeIndex[2];
    short* aData = (short*)rawData[a]->rawAudioFrame_->data();
    short* bData = (short*)rawData[b]->rawAudioFrame_->data();
    short* cData = (short*)rawData[c]->rawAudioFrame_->data();

    bool aIsStereo = rawData[a]->bIsStereo;
    bool bIsStereo = rawData[b]->bIsStereo;
    bool cIsStereo = rawData[c]->bIsStereo;
    for ( int i = sampleSize*2-1 ; i>=0; i-- ) {
        int val = 0;
        if( aIsStereo ) {
            val += aData[i];
        } else {
            val += aData[i/2];
        }

        if( bIsStereo ) {
            val += bData[i];
        } else {
            val += bData[i/2];
        }

        if( cIsStereo ) {
            val += cData[i];
        } else {
            val += cData[i/2];
        }

        valShort[i] = CLIP(val, 32767, -32768);
    }
}

void AudioMixer::mixFourStreams(SmartPtr<AudioRawData>* rawData,
                                int* fourIndex,
                                short* valShort,
                                int sampleSize)
{
    int a = fourIndex[0];
    int b = fourIndex[1];
    int c = fourIndex[2];
    int d = fourIndex[3];
    short* aData = (short*)rawData[a]->rawAudioFrame_->data();
    short* bData = (short*)rawData[b]->rawAudioFrame_->data();
    short* cData = (short*)rawData[c]->rawAudioFrame_->data();
    short* dData = (short*)rawData[d]->rawAudioFrame_->data();

    bool aIsStereo = rawData[a]->bIsStereo;
    bool bIsStereo = rawData[b]->bIsStereo;
    bool cIsStereo = rawData[c]->bIsStereo;
    bool dIsStereo = rawData[d]->bIsStereo;

    for ( int i = sampleSize*2-1; i>=0; i-- ) {
        int val = 0;
        if( aIsStereo ) {
            val += aData[i];
        } else {
            val += aData[i/2];
        }

        if( bIsStereo ) {
            val += bData[i];
        } else {
            val += bData[i/2];
        }

        if( cIsStereo ) {
            val += cData[i];
        } else {
            val += cData[i/2];
        }

        if( dIsStereo ) {
            val += dData[i];
        } else {
            val += dData[i/2];
        }
        valShort[i] = CLIP(val, 32767, -32768);
    }
}
void AudioMixer::findIndexes(SmartPtr<AudioRawData>* rawData,
                             u32 excludeStreamId,
                             int* indexArr)
{
    int i = 0;
    for(u32 j=0; j<MAX_XCODING_INSTANCES; j++) {
        if( rawData[j] && rawData[j]->bIsValid && j != excludeStreamId) {
            indexArr[i++] = j;
        }
    } 
}



//do the mixing, for now, always mix n speex streams into 1 speex stream
SmartPtr<SmartBuffer> AudioMixer::mixStreams(SmartPtr<AudioRawData>* rawData,
                                             int sampleSize,
                                             int totalStreams,
                                             u32 excludeStreamId)
{
    SmartPtr<SmartBuffer> result;
    if ( totalStreams > 0 ) {
        u32 frameTotalLen = sampleSize*sizeof(u16)*2; //stereo
        short valShort[sampleSize*2];
        switch ( totalStreams ) {
            case 1: {
                if( excludeStreamId != 0xffffffff ) {
                    //if nothing is mixed, i.e., one stream only voip, use white noise
                    genWhiteNoise(valShort, sampleSize*2);
                } else {
                    int a = 0;
                    for(u32 j=0; j<MAX_XCODING_INSTANCES; j++) {
                        if( rawData[j] && rawData[j]->bIsValid && j != excludeStreamId ) { 
                            a = j;
                            break;
                        }
                    }
                    mixOneStreams(rawData, a, valShort, sampleSize);
                }
                break;
            }
            case 2: {
                if( excludeStreamId != 0xffffffff ) {
                    int a = 0;
                    for(u32 j=0; j<MAX_XCODING_INSTANCES; j++) {
                        if( rawData[j] && rawData[j]->bIsValid && j != excludeStreamId ) { 
                            a = j;
                            break;
                        }
                    }
                    mixOneStreams(rawData, a, valShort, sampleSize);
                } else {
                    int twoIndex[2];
                    findIndexes(rawData, excludeStreamId, twoIndex);
                    mixTwoStreams(rawData, twoIndex, valShort, sampleSize);
                }
                break;
            }
            case 3: {
                if( excludeStreamId != 0xffffffff ) {
                    int twoIndex[2];
                    findIndexes(rawData, excludeStreamId, twoIndex);
                    mixTwoStreams(rawData, twoIndex, valShort, sampleSize);
                } else {
                    int threeIndex[3];
                    findIndexes(rawData, excludeStreamId, threeIndex);
                    mixThreeStreams(rawData, threeIndex, valShort, sampleSize);
                }
                break;
            }
            case 4: {
                if( excludeStreamId != 0xffffffff ) {
                    int threeIndex[3];
                    findIndexes(rawData, excludeStreamId, threeIndex);
                    mixThreeStreams(rawData, threeIndex, valShort, sampleSize);
                } else {
                    int fourIndex[2];
                    findIndexes(rawData, excludeStreamId, fourIndex);
                    mixFourStreams(rawData, fourIndex, valShort, sampleSize);
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
