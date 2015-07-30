//
//  glUtilities.h
//  LaputaDesktop
//
//  Created by Howard Wang on 15-5-29.
//  Copyright (c) 2015年 Howard Wang. All rights reserved.
//

#ifndef __LaputaDesktop__glUtilities__
#define __LaputaDesktop__glUtilities__

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <queue>
#include <stdint.h>

using namespace std;

//file io function
char* readAllocFile(const char* path);

class PerfCalc
{
public:
    PerfCalc(const char* prefix):totalElapsedTime_(0), numTimes_(0), prefix_(prefix){}
    void start();
    void end();
private:
    uint64_t startTs_;
    uint64_t totalElapsedTime_;
    int numTimes_;
    string prefix_;
};

#endif /* defined(__LaputaDesktop__glUtilities__) */
