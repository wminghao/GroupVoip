
#include <stdio.h> 
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>


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


long runTest(FILE* fd, int totalFlvChunks, int flvChunkStartPos)
{
  for(int i = 0; i < totalFlvChunks; i++) {
      unsigned char buffer[FIXED_DATA_SIZE+12];
      unsigned int bufLen = FIXED_DATA_SIZE;

      //first send the segHeader
      unsigned char metaData []= {'S', 'G', 'I', 0x0};
      memcpy(buffer, metaData, sizeof(metaData));
      unsigned int mask1Stream = 0x01;
      memcpy(buffer+sizeof(metaData), &mask1Stream, sizeof(unsigned int));

      //then send the stream heaer
      unsigned char streamIdSource[] = {0x02, 0x0}; //mobile stream
      memcpy(buffer+sizeof(metaData)+sizeof(unsigned int), &streamIdSource, sizeof(streamIdSource));
      memcpy(buffer+sizeof(metaData)+sizeof(unsigned int)+sizeof(streamIdSource), &bufLen, sizeof(unsigned int));

      //then send the buffer
      fread((char*)buffer+sizeof(metaData)+sizeof(unsigned int)+sizeof(streamIdSource)+sizeof(unsigned int), 1, bufLen, fd);
      doWrite(1, buffer, sizeof(buffer));
  }    
  return 1;
}

int main( int argc, char** argv ) {
  if ( argc != 2 ) {
    fprintf(stderr, "usage: %s input_file", argv[0]);
    return -1;
  } 

  FILE *fp;
  long file_size;
  char *buffer;

  fp = fopen(argv[1], "r");
  if (NULL == fp) {
    /* Handle Error */
  }

  if (fseek(fp, 0 , SEEK_END) != 0) {
    /* Handle Error */
  }

  file_size = ftell(fp);

  int flvChunkStartPos = 13;

  int totalFlvChunks = (file_size-flvChunkStartPos)/FIXED_DATA_SIZE;
  fprintf(stderr, "total file size=%ld, totalflvChunks=%d\r\n", file_size, totalFlvChunks, flvChunkStartPos);

  if (fseek(fp, flvChunkStartPos , SEEK_SET) != 0) {
    /* Handle Error */
  }

  runTest( fp, totalFlvChunks, flvChunkStartPos );
  return 0;
}
