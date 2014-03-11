
#include <sys/stat.h> 
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>


static const unsigned int FIXED_DATA_SIZE = 4096;

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
  unsigned char buffer[FIXED_DATA_SIZE+12];
    
  //first read the metadata
  unsigned char metaData []= {'S', 'E', 'G'};
  memcpy(buffer, metaData, sizeof(metaData));
  unsigned char mask1Stream = 0x01;
  memcpy(buffer+sizeof(metaData), &mask1Stream, sizeof(unsigned int));
  doWrite(1, buffer, sizeof(metaData)+sizeof(unsigned int));

  unsigned int bufLen = FIXED_DATA_SIZE;
  for(int i = 0; i < totalFlvChunks; i++) {
    unsigned char streamId[] = {0};
    memcpy(buffer, &streamId, sizeof(streamId));
    memcpy(buffer+8, &bufLen, sizeof(unsigned int));
    fread((char*)buffer+sizeof(streamId)+sizeof(unsigned int), 1, bufLen, fd);
    doWrite(1, buffer, sizeof(buffer));
  }    
  return 1;
}

int main( int argc, char** argv ) {
  if ( argc != 3 ) {
    fprintf(stderr, "usage: %s input_file flvDataStartPos", argv[0]);
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

  int flvChunkStartPos = atoi(argv[2]);

  int totalFlvChunks = (file_size-flvChunkStartPos)/FIXED_DATA_SIZE;
  fprintf(stderr, "total file size=%ld, totalflvChunks=%d\r\n", file_size, totalFlvChunks, flvChunkStartPos);

  if (fseek(fp, 0 , SEEK_SET) != 0) {
    /* Handle Error */
  }

  runTest( fp, totalFlvChunks, flvChunkStartPos );
  return 0;
}
