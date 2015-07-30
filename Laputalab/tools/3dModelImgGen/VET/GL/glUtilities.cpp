//
//  glUtilities.cpp
//  LaputaDesktop
//
//  Created by Howard Wang on 15-5-29.
//  Copyright (c) 2015年 Howard Wang. All rights reserved.
//

#include "glUtilities.h"
#include <sys/time.h>
#include "Output.h"

//time functions
uint64_t getTimeMs64() {
    /* Linux */
    struct timeval tv;
    
    gettimeofday(&tv, NULL);
    
    uint64_t ret = tv.tv_usec;
    /* Convert from micro seconds (10^-6) to milliseconds (10^-3) */
    ret /= 1000;
    
    /* Adds the seconds (10^0) after converting them to milliseconds (10^-3) */
    ret += (tv.tv_sec * 1000);
    
    return ret;
}

char* readAllocFile(const char* path)
{
    FILE *stream;
    char *contents;
    uint64_t fileSize = 0;
    
    //Open the stream. Note "b" to avoid DOS/UNIX new line conversion.
    stream = fopen(path, "rb");
    
    //Seek to the end of the file to determine the file size
    fseek(stream, 0L, SEEK_END);
    fileSize = ftell(stream);
    fseek(stream, 0L, SEEK_SET);
    
    //Allocate enough memory (add 1 for the \0, since fread won't add it)
    contents = (char*)malloc(fileSize+1);
    
    //Read the file
    size_t size=fread(contents,1,fileSize,stream);
    contents[size]=0; // Add terminating zero.
    
    //Close the file
    fclose(stream);
    
    return contents;
}

void PerfCalc::start()
{
    startTs_ = getTimeMs64();
    numTimes_++;
}
void PerfCalc::end()
{
    uint64_t elapsedTime = getTimeMs64() - startTs_;
    totalElapsedTime_ += elapsedTime;
    OUTPUT("---%s elapsedTimeInMs=%llu, avgElapsedTimeInMs=%llu", prefix_.c_str(), elapsedTime, (totalElapsedTime_/numTimes_));
}