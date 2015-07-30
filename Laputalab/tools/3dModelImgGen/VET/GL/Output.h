//
//  Output.h
//  VET
//
//  Created by Howard Wang on 15-7-22.
//  Copyright (c) 2015年 Xingze. All rights reserved.
//

#ifndef VET_Output_h
#define VET_Output_h

#define ENABLE_LOGGER
#ifdef ENABLE_LOGGER
#define OUTPUT(...) printf(__VA_ARGS__)
#define ASSERT(x) assert(x)
#else
#define OUTPUT(...)
#define ASSERT(x)
#endif

#endif
