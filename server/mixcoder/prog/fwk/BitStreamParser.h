
#ifndef __BITSTREAM_PARSER_H__
#define __BITSTREAM_PARSER_H__

#include <string>
#include <stdio.h>
#include "Units.h"


class BitStreamParser {
 public:
    BitStreamParser( const std::string &byteStream ) : byteStream_(byteStream), offset_(0) {}
    BitStreamParser() : offset_( 0 ) {}
    
    const std::string& bytes() { return byteStream_; }

    u8 readByte() {
        return (u8)readBits(8);
    }
    
    u32 readBits( int nBits ) {
        u32 ret = peekBits( 0, nBits );
        offset_ += nBits;
        return ret;
    }
    
    u32 peekBits( int nBitsToSkip, int nBits ) {
        u32 ret = 0;
        
        if ( offset_ + nBits > byteStream_.length() * 8 ) {
            fprintf(stderr, "ERROR: Insufficient bits remaining in stream");
            return 0;
        }
        
        for ( int i = 0 ; i < nBits ; ++i ) {
            int offset = offset_ + nBitsToSkip + i;
            
            //if ( offset % 8 == 0 ) fprintf( stderr, "Byte %02x\n", byteStream_[offset/8] & 0xff );
            
            u8 t = ( byteStream_[offset/8] >> (7-(offset%8)) ) & 0x1;
            //fprintf( stderr, "Bit %d\n", t );
            ret = (ret << 1) & 0xfffffffe;
            ret = ret | t; //( (t >> (7-(offset%8))) & 0x1 );
        }
        
        //fprintf( stderr, "Return %04x\n", ret );
        
        return ret;
    }
    
    void writeBits( u32 value, int nBits ) {
        value = value << (32-nBits);
        
        if ( offset_ + nBits > byteStream_.length() * 8 ) {
            //throw "ERROR: Insufficient bits remaining in stream";
            byteStream_.resize( (offset_ + nBits + 7) / 8, 0 );
        }
        
        for ( int i = 0 ; i < nBits ; ++i ) {
            u8 t = byteStream_[offset_/8];
            u8 b = ((value>>31)&0x1);
            b = b << (7 - (offset_%8));
            u8 mask = ~( 1 << (7 - (offset_%8)) );
            t = (t & mask) | b;
            byteStream_[offset_/8] = t;
            ++offset_;
            value = value << 1;
        }
    }
    
    bool byteAligned() {
        return (offset_%8) == 0;
    }
    
    size_t bitsRemainingInStream() {
        return byteStream_.length() * 8 - offset_;
    }
    
    size_t byteOffset() {
        if ( offset_ % 8 != 0 ) { fprintf(stderr, "byteOffset() called when not on a byte boundary"); return 0; }
        return offset_ / 8;
    }
    
    void byteOffset( size_t t ) {
        if ( offset_ % 8 != 0 ) { fprintf(stderr, "byteOffset() called when not on a byte boundary"); return; }
        offset_ = t * 8;
    }
    
    std::string readBytes( size_t t ) {
        if ( offset_ % 8 != 0 ) { fprintf(stderr, "BitStream::readBytes() called when not on a byte boundary"); ASSERT(0); }
        if ( offset_ / 8 + t > byteStream_.length() ) { fprintf(stderr,"not enough bytes in BitStream::readBytes()"); ASSERT(0); }
        std::string ret = byteStream_.substr( offset_ / 8, t );
        offset_ += t * 8;
        return ret;
    }
    
    inline void writeBytes( const std::string &s ) { return writeBytes( s.c_str(), s.length() ); }
    
    void writeBytes( const char *buf, size_t t ) {
        if ( offset_ % 8 != 0 ) { fprintf(stderr, "BitStream::writeBytes() called when not on a byte boundary"); return; }
        
        if ( offset_ / 8 + t > byteStream_.length() ) {
            byteStream_.resize( offset_ / 8 + t, 0 );
        }
        
        for ( size_t i = 0 ; i < t ; ++i ) {
            byteStream_[ offset_/8 ] = buf[i];
            offset_ += 8;
        }
    }
    
 private:
    std::string byteStream_;
    size_t offset_;
};


#endif
