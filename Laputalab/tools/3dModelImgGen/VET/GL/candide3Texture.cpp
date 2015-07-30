//
//  candide3Texture.cpp
//  LaputaDesktop
//
//  Created by Howard Wang on 15-5-28.
//  Copyright (c) 2015年 Howard Wang. All rights reserved.
//

#include "candide3Texture.h"
Candide3Texture::Candide3Texture(GLint texCountLocation,
                                 GLint textureImageLocation):m_texCountLocation(texCountLocation), m_textureImageLocation(textureImageLocation)
{}

void Candide3Texture::bind(GLint textureId,
                           GLuint textureObj)
{
    glUniform1i(m_texCountLocation, 3); //3 indicate candide3 Texture
    glActiveTexture( GL_TEXTURE0 + textureId );
    glBindTexture(GL_TEXTURE_2D, textureObj);
    glUniform1i(m_textureImageLocation, textureId); //set the sampler texture to textureId
}
