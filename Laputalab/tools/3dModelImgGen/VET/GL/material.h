//
//  material.h
//  Laputa
//
//  Created by Howard Wang on 15-4-6.
//
//

#ifndef __Laputa__material__
#define __Laputa__material__

#include <stdio.h>
#include <string>

#include "platform.h"
#include "unit.h"

class Material
{
public:
    Material(GLint texCountLocation,
             GLint diffuseColorLocation,
             GLint ambientColorLocation,
             GLint specularColorLocation,
             GLint textureImageLocation,
             const Vector4f& diffuseColor,
             const Vector4f& ambientColor,
             const Vector4f& specularColor){
        m_texCountLocation = texCountLocation;
        m_diffuseColorLocation = diffuseColorLocation;
        m_ambientColorLocation = ambientColorLocation;
        m_specularColorLocation = specularColorLocation;
        m_textureImageLocation = textureImageLocation;
        m_diffuseColor = diffuseColor;
        m_ambientColor = ambientColor;
        m_specularColor = specularColor;
    }
    virtual ~Material(){}
    virtual bool load() = 0;
    
    virtual void bind(GLint textureId) = 0;
    
protected:
    GLint m_texCountLocation;
    GLint m_diffuseColorLocation;
    GLint m_ambientColorLocation;
    GLint m_specularColorLocation;
    GLint m_textureImageLocation;
    Vector4f m_diffuseColor;
    Vector4f m_ambientColor;
    Vector4f m_specularColor;
};
#endif /* defined(__Laputa__material__) */
