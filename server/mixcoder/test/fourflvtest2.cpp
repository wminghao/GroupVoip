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

int cpSGIHeader(unsigned char*& buffer, unsigned int mask4Stream)
{
    //first send the segHeader
    unsigned char metaData []= {'S', 'G', 'I', 0x0}; //even layout
    memcpy(buffer, metaData, sizeof(metaData));
    memcpy(buffer+sizeof(metaData), &mask4Stream, sizeof(unsigned int));
    return (sizeof(metaData)+sizeof(unsigned int));
}

long runTest(FILE* fd1, FILE* fd2, FILE* fd3, FILE* fd4)
{
    unsigned char bigBuf[MAX_BUFFER_SIZE];
    int bufLen = 0;
    bool bFile1Done = false;
    bool bFile2Done = false;
    bool bFile3Done = false;
    bool bFile4Done = false;
    FLVRealTimeParser realtimeParser1;
    FLVRealTimeParser realtimeParser2;
    FLVRealTimeParser realtimeParser3;
    FLVRealTimeParser realtimeParser4;
    unsigned char* result = NULL;
    while(!(bFile1Done && bFile2Done && bFile3Done && bFile4Done)) {
        fprintf(stderr, "=================round \r\n");
        unsigned char* buffer = bigBuf;
        unsigned int mask4Stream = 0x0f;
        if( bFile1Done ) {
            mask4Stream &= 0xfe;
        } 
        if ( bFile2Done ) {
            mask4Stream &= 0xfd;
        } 
        if( bFile3Done ) {
            mask4Stream &= 0xfb;
        }
        if ( bFile4Done ) {
            mask4Stream &= 0xf7;
        }

        if ( !bFile1Done ) {
            //then send the stream heaer of stream 1
            result = realtimeParser1.readData(fd1, &bufLen);
            if ( result ) {
                int totalLen = 0;

                int headerSize = cpSGIHeader(buffer, mask4Stream);
                totalLen += headerSize;
                buffer += headerSize;
                
                unsigned char streamIdSource[] = {0x02, 0x0}; //mobile stream
                memcpy(buffer, &streamIdSource, sizeof(streamIdSource));
                memcpy(buffer+sizeof(streamIdSource), &bufLen, sizeof(unsigned int));
                memcpy((char*)buffer+sizeof(streamIdSource)+sizeof(unsigned int), result, bufLen);
                buffer += (sizeof(streamIdSource)+sizeof(unsigned int)+bufLen);
                totalLen += (sizeof(streamIdSource)+sizeof(unsigned int)+bufLen);

                for(int i = 0;  i < 4; i++) {
                    if( i!=0 ) {
                        int len = 0;
                        unsigned char streamIdSource[] = {(i<<3)|0x02, 0x0}; //mobile stream
                        memcpy(buffer, &streamIdSource, sizeof(streamIdSource));
                        memcpy(buffer+sizeof(streamIdSource), &len, sizeof(unsigned int));
                        buffer += (sizeof(streamIdSource)+sizeof(unsigned int));
                        totalLen += (sizeof(streamIdSource)+sizeof(unsigned int));
                    } 
                }
                doWrite(1, bigBuf, totalLen);
                //fprintf(stderr,"==file 1 write=%d\r\n", totalLen);
            } else {
                bFile1Done = true;
                fprintf(stderr,"==file 1 done\r\n");
            }
        }

        if ( !bFile2Done ) {

            //then send the stream heaer of stream 2
            result = realtimeParser2.readData(fd2, &bufLen);
            if ( result ) {
                int totalLen = 0;

                int headerSize = cpSGIHeader(buffer, mask4Stream);
                totalLen += headerSize;
                buffer += headerSize;

                unsigned char streamIdSource[] = {0x0a, 0x0}; //mobile stream
                memcpy(buffer, &streamIdSource, sizeof(streamIdSource));
                memcpy(buffer+sizeof(streamIdSource), &bufLen, sizeof(unsigned int));
                memcpy((char*)buffer+sizeof(streamIdSource)+sizeof(unsigned int), result, bufLen);
                buffer += (sizeof(streamIdSource)+sizeof(unsigned int)+bufLen);
                totalLen += (sizeof(streamIdSource)+sizeof(unsigned int)+bufLen);

                for(int i = 0;  i < 4; i++) {
                    if( i!=1 ) {
                        int len = 0;
                        unsigned char streamIdSource[] = {(i<<3)|0x02, 0x0}; //mobile stream
                        memcpy(buffer, &streamIdSource, sizeof(streamIdSource));
                        memcpy(buffer+sizeof(streamIdSource), &len, sizeof(unsigned int));
                        buffer += (sizeof(streamIdSource)+sizeof(unsigned int));
                        totalLen += (sizeof(streamIdSource)+sizeof(unsigned int));
                    } 
                }

                doWrite(1, bigBuf, totalLen);
                //fprintf(stderr,"==file 2 write=%d\r\n", totalLen);
            } else {
                bFile2Done = true;
                fprintf(stderr,"==file 2 done\r\n");
            }
        }

        if ( !bFile3Done ) {
            //then send the stream heaer of stream 3
            result = realtimeParser3.readData(fd3, &bufLen);
            if ( result ) {
                int totalLen = 0;

                int headerSize = cpSGIHeader(buffer, mask4Stream);
                totalLen += headerSize;
                buffer += headerSize;

                //then send the stream heaer of stream 3
                unsigned char streamIdSource[] = {0x12, 0x0};//mobile stream
                memcpy(buffer, &streamIdSource, sizeof(streamIdSource));
                memcpy(buffer+sizeof(streamIdSource), &bufLen, sizeof(unsigned int));
                
                memcpy((char*)buffer+sizeof(streamIdSource)+sizeof(unsigned int), result, bufLen);
                buffer += (sizeof(streamIdSource)+sizeof(unsigned int)+bufLen);
                totalLen += (sizeof(streamIdSource)+sizeof(unsigned int)+bufLen);

                for(int i = 0;  i < 4; i++) {
                    if( i!=2 ) {
                        int len = 0;
                        unsigned char streamIdSource[] = {(i<<3)|0x02, 0x0}; //mobile stream
                        memcpy(buffer, &streamIdSource, sizeof(streamIdSource));
                        memcpy(buffer+sizeof(streamIdSource), &len, sizeof(unsigned int));
                        buffer += (sizeof(streamIdSource)+sizeof(unsigned int));
                        totalLen += (sizeof(streamIdSource)+sizeof(unsigned int));
                    } 
                }

                doWrite(1, bigBuf, totalLen);
                //fprintf(stderr,"==file 3 write=%d\r\n", totalLen);
            } else {
                bFile3Done = true;
                fprintf(stderr,"==file 3 done\r\n");
            }
        }

        if ( !bFile4Done ) {
            //then send the stream heaer of stream 4
            result = realtimeParser4.readData(fd4, &bufLen);
            if ( result ) {
                int totalLen = 0;

                int headerSize = cpSGIHeader(buffer, mask4Stream);
                totalLen += headerSize;
                buffer += headerSize;

                //then send the stream heaer of stream 4
                unsigned char streamIdSource[] = {0x1a, 0x0};//mobile stream
                memcpy(buffer, &streamIdSource, sizeof(streamIdSource));
                memcpy(buffer+sizeof(streamIdSource), &bufLen, sizeof(unsigned int));
                memcpy((char*)buffer+sizeof(streamIdSource)+sizeof(unsigned int), result, bufLen);
                buffer += (sizeof(streamIdSource)+sizeof(unsigned int)+bufLen);
                totalLen += (sizeof(streamIdSource)+sizeof(unsigned int)+bufLen);

                for(int i = 0;  i < 4; i++) {
                    if( i!=3 ) {
                        int len = 0;
                        unsigned char streamIdSource[] = {(i<<3)|0x02, 0x0}; //mobile stream
                        memcpy(buffer, &streamIdSource, sizeof(streamIdSource));
                        memcpy(buffer+sizeof(streamIdSource), &len, sizeof(unsigned int));
                        buffer += (sizeof(streamIdSource)+sizeof(unsigned int));
                        totalLen += (sizeof(streamIdSource)+sizeof(unsigned int));
                    } 
                }

                doWrite(1, bigBuf, totalLen);
                //fprintf(stderr,"==file 4 write=%d\r\n", totalLen);
            } else {
                bFile4Done = true;
                fprintf(stderr,"==file 4 done\r\n");
            }
        }
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
  FILE *fp4;
  long fileSize4;
  char *buffer4;

  fp4 = fopen(argv[4], "r");
  if (NULL == fp4) {
    /* Handle Error */
  }

  if (fseek(fp4, FLV_START_POS , SEEK_SET) != 0) {
    /* Handle Error */
  }

  ///////////////
  runTest( fp1, fp2, fp3, fp4);
  return 0;
}
