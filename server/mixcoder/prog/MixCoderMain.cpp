#include "MixCoder.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/signal.h>
#include <execinfo.h>
#include <sys/types.h>
#include "fwk/log.h"

void handlesig( int signum )
{    
    LOG( "Exiting on signal: %d", signum  );
    LOG( "GroupVoip just crashed, see stack dump below." );
    //TODO print stack trace
    exit( 0 );
}

//big enough buffer
const int MAX_BUF_SIZE = 100;

int main( int argc, char** argv ) {

    signal( SIGPIPE, SIG_IGN );
    signal( SIGSEGV, handlesig );
    signal( SIGFPE, handlesig );
    signal( SIGBUS, handlesig );
    signal( SIGSYS, handlesig );
    
    Logger::initLog("MixCoder", kSyslog);

    int videoBitrate = 40;
    int videoWidth = 640;
    int videoHeight = 480;

    int audioBitrate = 16; //16kbps
    int audioFrequency = 16000;
    
    MixCoder* mixCoder = new MixCoder(videoBitrate, videoWidth, videoHeight, audioBitrate, audioFrequency);

    u8 data[MAX_BUF_SIZE];
    SmartPtr<SmartBuffer> output;
    int totalBytesRead = 0;

    //debug code
    int totalOutput = 0;
    while ( (totalBytesRead = read( 0, data, MAX_BUF_SIZE)) > 0 ) {
        SmartPtr<SmartBuffer> input = new SmartBuffer( totalBytesRead, data );
        if( !mixCoder->newInput( input ) ) {
            LOG("input error");
            return -1;
        }
        while( output = mixCoder->getOutput() ) {
            totalOutput+=output->dataLength();
            //LOG("------totalOutput=%d\n", totalOutput);
            write( 1, output->data(), output->dataLength() );
            fflush(stdout);
        } 
    }
    LOG("------final totalOutput=%d\n", totalOutput);
    //flush the remaining
    mixCoder->flush();
    while( output = mixCoder->getOutput() ) {
        write( 1, output->data(), output->dataLength() );
    }

    delete mixCoder;
    return 1;
}
