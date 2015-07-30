#include <iostream>
#include <vector>
#include <string>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "opencv2/imgproc/imgproc.hpp"

// GLFW
#include "GL/osmesa.h"

//glasses
#include "glasses.h"

using namespace std;
using namespace cv;

#define OPENGL_2_1 1

#define PI 3.14159265
#define NP 6 //number of parameters to track
#define V_WIDTH 640
#define V_HEIGHT 480
#define DIST 600 //to be verified, need to find the correct value

const int AA_FACTOR = 4;

const char* fragName = "outFrag";

Mat P2PMat(float P[]){
    Mat res = Mat::zeros(3, 4, CV_32F);
    float p0 = -P[0];
    res.at<float>(0,0) = cos(P[1]*PI)*cos(P[2]*PI);
    res.at<float>(0,1) = cos(p0*PI)*sin(P[2]*PI) + sin(p0*PI)*sin(P[1]*PI)*cos(P[2]*PI);
    res.at<float>(0,2) = sin(p0*PI)*sin(P[2]*PI) - cos(p0*PI)*sin(P[1]*PI)*cos(P[2]*PI);
    res.at<float>(0,3) = P[3]*V_WIDTH;
    res.at<float>(1,0) = -cos(P[1]*PI)*sin(P[2]*PI);
    res.at<float>(1,1) = cos(p0*PI)*cos(P[2]*PI) - sin(p0*PI)*sin(P[1]*PI)*sin(P[2]*PI);
    res.at<float>(1,2) = sin(p0*PI)*cos(P[2]*PI) + cos(p0*PI)*sin(P[1]*PI)*sin(P[2]*PI);
    res.at<float>(1,3) = -P[4]*V_HEIGHT;
    res.at<float>(2,0) = sin(P[1]*PI);
    res.at<float>(2,1) = -sin(p0*PI)*cos(P[1]*PI);
    res.at<float>(2,2) = cos(p0*PI)*cos(P[1]*PI);
    res.at<float>(2,3) = -P[5]*DIST;

    return res;
}

void mat3x4ToMat4(Mat* pMat3x4, glm::mat4& pMat4)
{
    //3 rows, 3 cols for x, y, z
    for (int i = 0; i < 3; ++i) {
        for(int j = 0; j< 4; ++j ) {
            pMat4[j][i] = pMat3x4->at<float>(i, j);
            //printf("mat3x4ToMat4: pMat4[%d][%d] = %f\r\n", i, j, pMat4[i][j]);
        }
    }
    pMat4[0][3] =  pMat4[1][3] = pMat4[2][3] = 0;
    pMat4[3][3] = 1;
}

glm::mat4 externalToRotTrans(float* P_arr)
{
    Mat mat3x4 = P2PMat(P_arr);
    glm::mat4 mat4;
    mat3x4ToMat4(&mat3x4, mat4);
    return mat4;
}

//according to article http://stackoverflow.com/questions/22064084/how-to-create-perspective-projection-matrix-given-focal-points-and-camera-princ
glm::mat4 IntrinsicToProjection(Mat* intrinsicMat, int W, int H)
{
    float fx = intrinsicMat->at<float>(0, 0);
    float fy = intrinsicMat->at<float>(1, 1);
    float cx = intrinsicMat->at<float>(0, 2);
    float cy = intrinsicMat->at<float>(1, 2);
    float zmax = 1000.0f;
    float zmin = 0.1f;
    
    glm::mat4 projectionMat = { fx/cx,0,0,0,
        0, fy/cy,0,0,
        0,0,-(zmax+zmin)/(zmax-zmin),-1,
        0,0,2*zmax*zmin/(zmin-zmax),0};
    return projectionMat;
}

static void saveBuffer(void* buffer, string& fileToSave, int srcWidth, int srcHeight) {
    //first flip the x axis
    uint8 *pImage_flipped_x = (uint8*)malloc( srcWidth * srcHeight * 4 );
    //convert from RGBA to BGRA
    uint8* src;
    uint8* dst;    

    if( pImage_flipped_x ) {
        for( int i = 0; i < srcHeight; i++) {
            for( int j = 0; j < srcWidth; j++ ) {
                src = (uint8*)buffer + ( (srcHeight-i)*srcWidth + j )*4;
                dst = pImage_flipped_x + (i * srcWidth + j) * 4;
                *dst = *(src+2);
                *(dst+1) = *(src+1);
                *(dst+2) = *src;
                *(dst+3) = *(src+3);
            }
        }
        
        //then resize the image
        Mat origImage( srcHeight, srcWidth, CV_8UC4, pImage_flipped_x);
        Mat finalImage;
        if( AA_FACTOR != 1 ) { 
            float ratio = (float)1/(float)AA_FACTOR;
            resize(origImage, finalImage, Size(), ratio, ratio, INTER_AREA);
            //finally save the image
            imwrite(fileToSave.c_str(), finalImage);
        } else {
            //finally save the image
            imwrite(fileToSave.c_str(), origImage);
        }
        free( pImage_flipped_x );
    } else {
        printf("OOM: srcWidth = %d, srcHeight=%d\r\n", srcWidth, srcHeight);        
    }
}

static void drawOpenGLGlasses(GLuint& dstTexture, Mat& frameOrig, Glasses& glasses, mat4& projectionMat, mat4& rotTransMat, float yRotateInDeg ) {
    Mat* frame = &frameOrig;

    ///////////////////////////////////////////////////////////////////////////////////
    //NEXT convert from mat to opengl texture
    //http://stackoverflow.com/questions/16809833/opencv-image-loading-for-opengl-texture
    ///////////////////////////////////////////////////////////////////////////////////
    glBindTexture(GL_TEXTURE_2D, dstTexture);
    flip(*frame, *frame, 0); //flip x
    glTexImage2D(GL_TEXTURE_2D,     // Type of texture
                 0,                 // Pyramid level (for mip-mapping) - 0 is the top level
                 GL_RGBA,           // Internal colour format to convert to
                 (*frame).cols,        // Image width  i.e. 640 for Kinect in standard mode
                 (*frame).rows,        // Image height i.e. 480 for Kinect in standard mode
                 0,                 // Border width in pixels (can either be 1 or 0)
                 GL_BGR,            // Input image format (i.e. GL_RGB, GL_RGBA, GL_BGR etc.)
                 GL_UNSIGNED_BYTE,  // Image data type
                 (*frame).ptr()); // The actual image data itself
    glBindTexture(GL_TEXTURE_2D, 0);
    
    //set new matrix
    glasses.setMatricesWithYRotation(projectionMat, rotTransMat, yRotateInDeg);
    
    //draw glasses
    glasses.render(dstTexture, dstTexture, false);
}

int main(int argc, char* argv[])
{
    string inputFile;
    string outputFile;
    string glassesFile;
    float yRotateInDeg;
    string glassesVsh;
    string glassesFsh;    
    string candide3VertexFile;
    string candide3FaceFile;
    if( argc == 6 ) {
        //Model file
        string pathPrefix = argv[5];
        string shaderFilePrefix = pathPrefix + "3dShaders/";
        string glassesFilePrefix = pathPrefix + "3dmodels/";
        candide3VertexFile = glassesFilePrefix + "Candide3/vertexlist_113.wfm";
        candide3FaceFile = glassesFilePrefix + "Candide3/facelist_184.wfm";
        inputFile = pathPrefix + "background/"+argv[1];
        outputFile = glassesFilePrefix + "result/"+argv[3]+"/"+argv[2];
        glassesFile = glassesFilePrefix+argv[3]+"/"+argv[3]+".obj";
        yRotateInDeg = (float)atoi(argv[4]);
        if( OPENGL_2_1 ) {
            glassesVsh = shaderFilePrefix + "3dGlassesVertexShaderGL2.1.vsh";
            glassesFsh = shaderFilePrefix + "3dGlassesFragmentShaderGL2.1.fsh";
        } else {
            glassesVsh = shaderFilePrefix + "3dGlassesVertexShaderGL.vsh";
            glassesFsh = shaderFilePrefix + "3dGlassesFragmentShaderGL.fsh";
        }
    } else {
        return 0;
    }
    
    Mat frame, frame_orig;
    frame_orig = imread( inputFile, CV_LOAD_IMAGE_COLOR);

    if( AA_FACTOR != 1 ) {
        //double the size
        resize(frame_orig, frame, Size(), AA_FACTOR, AA_FACTOR, INTER_CUBIC);
    } else {
        frame = frame_orig.clone();
    }
    Size srcSize = frame_orig.size();
    frame_orig.release();
    ASPECT_RATIO aspectRatio = ASPECT_RATIO_4_3;
    if( srcSize.width * 3 == srcSize.height * 4 ) {
        aspectRatio = ASPECT_RATIO_4_3;
        printf("image asepect ratio: 4/3\r\n");
    } else if(srcSize.width * 9 == srcSize.height * 16 ) {
        aspectRatio = ASPECT_RATIO_16_9;
        printf("image asepect ratio: 16/9\r\n");
    } else {
        printf("image asepect ratio: unknown\r\n");
    }
    
    //manually resize to achive aa
    const int srcWidth = srcSize.width * AA_FACTOR;
    const int srcHeight = srcSize.height * AA_FACTOR; 
    Glasses glasses(srcWidth, srcHeight, false);
    
    ////////////////
    //osmesa context
    ////////////////
    const GLint zdepth = 32; //24 bits z depth buffer
    const GLint stencil = 8; //8 bits stencil
    const GLint accum = 0; //accumulation buffer
    OSMesaContext ctx = OSMesaCreateContextExt( OSMESA_RGBA, zdepth, stencil, accum, NULL);
    if(!ctx) {
        printf("OSMesaCreateContextExt failed!\n");
        return 0;
    }
    void* buffer = malloc(srcWidth*srcHeight*4*sizeof(GLubyte));
    /* Bind the buffer to the context and make it current */
    if (!OSMesaMakeCurrent( ctx, buffer, GL_UNSIGNED_BYTE, srcWidth, srcHeight )) {
        printf("OSMesaMakeCurrent failed!\n");
        return 0;
    }
    
    ////////////////
    
    //------------Initialization Begin---------------
    Mat cam_int = (Mat_<float>(3,3) << 650.66, 0, 319.50, 0, 650.94, ( aspectRatio == ASPECT_RATIO_16_9 )?179.5:239.5, 0, 0, 1);
    glm::mat4 projectionMat4 = IntrinsicToProjection(&cam_int, srcWidth, srcHeight);
        
    float P[NP] =      //space between glasses and candide3
    {
        0.0f, 0.0f, 0.0f,   //normalized rotation parameter
        0.0f, 0.0f, 1.0f,  // normalized translation parameter
    };
    
    ////////////////
    //opengl objects
    ////////////////
    // add error checking here
    GLuint dstTexture;
    glGenTextures(1, &dstTexture);
    
    glasses.init(glassesVsh.c_str(),
                 glassesFsh.c_str(),
                 fragName,
                 glassesFile.c_str(),
                 candide3FaceFile.c_str(),
                 candide3VertexFile.c_str(),
                 0, aspectRatio,
                 false, NULL ); //read adjusted coordinates directly from opengl.
    ////////////////
    
    //------------Initialization End---------------
    
    //only process once.
    glm::mat4 rotTransMat4 = externalToRotTrans(P);
    drawOpenGLGlasses(dstTexture, frame, glasses, projectionMat4, rotTransMat4, yRotateInDeg);
    frame.release();
    saveBuffer(buffer, outputFile, srcWidth, srcHeight);

    //------------Clean up-----------
    free( buffer );
    OSMesaDestroyContext( ctx );
    return 0;
}
