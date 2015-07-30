//
//  glasses.h
//  Laputa
//
//  Created by Howard Wang on 15-5-13.
//
//

#ifndef Laputa_glasses_h
#define Laputa_glasses_h

#include "ShaderUtilities.h"
#include "glUtilities.h"
#include "matrix.h"
#include "unit.h"
#include "platform.h"

//math library
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace glm;

class Mesh;

enum {
    ATTRIB_POSITION, // "position" in vertext shader
    ATTRIB_TEXCOORD, // "TexCoord" in vertext shader
    ATTRIB_NORMAL, // "normal" in vertext shader
    NUM_ATTRIBUTES
};

enum {
    UNIFORM_MVP, // "MVP" in vertext shader
    UNIFORM_WORLD, // "gWorld" in vertext shader
    UNIFORM_NORMALMATRIX, // "NormalMatrix" in vertext shader
    UNIFORM_TEXCOUNT,
    UNIFORM_DIFFUSECOLOR,
    UNIFORM_AMBIENTCOLOR,
    UNIFORM_TEXTUREIMAGE,
    UNIFORM_ENVMAP,
    NUM_UNIFORMS
};

typedef enum {
    ASPECT_RATIO_4_3,
    ASPECT_RATIO_16_9,
    ASPECT_RATIO_1_1
}ASPECT_RATIO;


class Glasses{
public:
    Glasses(int srcWidth, int srcHeight, bool bEnableAA);
    ~Glasses();
    
    bool init(const char* vertLFilePath,
              const char* fragLFilePath,
              const char* fragColorLName,
              const char* glassesFilePath,
              const char* candide3FacePath,
              const char* candide3VertPath,
              float zRotateInDegree, ASPECT_RATIO ratio,
              bool bUploadCandide3Vertices, vector<myvec3>* candide3Vec);
    
    bool reloadGlasses(const char* glassesFilePath);
    
    bool render(GLuint dstTextureName, GLuint candide3Texture, bool shouldRotate);
    
    void setMatrices(mat4& projectMat, mat4& rotTransMat);
    
#if defined(THREED_MODEL_ONLY)
    void setMatricesWithYRotation(mat4& projectMat, mat4& rotTransMat, float yRotateInDeg);
#endif
    
#if defined(DESKTOP_GL )
    void readPixels(unsigned char* pixels);
#endif //DESKTOP_GL
    
private:
    void deinit();
    
private:
    /*mesh*/
    Mesh* _pMesh;
    
    /* Opengles assets */
    GLuint _programID; //compiled shader program for glasses
    GLint _matrixMVP; //matrix for glasses in vertex shader
    GLint _matrixWorld; //matrix for glasses in vertex shader
    GLint _matrixNormalMatrix; //matrix for glasses in vertex shader
    mat4 _Projection; //projection matrix matrix for rotation
    mat4 _World; //world matrix for rotation		     mat4 _View; //view matrix for rotation
    mat3 _NormalMatrix; //normal Matrix matrix
    mat4 _View; //view matrix for rotation
    GLuint _offscreenBufferHandle; //offscreen buffer
    GLuint _depthRenderbuffer; //depth render buffer
    
    int _srcWidth;
    int _srcHeight;
    
    int _zRotationInDegree;
    
    bool _enableAA; //enable anti-aliasing or not
    
    ASPECT_RATIO _aspectRatio;
    
#if defined(DESKTOP_GL)
    //input framebuffer
    GLuint _inputBufferHandle; //input buffer
    
    //anti-aliased framebuffer
    GLuint _aaColorbuffer; //anti-aliasing color buffer
    //GLuint _aaTexturebuffer; //anti-aliasing texture buffer
#endif //DESKTOP_GL
};

#endif
