#ifndef __AUDIOTIMESTAMP_MAPPER__
#define __AUDIOTIMESTAMP_MAPPER__

#include "AudioResampler.h"
#include "fwk/log.h"

#define MAX_INT 0xffffffff
//audio timestamp mapper from #ofsamples to ms
class AudioTimestampMapper
{
 public:
    AudioTimestampMapper() : startingTimestamp_(MAX_INT), cnt_(0), curTimestamp_(MAX_INT) {}
    ~AudioTimestampMapper() {}

    bool shouldAdjustTs(u32 ts) {
        return ts > ( curTimestamp_ + 100 );
    }

    u32 getNextTimestamp(u32 ts) {
        if( MAX_INT == startingTimestamp_ ) {
            startingTimestamp_ = ts;
            cnt_ = 0;
        } else {
            //if timestamp has drifted more than 100ms, reset startingTimestamp_ and cnt
            if( shouldAdjustTs( ts ) ) {
                LOG("-----------Timestamp JUMP, oldStartingTimestamp_=%d, newStartingTimestamp_=%d\r\n", startingTimestamp_, ts);
                startingTimestamp_ = ts;
                cnt_ = 0;
            }
        }
        curTimestamp_ = ts;

        double tsD = startingTimestamp_ + (cnt_ * (double)1000 * (double)MP3_FRAME_SAMPLE_SIZE)/(double)MP3_SAMPLE_PER_SEC;
        cnt_++;
        return (u32)tsD;    
    }

 private:
    double startingTimestamp_;
    double cnt_;

    u32 curTimestamp_;
};

#endif
