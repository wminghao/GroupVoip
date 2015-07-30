//
//  ReflectionTexture.h
//  Laputa
//
//  Created by Howard Wang on 15-4-10.
//
//

#ifndef __Laputa__ReflectionTexture__
#define __Laputa__ReflectionTexture__

#include <stdio.h>
#include "texture.h"

class ReflectionTexture: public Texture
{
public:
    ReflectionTexture(GLint texCountLocation,
                      GLint diffuseColorLocation,
                      GLint ambientColorLocation,
                      GLint textureImageLocation,
                      const Vector4f& diffuseColor,
                      const Vector4f& ambientColor,
                      const std::string& baseFileName,
                      GLint reflectionTextureImageLocation,
                      const std::string& reflectionFileName);
    
    virtual ~ReflectionTexture() {
        // Delete reflection texture object
        glDeleteTextures ( 1, &m_reflectionTextureObj );
    }
    virtual bool load();
    
    virtual void bind(GLint textureId);
private:
    GLint m_reflectionTextureImageLocation;
    std::string m_reflectionFileName;
    GLuint m_reflectionTextureObj; //object id generated
};

#endif /* defined(__Laputa__ReflectionTexture__) */
