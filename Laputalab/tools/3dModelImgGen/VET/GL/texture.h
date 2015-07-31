//
//  texture.h
//  Laputa
//
//  Created by Howard Wang on 15-4-2.
//
//

#ifndef Laputa_texture_h
#define Laputa_texture_h

#include <string>
#include "material.h"

class Texture: public Material
{
public:
    Texture(GLint texCountLocation,
            GLint diffuseColorLocation,
            GLint ambientColorLocation,
            GLint specularColorLocation,
            GLint textureImageLocation,
            const Vector4f& diffuseColor,
            const Vector4f& ambientColor,
            const Vector4f& specularColor,
            const std::string& fileName);
    
    virtual ~Texture() {
        // Delete texture object
        glDeleteTextures ( 1, &m_textureObj );
    }
    bool load();
    
    void bind(GLint textureId);
protected:
    std::string m_fileName;
    GLuint m_textureObj; //object id generated
};

#endif
