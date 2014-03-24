#include <stdio.h> 
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>


static const unsigned int FLV_START_POS = 13;
static const unsigned int FIXED_DATA_SIZE = 500;

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

long runTest(FILE* fd1, int totalFlvChunks1, FILE* fd2, int totalFlvChunks2)
{
    int totalFlvChunks = (totalFlvChunks1<totalFlvChunks2)?totalFlvChunks1:totalFlvChunks2;
    for(int i = 0; i < totalFlvChunks; i++) {
        unsigned char bigBuf[FIXED_DATA_SIZE*2+7+5*2];
        unsigned int bufLen = FIXED_DATA_SIZE;
        unsigned char* buffer = bigBuf;

        //first send the segHeader
        unsigned char metaData []= {'S', 'E', 'G'};
        memcpy(buffer, metaData, sizeof(metaData));
        unsigned int mask2Stream = 0x03;
        memcpy(buffer+sizeof(metaData), &mask2Stream, sizeof(unsigned int));
        buffer += (sizeof(metaData)+sizeof(unsigned int));

        //then send the stream heaer of stream 1
        unsigned char streamId[] = {0};
        memcpy(buffer, &streamId, sizeof(streamId));
        memcpy(buffer+sizeof(streamId), &bufLen, sizeof(unsigned int));
        //then send the buffer
        fread((char*)buffer+sizeof(streamId)+sizeof(unsigned int), 1, bufLen, fd1);
        buffer += (sizeof(streamId)+sizeof(unsigned int)+bufLen);
        
        //then send the stream heaer of stream 2
        streamId[0] = {1};
        memcpy(buffer, &streamId, sizeof(streamId));
        memcpy(buffer+sizeof(streamId), &bufLen, sizeof(unsigned int));
        //then send the buffer
        fread((char*)buffer+sizeof(streamId)+sizeof(unsigned int), 1, bufLen, fd2);
        buffer += (sizeof(streamId)+sizeof(unsigned int)+bufLen);

        doWrite(1, buffer, sizeof(buffer));
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

  if (fseek(fp1, 0 , SEEK_END) != 0) {
    /* Handle Error */
  }

  fileSize1 = ftell(fp1);

  int totalFlvChunks1 = (fileSize1-FLV_START_POS)/FIXED_DATA_SIZE;
  fprintf(stderr, "total file size=%ld, totalflvChunks=%d\r\n", fileSize1, totalFlvChunks1);

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

  if (fseek(fp2, 0 , SEEK_END) != 0) {
    /* Handle Error */
  }

  fileSize2 = ftell(fp2);

  int totalFlvChunks2 = (fileSize2-FLV_START_POS)/FIXED_DATA_SIZE;
  fprintf(stderr, "total file size=%ld, totalflvChunks=%d\r\n", fileSize2, totalFlvChunks2);

  if (fseek(fp2, FLV_START_POS , SEEK_SET) != 0) {
    /* Handle Error */
  }

  runTest( fp1, totalFlvChunks1, fp2, totalFlvChunks2 );
  return 0;
}
