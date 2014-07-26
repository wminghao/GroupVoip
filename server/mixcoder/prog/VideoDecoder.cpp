extern "C" {
#include <libavcodec/avcodec.h>    // required headers
#include <libavformat/avformat.h>
}

#include "VideoDecoder.h"
#include <assert.h>
#include "fwk/log.h"
            
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
        LOG("FAILED to find h264 decoder, FAILING\n");
        exit(-1);
    }

    codecCtx_ = avcodec_alloc_context3( codec_ );
    if( ! codecCtx_ ) {
        LOG("FAILED to init h264 decoder, FAILING2\n" );
        exit(-1);
    }
    
    AVDictionary* d = 0;
    codecCtx_->extradata = (uint8_t*) malloc( spspps->dataLength() + FF_INPUT_BUFFER_PADDING_SIZE );
    memcpy( codecCtx_->extradata, spspps->data(), spspps->dataLength() );
    codecCtx_->extradata_size = spspps->dataLength();
    if( avcodec_open2( codecCtx_, codec_, &d ) < 0 ) {
        LOG("FAILED to open h264 decoder, FAILING3\n" );
    }

    frame_ = avcodec_alloc_frame();
}

bool VideoDecoder::newAccessUnit( SmartPtr<AccessUnit> au, SmartPtr<VideoRawData> v)
{
    bool bIsValidFrame = false;
    assert( au->st == kVideoStreamType );
    assert( au->ctype == kAVCVideoPacket );

    //save the settings here
    v->sp = au->sp;
    v->pts = au->pts;

    //it can be a sps-pps header or regular nalu    
    if ( au->sp == kSpsPps ) {
        spspps_ = au->payload;
        initDecoder( spspps_ );
        //TODO parse sps/pps to get width and height, now assume it's 640*480
        inWidth_ = 640;
        inHeight_ = 480;

        v->rawVideoSettings_.vcid = kAVCVideoPacket;
        v->rawVideoSettings_.width = inWidth_;
        v->rawVideoSettings_.height =inHeight_; 

        LOG("Video decoded sps pps, len=%ld, ts=%d\n", spspps_->dataLength(), au->pts);
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
                    //LOG("Video decoded width=%d, height=%d\n", frame_->width, frame_->height);
                    assert(inWidth_ == frame_->width);
                    assert(inHeight_ == frame_->height);

                    //copy 3 planes and 3 strides
                    v->rawVideoPlanes_[0] = new SmartBuffer( frame_->linesize[0]*inHeight_, frame_->data[0]);
                    v->rawVideoPlanes_[1] = new SmartBuffer( frame_->linesize[1]*inHeight_, frame_->data[1]);
                    v->rawVideoPlanes_[2] = new SmartBuffer( frame_->linesize[2]*inHeight_, frame_->data[2]);

                    memcpy(v->rawVideoStrides_, frame_->linesize, sizeof(int)*3);

                    v->rawVideoSettings_.vcid = kAVCVideoPacket;
                    v->rawVideoSettings_.width = inWidth_;
                    v->rawVideoSettings_.height =inHeight_; 

                    bIsValidFrame = true;
                    if( !bHasFirstFrameStarted ) {
                        bHasFirstFrameStarted = true;
                        firstFramePts_ = au->pts;
                    }
                    /*
                    LOG( "video decoded pkt size=%d stride0=%d, stride1=%d, stride2=%d, width=%d, height=%d, ts=%d, streamId_=%d\n", pkt.size, 
                         frame_->linesize[0], frame_->linesize[1], frame_->linesize[2],
                         frame_->width, frame_->height, au->pts, streamId_);
                    */
                } else {
                    LOG( "DIDNT get video frame\n");
                }
            } else if ( rval == 0 ) {
                LOG( "NO FRAME!\n");
            } else {
                LOG( "DECODE ERROR!\n");
            }
        }
    }
    return bIsValidFrame;
}
