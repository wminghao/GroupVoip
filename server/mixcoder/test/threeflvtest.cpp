#include <stdio.h> 
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "flvrealtimeparser.h"

static const unsigned int FLV_START_POS = 13;
static const unsigned int FIXED_DATA_SIZE = 100; //must be small so that multiple videos are in sync

bool doWrite( int fd, const void *buf, size_t len ) {
  size_t bytesWrote = 0;
    
  while ( bytesWrote < len ) {
    size_t bytesToWrite = len - bytesWrote;
    if ( bytesToWrite > 41920 ) bytesToWrite = 41920;
        
    int t = write( fd, (const char *)buf + bytesWrote, bytesToWrite );
    if ( t <= 0 ) {
      return false;
    }
        
    bytesWrote += t;
  }
    
  return true;
}

long runTest(FILE* fd1, FILE* fd2, FILE* fd3)
{
    unsigned char bigBuf[MAX_BUFFER_SIZE];
    int totalLen = 0;
    int bufLen = 0;
    bool bFile1Done = false;
    bool bFile2Done = false;
    bool bFile3Done = false;
    FLVRealTimeParser realtimeParser1;
    FLVRealTimeParser realtimeParser2;
    FLVRealTimeParser realtimeParser3;
    unsigned char* result = NULL;
    while(!bFile1Done && !bFile2Done && !bFile3Done) {
        unsigned char* buffer = bigBuf;
        totalLen = 0;

        //first send the segHeader
        unsigned char metaData []= {'S', 'G', 'I', 0x0}; //even layout
        memcpy(buffer, metaData, sizeof(metaData));
        unsigned int mask3Stream = 0x92;
        memcpy(buffer+sizeof(metaData), &mask3Stream, sizeof(unsigned int));
        buffer += (sizeof(metaData)+sizeof(unsigned int));
        totalLen += (sizeof(metaData)+sizeof(unsigned int));

        //then send the stream heaer of stream 1                                                                                                                                                          
        result = realtimeParser1.readData(fd1, &bufLen);
        if ( result ) {
            //unsigned char streamIdSource[] = {0x09, 0x0}; //desktop stream
            unsigned char streamIdSource[] = {0x0a, 0x0}; //mobile stream
            memcpy(buffer, &streamIdSource, sizeof(streamIdSource));
            memcpy(buffer+sizeof(streamIdSource), &bufLen, sizeof(unsigned int));
            memcpy((char*)buffer+sizeof(streamIdSource)+sizeof(unsigned int), result, bufLen);
            buffer += (sizeof(streamIdSource)+sizeof(unsigned int)+bufLen);
            totalLen += (sizeof(streamIdSource)+sizeof(unsigned int)+bufLen);
        } else {
            bFile1Done = true;
        }

        //then send the stream heaer of stream 2                                                                                                                                                         
        result = realtimeParser2.readData(fd2, &bufLen);
        if ( result ) {        
            //then send the stream header of stream 2
            unsigned char streamIdSource[] = {0x22, 0};//mobile stream
            memcpy(buffer, &streamIdSource, sizeof(streamIdSource));
            memcpy(buffer+sizeof(streamIdSource), &bufLen, sizeof(unsigned int));
            memcpy((char*)buffer+sizeof(streamIdSource)+sizeof(unsigned int), result, bufLen);
            buffer += (sizeof(streamIdSource)+sizeof(unsigned int)+bufLen);
            totalLen += (sizeof(streamIdSource)+sizeof(unsigned int)+bufLen);
        } else {
            bFile2Done = true;
        }

        //then send the stream heaer of stream 3                                                                                                                                                         
        result = realtimeParser3.readData(fd3, &bufLen);
        if ( result ) {        
            unsigned char streamIdSource[] = {0x39, 0};//desktop stream
            memcpy(buffer, &streamIdSource, sizeof(streamIdSource));
            memcpy(buffer+sizeof(streamIdSource), &bufLen, sizeof(unsigned int));
            memcpy((char*)buffer+sizeof(streamIdSource)+sizeof(unsigned int), result, bufLen);
            buffer += (sizeof(streamIdSource)+sizeof(unsigned int)+bufLen);
            totalLen += (sizeof(streamIdSource)+sizeof(unsigned int)+bufLen);
        } else {
            bFile3Done = true;
        }

        doWrite(1, bigBuf, totalLen);
    }    
    return 1;
}

int main( int argc, char** argv ) {
  if ( argc != 4 ) {
      fprintf(stderr, "usage: %s input_file1 input_file2 input_file3", argv[0]);
      return -1;
  } 

  //////////////////
  FILE *fp1;
  long fileSize1;
  char *buffer1;

  fp1 = fopen(argv[1], "r");
  if (NULL == fp1) {
    /* Handle Error */
  }

  if (fseek(fp1, FLV_START_POS , SEEK_SET) != 0) {
    /* Handle Error */
  }

  //////////////////
  FILE *fp2;
  long fileSize2;
  char *buffer2;

  fp2 = fopen(argv[2], "r");
  if (NULL == fp2) {
    /* Handle Error */
  }

  if (fseek(fp2, FLV_START_POS , SEEK_SET) != 0) {
    /* Handle Error */
  }

  //////////////////
  FILE *fp3;
  long fileSize3;
  char *buffer3;

  fp3 = fopen(argv[3], "r");
  if (NULL == fp3) {
    /* Handle Error */
  }

  if (fseek(fp3, FLV_START_POS , SEEK_SET) != 0) {
    /* Handle Error */
  }

  //////////////////
  //finally run
  runTest( fp1, fp2, fp3 );
  return 0;
}
