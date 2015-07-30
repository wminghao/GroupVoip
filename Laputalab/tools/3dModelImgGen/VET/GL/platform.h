//
//  platform.h
//  LaputaDesktop
//
//  Created by Howard Wang on 15-5-12.
//  Copyright (c) 2015年 Howard Wang. All rights reserved.
//

#ifndef __platform_h__
#define __platform_h__

#if defined( __MACH__ )
#include "TargetConditionals.h"
#if TARGET_OS_IPHONE
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>
#else //!TARGET_OS_IPHONE
#define DESKTOP_GL //define desktop opengl
#include <OpenGL/gl.h>
#include <OpenGL/gl3.h>
#include <OpenGL/OpenGL.h>
#include <OpenGL/CGLCurrent.h>
#include <OpenGL/CGLTypes.h>
#endif //!TARGET_OS_IPHONE
//#elif defined( __linux__ )
#else //__linux__
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#define DESKTOP_GL //define desktop opengl
#endif

#endif
