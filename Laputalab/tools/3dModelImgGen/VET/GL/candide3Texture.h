//
//  candide3Texture.h
//  LaputaDesktop
//
//  Created by Howard Wang on 15-5-28.
//  Copyright (c) 2015年 Howard Wang. All rights reserved.
//

#ifndef __LaputaDesktop__candide3Texture__
#define __LaputaDesktop__candide3Texture__

#include "texture.h"

//a mask texture
class Candide3Texture
{
public:
    Candide3Texture(GLint texCountLocation,
                    GLint textureImageLocation);
    
    virtual void bind(GLint textureId,
                      GLuint textureObj);
    
private:
    GLint m_texCountLocation;
    GLint m_textureImageLocation;
};
#endif /* defined(__LaputaDesktop__candide3Texture__) */
