#include "VideoDecoder.h"
#include <assert.h>
            
VideoDecoder::~VideoDecoder() {
    reset();
}

void VideoDecoder::reset() {
    if( codecCtx_ ) {
        if( codecCtx_->extradata ) {
            free( codecCtx_->extradata );
            codecCtx_->extradata = 0;
        }
        avcodec_close( codecCtx_ );
        av_free( codecCtx_ );
        codecCtx_ = 0;
    }

    if( frame_ ) {
        av_free( frame_ );
        frame_ = 0;
    }
}

void VideoDecoder::initDecoder( SmartPtr<SmartBuffer> spspps ) {
    reset();

    /* AVCodec/Decode init */
    codec_ = avcodec_find_decoder( CODEC_ID_H264 );
    if( ! codec_ ) {
        fprintf( stderr, "FAILED to find h264 decoder, FAILING\n");
        exit(-1);
    }

    codecCtx_ = avcodec_alloc_context3( codec_ );
    if( ! codecCtx_ ) {
        fprintf( stderr, "FAILED to init h264 decoder, FAILING2\n" );
        exit(-1);
    }
    
    AVDictionary* d = 0;
    codecCtx_->extradata = (uint8_t*) malloc( spspps->dataLength() + FF_INPUT_BUFFER_PADDING_SIZE );
    memcpy( codecCtx_->extradata, spspps->data(), spspps->dataLength() );
    codecCtx_->extradata_size = spspps->dataLength();
    if( avcodec_open2( codecCtx_, codec_, &d ) < 0 ) {
        fprintf( stderr, "FAILED to open h264 decoder, FAILING3\n" );
    }

    frame_ = avcodec_alloc_frame();
}

SmartPtr<SmartBuffer> VideoDecoder::newAccessUnit( SmartPtr<AccessUnit> au )
{
    assert( au->st == kVideoStreamType );
    assert( au->ct == kAVCVideoPacket );

    //it can be a sps-pps header or regular nalu    
    SmartPtr<SmartBuffer> result = NULL;
    if ( au->sp == kSpsPps ) {
        spspps_ = au->payload;
        initDecoder( spspps_ );
        //TODO parse sps/pps to get width and height, now assume it's 640*480
        inWidth_ = 640;
        inHeight_ = 480;
        fprintf( stderr, "Got sps pps, len=%ld\n", spspps_->dataLength());
    } else if( au->sp == kRawData ) {
        assert(inWidth_ && inHeight_);
        if ( spspps_ ) {
            SmartPtr<SmartBuffer> buf = au->payload;
            
            //key frame must be combined with sps pps header
            if( au->isKey ) {
                SmartPtr<SmartBuffer> totalBuf = new SmartBuffer( buf->dataLength() + spspps_->dataLength() );
                memcpy( totalBuf->data(), spspps_->data(), spspps_->dataLength() );
                memcpy( totalBuf->data() + spspps_->dataLength(), buf->data(), buf->dataLength() );
                buf = totalBuf;
            }

            AVPacket pkt;
            av_init_packet( &pkt );
            pkt.size = buf->dataLength();
            pkt.data = buf->data();

            int gotPic = 0;
            int rval;
            if( ( rval = avcodec_decode_video2( codecCtx_, frame_, &gotPic, &pkt ) ) > 0) {
                if( gotPic ) {
                    //yv12/yuv420, 3 planes combined into 1 buffer
                    int totalPixels = inWidth_*inHeight_;
                    result = new SmartBuffer( (totalPixels*3)/2, frame_->data[0] );
                    fprintf( stderr, "GOTIT, video frame size=%d, stride=%d\n", (totalPixels*3)/2, frame_->linesize[0]);
                } else {
                    fprintf( stderr, "DIDNT get video frame\n");
                }
            } else if ( rval == 0 ) {
                fprintf( stderr, "NO FRAME!\n");
            } else {
                fprintf( stderr, "DECODE ERROR!\n");
            }
        }
    }
    return result;
}
