#include "VideoMixer.h"
#include "fwk/log.h"
#include <assert.h>

int mappingToScalingWidth(int totalStream) {
    if(totalStream == 1) {
        return 640;
    } else {
        return 320;
    }
}
int mappingToScalingHeight(int totalStream) {
    if(totalStream == 1) {
        return 480;
    } else {
        return 240;
    }
}

//do the mixing, for now, always mix n raw streams into 1 rawstream
SmartPtr<SmartBuffer> VideoMixer::mixStreams(SmartPtr<SmartBuffer> planes[][3],
                                             int strides[][3],
                                             VideoStreamSetting* settings, 
                                             int totalStreams,
                                             VideoRect* videoRect)
{
    SmartPtr<SmartBuffer> result;
    if( tryToInitSws(settings, totalStreams) ) {
        int scaledWidth = mappingToScalingWidth(totalStreams);
        int scaledHeight = mappingToScalingHeight(totalStreams);

        SmartPtr<SmartBuffer> scaledVideoPlanes[MAX_XCODING_INSTANCES][3];
        int scaledVideoStrides[MAX_XCODING_INSTANCES][3];

        int validStreamId[totalStreams];
        int validStreamIdIndex = 0;
        
        for(u32 i=0; i<MAX_XCODING_INSTANCES; i++) {
            if( settings[i].bIsValid ) { 
                scaledVideoPlanes[i][0] = new SmartBuffer( scaledWidth* scaledHeight );
                scaledVideoPlanes[i][1] = new SmartBuffer( ( scaledWidth * scaledHeight  + 4) / 4 );
                scaledVideoPlanes[i][2] = new SmartBuffer( ( scaledWidth * scaledHeight  + 4) / 4 );
                scaledVideoStrides[i][0] = scaledWidth;
                scaledVideoStrides[i][1] = scaledWidth/2;
                scaledVideoStrides[i][2] = scaledWidth/2;
                
                u8* inputPlanes[3];
                inputPlanes[0] = planes[i][0]->data();
                inputPlanes[1] = planes[i][1]->data();
                inputPlanes[2] = planes[i][2]->data();

                u8* scaledPlanes[3];
                scaledPlanes[0] = scaledVideoPlanes[i][0]->data();
                scaledPlanes[1] = scaledVideoPlanes[i][1]->data();
                scaledPlanes[2] = scaledVideoPlanes[i][2]->data();
                sws_scale( swsCtx_[i], inputPlanes, strides[i], 0, settings[i].height,
                           scaledPlanes, scaledVideoStrides[i]);
                validStreamId[validStreamIdIndex++] = i;
                assert(validStreamIdIndex <= totalStreams);
            }
        }
        assert( validStreamIdIndex == totalStreams );

        if ( totalStreams > 0 ) {
            int outputWidth = outputSetting_.width;
            int outputHeight = outputSetting_.height;
            result = new SmartBuffer( outputWidth*outputHeight*3/2 );
            u8* out = result->data();
            u32 offsetOut = 0;

            if( totalStreams == 1 ) {
                int curStreamId = validStreamId[0];
                assert( curStreamId != -1 );

                if(videoRect) {
                    videoRect[curStreamId].x = 0;
                    videoRect[curStreamId].y = 0;
                    videoRect[curStreamId].width = outputWidth;
                    videoRect[curStreamId].height = outputHeight;
                    //LOG("----i=%d, x=%d, y=%d, w=%d,h=%d\r\n", curStreamId, videoRect[curStreamId].x, videoRect[curStreamId].y, videoRect[curStreamId].width, videoRect[curStreamId].height);
                }
                    
                //convert from AV_PIX_FMT_YUV420P
                //3 planes combined into 1 buffer
                u8* in = scaledVideoPlanes[curStreamId][0]->data();            
                u32 bytesPerLineInY = scaledVideoStrides[curStreamId][0];                                                                                                                            
                u32 offsetInY = 0;                                                                                                                                                                                         
                for(int i = 0; i < outputHeight; i ++ ) {          
                    memcpy( out+offsetOut, in+offsetInY, outputWidth);
                    offsetInY += bytesPerLineInY;
                    offsetOut += outputWidth;
                }                                                                                                                                                                                              
                in = scaledVideoPlanes[curStreamId][1]->data();
                u32 bytesPerLineInU = scaledVideoStrides[curStreamId][1];
                u32 offsetInU = 0;                                                                                                                                                                                         
                for(int i = 0; i < outputHeight/2; i ++ ) {
                    memcpy( out+offsetOut, in+offsetInU, outputWidth/2);                                                                
                    offsetInU += bytesPerLineInU;
                    offsetOut += outputWidth/2;                                                                                                                                                               
                }                                                       
                in = scaledVideoPlanes[curStreamId][2]->data();
                u32 bytesPerLineInV = scaledVideoStrides[curStreamId][2];
                u32 offsetInV = 0; 
                for(int i = 0; i < outputHeight/2; i ++ ) {
                    memcpy( out+offsetOut, in+offsetInV, outputWidth/2);
                    offsetInV += bytesPerLineInV;
                    offsetOut += outputWidth/2;
                }
            } else if( totalStreams == 2 ) {                
                int startingOffsetY = 0;
                int startingOffsetUV = 0;
                //somehow it's green
                int blackY = 16;
                memset(out, blackY, outputWidth*outputHeight);
                int blackUV = 128;
                memset(out+outputWidth*outputHeight, blackUV, outputWidth*outputHeight/2);
                
                for(int index=0; index<totalStreams; index++) {

                    int curStreamId = validStreamId[index];

                    if(videoRect) {
                        videoRect[curStreamId].x = startingOffsetY%outputWidth;
                        videoRect[curStreamId].y = outputHeight/4;
                        videoRect[curStreamId].width = scaledWidth;
                        videoRect[curStreamId].height = scaledHeight;
                        //LOG("----i=%d, x=%d, y=%d, w=%d,h=%d\r\n", curStreamId, videoRect[curStreamId].x, videoRect[curStreamId].y, videoRect[curStreamId].width, videoRect[curStreamId].height);
                    }

                    //convert from AV_PIX_FMT_YUV420P
                    //3 planes combined into 1 buffer
                    offsetOut = outputWidth*outputHeight/4 + startingOffsetY;
                    u8* in = scaledVideoPlanes[curStreamId][0]->data();            
                    u32 bytesPerLineInY = scaledVideoStrides[curStreamId][0];                                                                                                                            
                    u32 offsetInY = 0;                                                                                                                                                                                         
                    for(int i = 0; i < scaledHeight; i ++ ) {          
                        memcpy( out+offsetOut, in+offsetInY, scaledWidth);
                        offsetInY += bytesPerLineInY; 
                        offsetOut += outputWidth;                                                                                                                                                  
                    }
             
                    offsetOut = outputWidth*outputHeight*17/16 + startingOffsetUV;
                    in = scaledVideoPlanes[curStreamId][1]->data();
                    u32 bytesPerLineInU = scaledVideoStrides[curStreamId][1];
                    u32 offsetInU = 0;                                                                                                                                                                                         
                    for(int i = 0; i < scaledHeight/2; i ++ ) {
                        memcpy( out+offsetOut, in+offsetInU, scaledWidth/2);                                                                
                        offsetInU += bytesPerLineInU;
                        offsetOut += outputWidth/2;                                                                                                                                              
                    }                                                     

                    offsetOut = outputWidth*outputHeight*21/16 + startingOffsetUV;  
                    in = scaledVideoPlanes[curStreamId][2]->data();
                    u32 bytesPerLineInV = scaledVideoStrides[curStreamId][2];
                    u32 offsetInV = 0; 
                    for(int i = 0; i < scaledHeight/2; i ++ ) {
                        memcpy( out+offsetOut, in+offsetInV, scaledWidth/2);
                        offsetInV += bytesPerLineInV;
                        offsetOut += outputWidth/2;
                    }
                    startingOffsetY += scaledWidth;
                    startingOffsetUV += scaledWidth/2;                    
                } 
            } else if( totalStreams == 3 ) {                
                int startingOffsetY = 0;
                int startingOffsetUV = 0;
                //somehow it's green
                int blackY = 16;
                memset(out, blackY, outputWidth*outputHeight);
                int blackUV = 128;
                memset(out+outputWidth*outputHeight, blackUV, outputWidth*outputHeight/2);
                
                for(int index=0; index<totalStreams; index++) {
                    int curStreamId = validStreamId[index];
                    if(videoRect) {
                        videoRect[curStreamId].x = startingOffsetY%outputWidth;
                        videoRect[curStreamId].y = outputHeight/4;
                        videoRect[curStreamId].width = scaledWidth;
                        videoRect[curStreamId].height = scaledHeight;
                        //LOG("----i=%d, x=%d, y=%d, w=%d,h=%d\r\n", curStreamId, videoRect[curStreamId].x, videoRect[curStreamId].y, videoRect[curStreamId].width, videoRect[curStreamId].height);
                    }

                    //convert from AV_PIX_FMT_YUV420P
                    //3 planes combined into 1 buffer
                    offsetOut = startingOffsetY;
                    u8* in = scaledVideoPlanes[curStreamId][0]->data();            
                    u32 bytesPerLineInY = scaledVideoStrides[curStreamId][0];                                                                                                                            
                    u32 offsetInY = 0;                                                                                                                                                            
                    for(int i = 0; i < scaledHeight; i ++ ) {          
                        memcpy( out+offsetOut, in+offsetInY, scaledWidth);
                        offsetInY += bytesPerLineInY;                                                                                                               
                        offsetOut += outputWidth;                                                                                                                                   
                    }
             
                    offsetOut = outputWidth*outputHeight + startingOffsetUV;
                    in = scaledVideoPlanes[curStreamId][1]->data();
                    u32 bytesPerLineInU = scaledVideoStrides[curStreamId][1];
                    u32 offsetInU = 0;                                                                                                                                                              
                    for(int i = 0; i < scaledHeight/2; i ++ ) {
                        memcpy( out+offsetOut, in+offsetInU, scaledWidth/2);                                                                
                        offsetInU += bytesPerLineInU;
                        offsetOut += outputWidth/2;       
                    }                                                     

                    offsetOut = outputWidth*outputHeight*5/4 + startingOffsetUV;  
                    in = scaledVideoPlanes[curStreamId][2]->data();
                    u32 bytesPerLineInV = scaledVideoStrides[curStreamId][2];
                    u32 offsetInV = 0; 
                    for(int i = 0; i < scaledHeight/2; i ++ ) {
                        memcpy( out+offsetOut, in+offsetInV, scaledWidth/2);
                        offsetInV += bytesPerLineInV;
                        offsetOut += outputWidth/2;
                    }

                    
                    if((index + 1) == 2) {
                        startingOffsetY = outputWidth*outputHeight/2+scaledWidth/2;
                        startingOffsetUV = outputWidth*outputHeight/8+scaledWidth/4;
                    } else {
                        startingOffsetY += scaledWidth;
                        startingOffsetUV += scaledWidth/2;                    
                    }
                }
            } else if( totalStreams == 4 ) {
         
                int startingOffsetY = 0;
                int startingOffsetUV = 0;
                //somehow it's green
                int blackY = 16;
                memset(out, blackY, outputWidth*outputHeight);
                int blackUV = 128;
                memset(out+outputWidth*outputHeight, blackUV, outputWidth*outputHeight/2);
                
                for(int index=0; index<totalStreams; index++) {
                    int curStreamId = validStreamId[index];
                    if(videoRect) {
                        videoRect[curStreamId].x = startingOffsetY%outputWidth;
                        videoRect[curStreamId].y = outputHeight/4;
                        videoRect[curStreamId].width = scaledWidth;
                        videoRect[curStreamId].height = scaledHeight;
                        //LOG("----i=%d, x=%d, y=%d, w=%d,h=%d\r\n", 
                        //curStreamId, videoRect[curStreamId].x, videoRect[curStreamId].y, videoRect[curStreamId].width, videoRect[curStreamId].height);
                    }
                    //convert from AV_PIX_FMT_YUV420P
                    //3 planes combined into 1 buffer
                    offsetOut = startingOffsetY;
                    u8* in = scaledVideoPlanes[curStreamId][0]->data();            
                    u32 bytesPerLineInY = scaledVideoStrides[curStreamId][0];                                                                                                                            
                    u32 offsetInY = 0;                                                                                                                                                             
                    for(int i = 0; i < scaledHeight; i ++ ) {          
                        memcpy( out+offsetOut, in+offsetInY, scaledWidth);
                        offsetInY += bytesPerLineInY;                                                                                                                     
                        offsetOut += outputWidth;                                                                                                                      
                    }

                    offsetOut = outputWidth*outputHeight + startingOffsetUV;  
                    in = scaledVideoPlanes[curStreamId][1]->data();
                    u32 bytesPerLineInU = scaledVideoStrides[curStreamId][1];
                    u32 offsetInU = 0;                                                                                                                                                                
                    for(int i = 0; i < scaledHeight/2; i ++ ) {
                        memcpy( out+offsetOut, in+offsetInU, scaledWidth/2);                                                                
                        offsetInU += bytesPerLineInU;
                        offsetOut += outputWidth/2;                                                                                                                                               
                    }                                                     

                    offsetOut = outputWidth*outputHeight*5/4 + startingOffsetUV;  
                    in = scaledVideoPlanes[curStreamId][2]->data();
                    u32 bytesPerLineInV = scaledVideoStrides[curStreamId][2];
                    u32 offsetInV = 0; 
                    for(int i = 0; i < scaledHeight/2; i ++ ) {
                        memcpy( out+offsetOut, in+offsetInV, scaledWidth/2);
                        offsetInV += bytesPerLineInV;
                        offsetOut += outputWidth/2;
                    }
                    
                    if((index + 1) == 2) {
                        startingOffsetY = outputWidth*outputHeight/2;
                        startingOffsetUV = outputWidth*outputHeight/8;
                    } else {
                        startingOffsetY += scaledWidth;
                        startingOffsetUV += scaledWidth/2;                    
                    }
                }
            } else {
                //TODO do mixing for other cases
            }
        }
    }
    return result;
}
VideoMixer::~VideoMixer()
{
    releaseSws();
}
void VideoMixer::releaseSws()
{
    for(u32 i=0; i<MAX_XCODING_INSTANCES; i++) {
        if( swsCtx_[i] ) {
            sws_freeContext( swsCtx_[i] );
            swsCtx_[i] = 0;
        }
    }
}
bool VideoMixer::tryToInitSws(VideoStreamSetting* settings, int totalStreams)
{
    bool ret = true;
    int outputWidth = mappingToScalingWidth(totalStreams);
    int outputHeight = mappingToScalingHeight(totalStreams);

    if( mappingToScalingWidth(totalStreams_) != outputWidth ) {
        releaseSws();
    }
    for(u32 i=0;  i<MAX_XCODING_INSTANCES; i++) {
        if ( settings[i].bIsValid && !swsCtx_[i] ) {
            swsCtx_[i] = sws_getCachedContext( swsCtx_[i], settings[i].width, settings[i].height, PIX_FMT_YUV420P,
                                               outputWidth, outputHeight, PIX_FMT_YUV420P,
                                               SWS_BICUBIC, 0, 0, 0 );
            if( !swsCtx_[i] ) {
                LOG("FAILED to create swscale context, inWidth=%d, inHeight=%d, outWith=%d, outHeight=%d\n", settings[i].width, settings[i].height, outputWidth, outputHeight);
                assert(0);
                ret = false;
            }
        }
    }
    totalStreams_ = totalStreams;
    return ret;
}
