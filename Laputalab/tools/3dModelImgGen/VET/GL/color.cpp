//
//  color.cpp
//  Laputa
//
//  Created by Howard Wang on 15-4-6.
//
//

#include "color.h"
#include <stdio.h>

bool Color::load()
{
    return true;
}

void Color::bind(GLint textureId)
{
    glUniform1i(m_texCountLocation, 0);
    glUniform4f(m_diffuseColorLocation, m_diffuseColor.x, m_diffuseColor.y, m_diffuseColor.z, m_diffuseColor.w);
    glUniform4f(m_ambientColorLocation, m_ambientColor.x, m_ambientColor.y, m_ambientColor.z, m_ambientColor.w);
    glUniform4f(m_specularColorLocation, m_specularColor.x, m_specularColor.y, m_specularColor.z, m_specularColor.w);
}
