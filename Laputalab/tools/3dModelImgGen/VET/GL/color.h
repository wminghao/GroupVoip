//
//  color.h
//  Laputa
//
//  Created by Howard Wang on 15-4-6.
//
//

#ifndef __Laputa__color__
#define __Laputa__color__

#include <stdio.h>

#include "material.h"

class Color: public Material
{
public:
    Color(GLint texCountLocation,
          GLint diffuseColorLocation,
          GLint ambientColorLocation,
          GLint specularColorLocation,
          GLint textureImageLocation,
          const Vector4f& diffuseColor,
          const Vector4f& ambientColor,
          const Vector4f& specularColor):Material(texCountLocation,
                                                 diffuseColorLocation, ambientColorLocation, specularColorLocation,
                                                 textureImageLocation,
                                                 diffuseColor, ambientColor, specularColor)
    {
    }
    
    virtual ~Color() {}
    bool load();
    
    void bind(GLint textureId);
private:
};
#endif /* defined(__Laputa__color__) */
