 //
//  glasses.cpp
//  Laputa
//
//  Created by Howard Wang on 15-5-13.
//
//

#include <stdio.h>
#include "glasses.h"
#include "mesh.h"
#include "err.h"
#include <glm/gtc/type_ptr.hpp>
#include "Output.h"

const int AA_LEVEL = 8; //4 is normal, 0 means no AA, max 8 on Mac, mesa does not support Anti-aliasing.

Glasses::Glasses(int srcWidth, int srcHeight, bool bEnableAA):_srcWidth(srcWidth), _srcHeight(srcHeight), _enableAA(bEnableAA)
{
    _pMesh = new Mesh();
}

Glasses::~Glasses()
{
    deinit();
    delete(_pMesh);
}

void Glasses::setMatrices(mat4& projectMat, mat4& rotTransMat) {
    _Projection = projectMat;
    
    mat4 Model_rotateX = rotate(mat4(1.0f), radians(10.0f), vec3(1,0,0)); //rotate x of 10 degree to align to nose
    _World = rotTransMat * Model_rotateX;
    _NormalMatrix = transpose(inverse(mat3(_World))); //remove translation and scaling
    _View       = lookAt(vec3(0,0,0.01), // Camera is at (0, 0, 0.01), in World Space
                         vec3(0,0,0), // and looks at the origin
                         vec3(0,1,0)  // Head is up (set to 0,-1,0 to look upside-down)
                        );
}

#if defined(THREED_MODEL_ONLY)
void Glasses::setMatricesWithYRotation(mat4& projectMat, mat4& rotTransMat, float yRotateInDeg) {
    _Projection = projectMat;
    
    float scaleFactor = 7.0f;
    mat4 Model_scale = scale(mat4(1.0f), vec3(scaleFactor,scaleFactor,scaleFactor));
    mat4 Model_rotateX = rotate(mat4(1.0f), radians(10.0f), vec3(1,0,0)); //rotate x of 10 degree to align to nose
    mat4 Model_rotateY = rotate(mat4(1.0f), radians(yRotateInDeg), vec3(0,1,0)); //rotate y
    
    _World = rotTransMat * Model_rotateX * Model_rotateY * Model_scale;
    _NormalMatrix = transpose(inverse(mat3(_World))); //remove translation and scaling
    _View       = lookAt(vec3(0,0,0.01), // Camera is at (0, 0, 0.01), in World Space
                         vec3(0,0,0), // and looks at the origin
                         vec3(0,1,0)  // Head is up (set to 0,-1,0 to look upside-down)
                         );
}
#endif

bool Glasses::init(const char* vertLFilePath,
                   const char* fragLFilePath,
                   const char *fragColorLName,
                   const char* glassesFilePath,
                   const char* candide3FacePath,
                   const char* candide3VertPath,
                   float zRotateInDegree, ASPECT_RATIO ratio,
                   bool bUploadCandide3Vertices, vector<myvec3>* candide3Vec)
{
    bool ret = false;
    
    char* vertLSrc = readAllocFile(vertLFilePath);
    char* fragLSrc = readAllocFile(fragLFilePath);
    
    _zRotationInDegree = zRotateInDegree;
    _aspectRatio = ratio;
    
    /////////////////////
    // offscreen buffer
    /////////////////////
    glEnable(GL_CULL_FACE);  //enable culling to speed up rendering.
    glEnable(GL_DEPTH_TEST); //MUST enable depth buffer
    glEnable(GL_DITHER); //enable dithering.
    glEnable(GL_BLEND); //enable blending
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
#if defined( DESKTOP_GL )
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_MULTISAMPLE_ARB);
    glDepthMask(GL_TRUE);
    //glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
#endif //DESKTOP_GL
    
    //offscreen framebuffer
    glGenFramebuffers( 1, &_offscreenBufferHandle );
    glBindFramebuffer( GL_FRAMEBUFFER, _offscreenBufferHandle );
    
    glGenRenderbuffers(1, &_depthRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, _depthRenderbuffer);
    
#if defined(DESKTOP_GL)
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, AA_LEVEL, GL_DEPTH32F_STENCIL8, _srcWidth, _srcHeight);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depthRenderbuffer);
    
    glGenRenderbuffers(1, &_aaColorbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, _aaColorbuffer);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, AA_LEVEL, GL_RGBA, _srcWidth, _srcHeight);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, _aaColorbuffer);
    
    /*
    glGenTextures(1, &_aaTexturebuffer);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, _aaTexturebuffer);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, AA_LEVEL, GL_RGBA, _srcWidth, _srcHeight, GL_TRUE);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
    glFramebufferTexture2D( GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, _aaTexturebuffer, 0 );
    */
    
    //input frame buffer
    glGenFramebuffers( 1, &_inputBufferHandle );
    glBindFramebuffer( GL_FRAMEBUFFER, _inputBufferHandle );
#else //DESKTOP_GL
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, _srcWidth, _srcHeight);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
#endif //DESKTOP_GL
    
    /////////////////
    // shader program
    /////////////////
    //glasses shaders
    // Load vertex and fragment shaders
    GLint attribLocation[NUM_ATTRIBUTES] = {
        ATTRIB_POSITION, // "position" in vertext shader
        ATTRIB_TEXCOORD, // "TexCoord" in vertext shader
        ATTRIB_NORMAL, // "normal" in vertext shader,
    };
    GLchar *attribName[NUM_ATTRIBUTES] = {
        (GLchar *)"position",
        (GLchar *)"texCoord",
        (GLchar *)"normal",
    };
    GLint uniformLocation[NUM_UNIFORMS];
    GLchar *uniformName[NUM_UNIFORMS] = {
        (GLchar *)"MVP",
        (GLchar *)"World",
        (GLchar *)"NormalMatrix",
        (GLchar *)"texCount",
        (GLchar *)"diffuseColor",
        (GLchar *)"ambientColor",
        (GLchar *)"textureImage",
        (GLchar *)"envMap",
    };
    
    // Load vertex and fragment shaders
    // Load vertex and fragment shaders for glasses    
    glueCreateProgram( vertLSrc, fragLSrc, fragColorLName,
                      NUM_ATTRIBUTES, (const GLchar **)&attribName[0], attribLocation,
                      NUM_UNIFORMS, (const GLchar **)&uniformName[0], uniformLocation,
                      &_programID );
    if ( _programID ) {
        _matrixMVP = uniformLocation[UNIFORM_MVP];
        _matrixWorld = uniformLocation[UNIFORM_WORLD];
        _matrixNormalMatrix = uniformLocation[UNIFORM_NORMALMATRIX];
        
        _pMesh->setAttrUni(uniformLocation[UNIFORM_TEXCOUNT], uniformLocation[UNIFORM_DIFFUSECOLOR], uniformLocation[UNIFORM_AMBIENTCOLOR],
                           uniformLocation[UNIFORM_TEXTUREIMAGE], uniformLocation[UNIFORM_ENVMAP],
                           attribLocation[ATTRIB_POSITION], attribLocation[ATTRIB_TEXCOORD], attribLocation[ATTRIB_NORMAL]);
        
        ////////////////////////
        //Load model with ASSIMP
        ////////////////////////
        ret = _pMesh->LoadMesh(glassesFilePath, candide3FacePath, candide3VertPath, zRotateInDegree, bUploadCandide3Vertices, candide3Vec);
    }
    
    free(vertLSrc);
    free(fragLSrc);
    
    return ret;
}


bool Glasses::reloadGlasses(const char* glassesFilePath)
{
    return _pMesh->reloadMesh(glassesFilePath, _zRotationInDegree);
}

void Glasses::deinit()
{
    if ( _offscreenBufferHandle ) {
        glDeleteFramebuffers( 1, &_offscreenBufferHandle );
        _offscreenBufferHandle = 0;
    }
    
#if defined( DESKTOP_GL )
    if ( _inputBufferHandle ) {
        glDeleteFramebuffers( 1, &_inputBufferHandle );
        _inputBufferHandle = 0;
    }
    if( _aaColorbuffer ) {
        glDeleteRenderbuffers(1, &_aaColorbuffer);
        _aaColorbuffer = 0;
    }
    /*
    if( _aaTexturebuffer ) {
        glDeleteTextures(1, &_aaTexturebuffer);
        _aaTexturebuffer = 0;
    }
    */
#endif //DESKTOP_GL
    if ( _depthRenderbuffer ) {
        glDeleteFramebuffers( 1, &_depthRenderbuffer );
        _depthRenderbuffer = 0;
    }
    if ( _programID ) {
        glDeleteProgram( _programID );
        _programID = 0;
    }
}

bool Glasses::render(GLuint dstTextureName, GLuint candide3Texture, bool shouldRotate)
{
    bool ret = false;
    if ( _offscreenBufferHandle != 0 ) {
        
        //////////////////////
        //Draw the lens
        //////////////////////
        glViewport( 0, 0, _srcWidth, _srcHeight);
        
#if defined(DESKTOP_GL)
        //Somehow OpenGL does no support attaching texture as a read and write buffer. (In the background)
        //We have to create two frame buffers, 1 for read and 1 for write in order to do it properly.
        //
        //Step 1. Bind a framebuffer for read
        glBindFramebuffer( GL_READ_FRAMEBUFFER, _inputBufferHandle );
        // Set up our destination pixel buffer as the framebuffer's render target.
        glActiveTexture( GL_TEXTURE0 );
        glBindTexture( GL_TEXTURE_2D, dstTextureName );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
        glBindTexture( GL_TEXTURE_2D, 0 );
        glFramebufferTexture2D( GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, dstTextureName, 0 );
        
        //Step 2. Bind a framebuffer for write
        if( _enableAA ) {
            glBindFramebuffer( GL_DRAW_FRAMEBUFFER, _offscreenBufferHandle );
        } else {
            glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 ); //bind to target framebuffer directly.
        }
        //glClear is ALWAYS associated with a framebuffer, should clear it here instead of outside.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        //Step 3. copy from read buffer to write buffer
        glBlitFramebuffer(0, 0, _srcWidth, _srcHeight, 0, 0, _srcWidth, _srcHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        
#else //DESKTOP_GL
        //Bind a framebuffer
        glBindFramebuffer( GL_FRAMEBUFFER, _offscreenBufferHandle );
        // Set up our destination pixel buffer as the framebuffer's render target.
        glActiveTexture( GL_TEXTURE0 );
        glBindTexture( GL_TEXTURE_2D, dstTextureName );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
        glBindTexture( GL_TEXTURE_2D, 0 );
        glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, dstTextureName, 0 );
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depthRenderbuffer);
#endif //DESKTOP_GL
        
        GLenum framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if( framebufferStatus == GL_FRAMEBUFFER_COMPLETE ) {
            glUseProgram( _programID );
            
        #if !defined(DESKTOP_GL)
            ////////////////////////////////////////////
            //TODO TEST CODE for Mobile to Set the matrices
            ////////////////////////////////////////////
            float ratioW = 0;
            float ratioH = 0;
            if( _aspectRatio == ASPECT_RATIO_4_3 ) {
                ratioW = 12;
                ratioH = 9;
            } else if( _aspectRatio == ASPECT_RATIO_16_9 ){
                ratioW = 16;
                ratioH = 9;
            } else {
                //assume it's 1:1
                ratioW = 12;
                ratioH = 12;
            }
            
            // Projection matrix : 45° Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
            _Projection = perspective(radians(45.0f), ratioW/ratioH, 0.1f, 100.0f); //for portrait mode, front/back camera, is: 16:9
            // Or, for an ortho camera :
            //mat4 Projection = ortho(-ratioW/2,ratioW/2,-ratioH/2,ratioH/2,0.0f,100.0f); // In world coordinates, x/y =16/9 ratio, far-near is big enough
            
            // Camera matrix
            _View = lookAt(vec3(0,0,0.01), // Camera is at (0, 0, 0.01), in World Space
                                     vec3(0,0,0), // and looks at the origin
                                     vec3(0,1,0)  // Head is up (set to 0,-1,0 to look upside-down)
                                     );
            
            //below is the test code to transformation
            static float angleInDegree = 0.0f;
            static int sign = -1;
            if(angleInDegree >= 60) {
                sign = -1;
            } else if(angleInDegree <= -60) {
                sign = 1;
            }
            angleInDegree += sign;

            mat4 Model_rotateX = rotate(mat4(1.0f), radians(10.0f), vec3(1,0,0)); //rotate x of 10 degree
            mat4 Model_rotateY = rotate(mat4(1.0f), radians(angleInDegree), vec3(0,1,0)); //rotate x of 10 degree
            mat4 Model_rotateZ = rotate(mat4(1.0f), radians((float)_zRotationInDegree), vec3(0,0,1)); //rotate z of 90 degree
            
            // Model matrix : an identity matrix (model will be at the origin)
            float scaleFactor = ((_zRotationInDegree == 90)?ratioH * 0.7:ratioW * 1/3)/_pMesh->getWidth(); //put the object width the same as portaint mode 9:16
            mat4 Model_scale = scale(mat4(1.0f), vec3(scaleFactor,scaleFactor,scaleFactor));
            
            if( shouldRotate ) {
                _World = Model_rotateZ * Model_rotateX * Model_rotateY * Model_scale;
            } else {
                _World = Model_rotateZ * Model_rotateX * Model_scale;
            }
            _NormalMatrix = transpose(inverse(mat3(_World))); //rotation and scaling, w/o
        #endif //!DESKTOP_GL
            
            mat4 curMVP = _Projection * _View * _World;
            glUniformMatrix4fv(_matrixMVP, 1, GL_FALSE, &curMVP[0][0]);
            glUniformMatrix4fv(_matrixWorld, 1, GL_FALSE, &_World[0][0]);
            glUniformMatrix3fv(_matrixNormalMatrix, 1, GL_FALSE, &_NormalMatrix[0][0]);
            
            //render the meshes
            _pMesh->Render(candide3Texture);
            
            // Make sure that outstanding GL commands which render to the destination pixel buffer have been submitted.
            // AVAssetWriter, AVSampleBufferDisplayLayer, and GL will block until the rendering is complete when sourcing from this pixel buffer.
            glFlush();
            
#if defined(DESKTOP_GL)
            if( _enableAA ) {
                //last blit multisample framebuffer to normal framebuffer 0
                glBindFramebuffer( GL_READ_FRAMEBUFFER, _offscreenBufferHandle );
                glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 );
                if( glCheckFramebufferStatus(GL_READ_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE &&
                    glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE ) {
                    //do blitting here
                    glBlitFramebuffer(0, 0, _srcWidth, _srcHeight, 0, 0, _srcWidth, _srcHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);
                } else {
                    OUTPUT("render final fail:framebufferStatus=%d\r\n", framebufferStatus);
                }
            }
#endif//DESKTOP_GL
            
            ret = true;
        } else {
            OUTPUT("render offscreen fail:framebufferStatus=%d\r\n", framebufferStatus);
        }
        
    }
    return ret;
}

#if defined(DESKTOP_GL )
void Glasses::readPixels(unsigned char* pixels)
{
    glBindFramebuffer( GL_READ_FRAMEBUFFER, 0 );
    GLenum framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if( framebufferStatus == GL_FRAMEBUFFER_COMPLETE ) {
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        //getGLErr("GL_COLOR_ATTACHMENT0");
        glReadPixels(0, 0, _srcWidth, _srcHeight, GL_RGB, GL_UNSIGNED_BYTE, pixels);
        //getGLErr("glGetTexImage");
    } else {
        OUTPUT("readPixels:framebufferStatus=%d\r\n", framebufferStatus);
    }
}
#endif //DESKTOP_GL
