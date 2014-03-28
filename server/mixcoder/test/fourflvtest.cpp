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

long runTest(FILE* fd1, int totalFlvChunks1, FILE* fd2, int totalFlvChunks2, FILE* fd3, int totalFlvChunks3, FILE* fd4, int totalFlvChunks4)
{
    int totalFlvChunks12 = (totalFlvChunks1<totalFlvChunks2)?totalFlvChunks1:totalFlvChunks2;
    int totalFlvChunks34 = (totalFlvChunks3<totalFlvChunks4)?totalFlvChunks3:totalFlvChunks4;
    int totalFlvChunks = (totalFlvChunks12<totalFlvChunks34)?totalFlvChunks12:totalFlvChunks34;

    for(int i = 0; i < totalFlvChunks; i++) {
        unsigned char bigBuf[FIXED_DATA_SIZE*4+8+6*4];
        unsigned int bufLen = FIXED_DATA_SIZE;
        unsigned char* buffer = bigBuf;

        //first send the segHeader
        unsigned char metaData []= {'S', 'G', 'I', 0x0}; //even layout
        memcpy(buffer, metaData, sizeof(metaData));
        unsigned int mask4Stream = 0x0f;
        memcpy(buffer+sizeof(metaData), &mask4Stream, sizeof(unsigned int));
        buffer += (sizeof(metaData)+sizeof(unsigned int));

        //then send the stream heaer of stream 1
        //unsigned char streamIdSource[] = {0x01, 0x0}; //desktop stream
        unsigned char streamIdSource[] = {0x02, 0x0}; //mobile stream
        memcpy(buffer, &streamIdSource, sizeof(streamIdSource));
        memcpy(buffer+sizeof(streamIdSource), &bufLen, sizeof(unsigned int));
        //then send the buffer
        fread((char*)buffer+sizeof(streamIdSource)+sizeof(unsigned int), 1, bufLen, fd1);
        buffer += (sizeof(streamIdSource)+sizeof(unsigned int)+bufLen);
        
        //then send the stream heaer of stream 2
        streamIdSource[0] = 0x0a;//mobile stream
        memcpy(buffer, &streamIdSource, sizeof(streamIdSource));
        memcpy(buffer+sizeof(streamIdSource), &bufLen, sizeof(unsigned int));
        //then send the buffer
        fread((char*)buffer+sizeof(streamIdSource)+sizeof(unsigned int), 1, bufLen, fd2);
        buffer += (sizeof(streamIdSource)+sizeof(unsigned int)+bufLen);

        //then send the stream heaer of stream 3
        streamIdSource[0] = 0x11;//desktop stream
        memcpy(buffer, &streamIdSource, sizeof(streamIdSource));
        memcpy(buffer+sizeof(streamIdSource), &bufLen, sizeof(unsigned int));
        //then send the buffer
        fread((char*)buffer+sizeof(streamIdSource)+sizeof(unsigned int), 1, bufLen, fd3);
        buffer += (sizeof(streamIdSource)+sizeof(unsigned int)+bufLen);

        //then send the stream heaer of stream 4
        streamIdSource[0] = 0x1a;//mobile stream
        memcpy(buffer, &streamIdSource, sizeof(streamIdSource));
        memcpy(buffer+sizeof(streamIdSource), &bufLen, sizeof(unsigned int));
        //then send the buffer
        fread((char*)buffer+sizeof(streamIdSource)+sizeof(unsigned int), 1, bufLen, fd4);
        buffer += (sizeof(streamIdSource)+sizeof(unsigned int)+bufLen);

        doWrite(1, bigBuf, sizeof(bigBuf));
    }    
    return 1;
}

int main( int argc, char** argv ) {
  if ( argc != 5 ) {
      fprintf(stderr, "usage: %s input_file1 input_file2 input_file3 input_file4", argv[0]);
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
  //////////////////
  FILE *fp3;
  long fileSize3;
  char *buffer3;

  fp3 = fopen(argv[3], "r");
  if (NULL == fp3) {
    /* Handle Error */
  }

  if (fseek(fp3, 0 , SEEK_END) != 0) {
    /* Handle Error */
  }

  fileSize3 = ftell(fp3);

  int totalFlvChunks3 = (fileSize3-FLV_START_POS)/FIXED_DATA_SIZE;
  fprintf(stderr, "total file size=%ld, totalflvChunks=%d\r\n", fileSize3, totalFlvChunks3);

  if (fseek(fp3, FLV_START_POS , SEEK_SET) != 0) {
    /* Handle Error */
  }

  //////////////////
  FILE *fp4;
  long fileSize4;
  char *buffer4;

  fp4 = fopen(argv[4], "r");
  if (NULL == fp4) {
    /* Handle Error */
  }

  if (fseek(fp4, 0 , SEEK_END) != 0) {
    /* Handle Error */
  }

  fileSize4 = ftell(fp4);

  int totalFlvChunks4 = (fileSize4-FLV_START_POS)/FIXED_DATA_SIZE;
  fprintf(stderr, "total file size=%ld, totalflvChunks=%d\r\n", fileSize4, totalFlvChunks4);

  if (fseek(fp4, FLV_START_POS , SEEK_SET) != 0) {
    /* Handle Error */
  }

  ///////////////
  runTest( fp1, totalFlvChunks1, fp2, totalFlvChunks2, fp3, totalFlvChunks3, fp4, totalFlvChunks4 );
  return 0;
}
