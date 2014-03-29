#include <stdio.h> 
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "flvrealtimeparser.h"

static const unsigned int FLV_START_POS = 13;

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

long runTest(FILE* fd1, FILE* fd2)
{
    unsigned char bigBuf[MAX_BUFFER_SIZE];
    int totalLen = 0;
    int bufLen = 0;
    bool bFile1Done = false;
    bool bFile2Done = false;
    FLVRealTimeParser realtimeParser1;
    FLVRealTimeParser realtimeParser2;
    unsigned char* result = NULL;
    while(!bFile1Done && !bFile2Done) {
        unsigned char* buffer = bigBuf;
        totalLen = 0;

        //first send the segHeader
        unsigned char metaData []= {'S', 'G', 'I', 0x0}; //even layout
        memcpy(buffer, metaData, sizeof(metaData));
        unsigned int mask2Stream = 0x03;
        memcpy(buffer+sizeof(metaData), &mask2Stream, sizeof(unsigned int));
        buffer += (sizeof(metaData)+sizeof(unsigned int));
        totalLen += (sizeof(metaData)+sizeof(unsigned int));

        //then send the stream heaer of stream 1
        result = realtimeParser1.readData(fd1, &bufLen);
        if ( result ) {
            //unsigned char streamIdSource[] = {0x01, 0x0}; //desktop stream
            unsigned char streamIdSource[] = {0x02, 0x0}; //mobile stream
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
            unsigned char streamIdSource[] = {0x0a, 0x0}; //mobile stream
            memcpy(buffer, &streamIdSource, sizeof(streamIdSource));
            memcpy(buffer+sizeof(streamIdSource), &bufLen, sizeof(unsigned int));
            memcpy((char*)buffer+sizeof(streamIdSource)+sizeof(unsigned int), result, bufLen);
            buffer += (sizeof(streamIdSource)+sizeof(unsigned int)+bufLen);
            totalLen += (sizeof(streamIdSource)+sizeof(unsigned int)+bufLen);
        } else {
            bFile2Done = true;
        }
        doWrite(1, bigBuf, totalLen);
    }    
    return 1;
}

int main( int argc, char** argv ) {
  if ( argc != 3 ) {
      fprintf(stderr, "usage: %s input_file1 input_file2", argv[0]);
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

  runTest( fp1, fp2 );
  return 0;
}
