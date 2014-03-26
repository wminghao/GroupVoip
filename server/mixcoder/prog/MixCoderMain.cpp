#include "MixCoder.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

//big enough buffer
const int MAX_BUF_SIZE = 100;

int main( int argc, char** argv ) {
    
    int videoBitrate = 40;
    int videoWidth = 640;
    int videoHeight = 480;

    int audioBitrate = 32;
    int audioFrequency = 44100;
    
    MixCoder* mixCoder = new MixCoder(videoBitrate, videoWidth, videoHeight, audioBitrate, audioFrequency);

    u8 data[MAX_BUF_SIZE];
    SmartPtr<SmartBuffer> output;
    int totalBytesRead = 0;

    //debug code
    int totalOutput = 0;
    while ( (totalBytesRead = read( 0, data, MAX_BUF_SIZE)) > 0 ) {
        SmartPtr<SmartBuffer> input = new SmartBuffer( totalBytesRead, data );
        if( !mixCoder->newInput( input ) ) {
            fprintf(stderr, "input error");
            return -1;
        }
        while( output = mixCoder->getOutput() ) {
            totalOutput+=output->dataLength();
            //fprintf(stderr, "------totalOutput=%d\r\n", totalOutput);
            write( 1, output->data(), output->dataLength() );
        } 
    }
    
    //flush the remaining
    mixCoder->flush();
    while( output = mixCoder->getOutput() ) {
        write( 1, output->data(), output->dataLength() );
    }

    delete mixCoder;
    return 1;
}
