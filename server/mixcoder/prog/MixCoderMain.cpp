#include "MixCoder.h"

int main( int argc, char** argv ) {
    
    int videoBitrate = 400;
    int videoWidth = 480;
    int audioBitrate = 32;
    int audioFrequency = 44100;
    
    MixCoder mixCoder = new MixCoder();
    mixCoder->initialize();

    delete mixCoder;
}

