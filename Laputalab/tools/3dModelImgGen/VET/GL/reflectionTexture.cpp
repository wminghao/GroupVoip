//
//  ReflectionTexture.cpp
//  Laputa
//
//  Created by Howard Wang on 15-4-10.
//
//

#include "reflectionTexture.h"

#include <wand/magick_wand.h>
#include <wand/magick-image.h>
#include "Output.h"


ReflectionTexture::ReflectionTexture(GLint texCountLocation,
                                     GLint diffuseColorLocation,
                                     GLint ambientColorLocation,
                                     GLint m_specularColorLocation,
                                     GLint textureImageLocation,
                                     const Vector4f& diffuseColor,
                                     const Vector4f& ambientColor,
                                     const Vector4f& specularColor,
                                     const std::string& baseFileName,
                                     GLint reflectionTextureImageLocation,
                                     const std::string& reflectionFileName):Texture(texCountLocation,
                                                       diffuseColorLocation, ambientColorLocation, m_specularColorLocation,
                                                       textureImageLocation,
                                                       diffuseColor, ambientColor, specularColor,
                                                       baseFileName)
{
    m_reflectionTextureImageLocation = reflectionTextureImageLocation;
    m_reflectionFileName      = reflectionFileName;
}

static int getPixelIndex(int x, int y, size_t width, size_t height) {
    return (int)(y * width + x ) * 3;
}

bool ReflectionTexture::load()
{
    //first load parent texture
    Texture::load();
    
    MagickWandGenesis();
    MagickWand *wand = NewMagickWand();
    
    // Read the image - all you need to do is change "logo:" to some other
    // filename to have this resize and, if necessary, convert a different file
    if( MagickReadImage(wand, m_reflectionFileName.c_str()) != MagickFalse) {
        // Get the image's width and height
        size_t width,height;
        width = MagickGetImageWidth(wand);
        height = MagickGetImageHeight(wand);
        
        //b/c the input image can be odd width or odd height, mipmap will complain.
        //it's better not to use odd width/height image for performance reasons.
        unsigned char * pixels = (unsigned char*)malloc(sizeof(unsigned char) * width * height * 3);
        if( MagickTrue == MagickGetImagePixels(wand, 0, 0, width, height, "RGB", CharPixel, pixels)) {
            //load the hdr file into envMap
            // cross is 4 faces wide, 3 faces high, MUST be square
            int face_height = (int)height / 3;
            int face_width = (int)width / 4;
            face_height = face_height > face_width? face_width: face_height;
            face_width = face_height;
            unsigned char* face = new unsigned char [face_width * face_height * 3];
            
            glGenTextures(1, &m_reflectionTextureObj);
            glBindTexture(GL_TEXTURE_CUBE_MAP, m_reflectionTextureObj);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            
            // extract 6 faces
            
            // positive Y, Top
            int ptr = 0;
            for (int j=0; j<face_height; j++) {
                for (int i=0; i<face_width; i++) {
                    int src = getPixelIndex(face_width+i, j, width, height);
                    face[ptr++] = pixels[src + 0];
                    face[ptr++] = pixels[src + 1];
                    face[ptr++] = pixels[src + 2];
                }
            }
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB, face_width, face_height, 0, GL_RGB, GL_UNSIGNED_BYTE, face);
            
            // positive X, right
            ptr = 0;
            for (int j=0; j<face_height; j++) {
                for (int i=0; i<face_width; i++) {
                    int src = getPixelIndex(2 * face_width + i, face_height + j,  width, height);
                    face[ptr++] = pixels[src + 0];
                    face[ptr++] = pixels[src + 1];
                    face[ptr++] = pixels[src + 2];
                }
            }
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB, face_width, face_height, 0, GL_RGB, GL_UNSIGNED_BYTE, face);
            
            // negative Z, back
            ptr = 0;
            for (int j=0; j<face_height; j++) {
                for (int i=0; i<face_width; i++) {
                    int src = getPixelIndex(3 * face_width + i, face_height + j, width, height);
                    face[ptr++] = pixels[src + 0];
                    face[ptr++] = pixels[src + 1];
                    face[ptr++] = pixels[src + 2];
                }
            }
            glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB, face_width, face_height, 0, GL_RGB, GL_UNSIGNED_BYTE, face);
            
            // negative X, Left
            ptr = 0;
            for (int j=0; j<face_height; j++) {
                for (int i=0; i<face_width; i++) {
                    int src = getPixelIndex(i, face_height + j, width, height);
                    face[ptr++] = pixels[src + 0];
                    face[ptr++] = pixels[src + 1];
                    face[ptr++] = pixels[src + 2];
                }
            }
            glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB, face_width, face_height, 0, GL_RGB, GL_UNSIGNED_BYTE, face);

            // negative Y, Bottom
            ptr = 0;
            for (int j=0; j<face_height; j++) {
                for (int i=0; i<face_width; i++) {
                    int src = getPixelIndex(face_width+i, 2*face_height+j, width, height);
                    face[ptr++] = pixels[src + 0];
                    face[ptr++] = pixels[src + 1];
                    face[ptr++] = pixels[src + 2];
                }
            }
            glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB, face_width, face_height, 0, GL_RGB, GL_UNSIGNED_BYTE, face);
            
            // positive Z, front
            ptr = 0;
            for (int j=0; j<face_height; j++) {
                for (int i=0; i<face_width; i++) {
                    int src = getPixelIndex(face_width+i, face_height+j, width, height);
                    face[ptr++] = pixels[src + 0];
                    face[ptr++] = pixels[src + 1];
                    face[ptr++] = pixels[src + 2];
                }
            }
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB, face_width, face_height, 0, GL_RGB, GL_UNSIGNED_BYTE, face);
            
            glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
            
            delete[] face;
        }
        free(pixels);
    } else {
      /*
        // Handle the error
        char error_string[256];
        char *description;
        ExceptionType severity;
        
        description=MagickGetException(wand,&severity);
        FormatLocaleString(error_string,250,"%s %s %lu %s\n",GetMagickModule(),description);
        MagickRelinquishMemory(description);
        OUTPUT("Warning: cannot read image. error=%s\n", error_string);
      */
      OUTPUT("Warning: cannot read image. reflectiontexure.");
    }
    
    DestroyMagickWand(wand);
    MagickWandTerminus();
    
    return true;
}

void ReflectionTexture::bind(GLint textureId)
{
    glUniform1i(m_texCountLocation, 2);
    glUniform4f(m_diffuseColorLocation, m_diffuseColor.x, m_diffuseColor.y, m_diffuseColor.z, m_diffuseColor.w);
    glUniform4f(m_ambientColorLocation, m_ambientColor.x, m_ambientColor.y, m_ambientColor.z, m_ambientColor.w);
    glUniform4f(m_specularColorLocation, m_specularColor.x, m_specularColor.y, m_specularColor.z, m_specularColor.w);
    
    glActiveTexture(GL_TEXTURE0 + textureId);
    glBindTexture(GL_TEXTURE_2D, m_textureObj);
    glUniform1i(m_textureImageLocation, textureId); //set the sampler texture to textureId
    
    glActiveTexture(GL_TEXTURE0 + textureId + 1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_reflectionTextureObj);
    glUniform1i(m_reflectionTextureImageLocation, textureId+1); //set the reflection texture to textureId

}
