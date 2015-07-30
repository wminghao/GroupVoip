//
//  err.h
//  Laputa
//
//  Created by Howard Wang on 15-5-14.
//
//

#ifndef Laputa_err_h
#define Laputa_err_h

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "platform.h"
#include "Output.h"

inline void getGLErr(const char* prefix){
    int err = glGetError();
    if( err != GL_NO_ERROR ) {
        OUTPUT("%s err=%d\r\n", prefix, err);
    }
}
/*
#ifdef DESKTOP_GL
inline void readParam(const char* prefix)
{
    int objectType;
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,&objectType);
    assert(objectType == GL_RENDERBUFFER || GL_TEXTURE == objectType);
    int objectName;
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,&objectName);
    assert(glIsRenderbuffer(objectName) == GL_TRUE);
    
    glBindRenderbuffer(GL_RENDERBUFFER,objectName);
    int wid,hei,fmt,sam;
    glGetRenderbufferParameteriv(GL_RENDERBUFFER,GL_RENDERBUFFER_WIDTH,&wid);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER,GL_RENDERBUFFER_HEIGHT,&hei);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER,GL_RENDERBUFFER_INTERNAL_FORMAT,&fmt);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER,GL_RENDERBUFFER_SAMPLES,&sam);
    glBindRenderbuffer(GL_RENDERBUFFER,0);
    OUTPUT("%s: color %dx%d, fmt=%d, samples=%d\n",prefix, wid,hei,fmt,sam);
    
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,&objectType);
    assert(objectType == GL_RENDERBUFFER);
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,&objectName);
    assert(glIsRenderbuffer(objectName) == GL_TRUE);
    
    glBindRenderbuffer(GL_RENDERBUFFER,objectName);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER,GL_RENDERBUFFER_WIDTH,&wid);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER,GL_RENDERBUFFER_HEIGHT,&hei);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER,GL_RENDERBUFFER_INTERNAL_FORMAT,&fmt);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER,GL_RENDERBUFFER_SAMPLES,&sam);
    glBindRenderbuffer(GL_RENDERBUFFER,0);
    OUTPUT("%s: depth %dx%d, fmt=%d, samples=%d\n",prefix, wid,hei,fmt,sam);
}

inline void readFrameBuffers(const char* str)
{
    GLuint frameBufferObject = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&frameBufferObject);
    OUTPUT("%s GL_FRAMEBUFFER_BINDING=%d\r\n",str, frameBufferObject);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, (GLint*)&frameBufferObject);
    OUTPUT("%s GL_DRAW_FRAMEBUFFER_BINDING=%d\r\n",str, frameBufferObject);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, (GLint*)&frameBufferObject);
    OUTPUT("%s GL_READ_FRAMEBUFFER_BINDING=%d\r\n",str, frameBufferObject);
}

inline void getReadBuffers()
{
    GLint data[10];
    memset(data, 0, 40);
    glGetIntegerv( GL_READ_BUFFER, data );
    for(int i = 0; i< 10; i++) {
        OUTPUT("GL_COLOR_ATTACHMENT0=%d, data[%d]=%d\r\n", GL_COLOR_ATTACHMENT0, i, data[i]);
    }
}
#endif //DESKTOP_GL
*/
#endif
