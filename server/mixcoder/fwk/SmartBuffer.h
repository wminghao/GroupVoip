#ifndef __FWK_SMARTBUFFER_H__
#define __FWK_SMARTBUFFER_H__

#include "SmartPtr.h""
#include "SmartPtrInterface.h"
#include <string.h>

namespace mixcodec
{

    class SmartBuffer;
    typedef SmartPtr<SmartBuffer> SmartBufferPtr;
    class SmartBuffer : public SmartPtrInterface<SmartBuffer>
    {
    public:
        SmartBuffer( size_t dataLength, const u8 *data )
            : dataLength_(dataLength), more_( false ) {
            data_ = new u8[dataLength];
            memcpy( data_, data, dataLength );
        }

        SmartBuffer( size_t dataLength, const char *data )
            : dataLength_(dataLength), more_( false ) {
            data_ = new u8[dataLength];
            memcpy( data_, data, dataLength );
        }

        SmartBuffer( size_t dataLength ) 
            : dataLength_(dataLength), more_( false ) {
            data_ = new u8[dataLength];
        }

        virtual ~SmartBuffer() {
            delete[] data_;
        }

        virtual size_t dataLength() const { return dataLength_; }
        virtual u8 *data() const { return data_; }

        virtual bool hasSameDataAs( const SmartBufferPtr &b ) const { 
            return ( dataLength_ == b->dataLength_ ) && 
                ( !memcmp( data_, b->data_, dataLength_ ) );
        }

    private:
        SmartBuffer();
        SmartBuffer( const SmartBufferPtr& b );
        SmartBuffer& operator=( const SmartBufferPtr& b );

    private:
        size_t dataLength_;
        u8 *data_;

        bool more_;

    public:
        bool more() const { return more_; }
        void more( bool t ) { more_ = t; }
    };


} // end of namespace mixcoder

#endif


