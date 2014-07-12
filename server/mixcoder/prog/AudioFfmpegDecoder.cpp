#include "AudioFfmpegDecoder.h"

AudioFfmpegDecoder::~AudioFfmpegDecoder()
{
    if( decoderCtx_ ) {
        if ( decoderCtx_->extradata ) {
            free(decoderCtx_->extradata);
            decoderCtx_->extradata = 0;
        }
        avcodec_close( decoderCtx_ );
        av_free( decoderCtx_ );
        decoderCtx_  = 0;
    }
    
    if( decoderFrame_ ) {
        av_free( decoderFrame_ );
        decoderFrame_ = 0;
    }
}
//send it to the decoder
SmartPtr<SmartBuffer>  AudioFfmpegDecoder::newAccessUnit( SmartPtr<AccessUnit> au, AudioStreamSetting* aInputSetting)
{
    if( !hasFirstFrameDecoded_ ) {
        hasFirstFrameDecoded_ = true;

        CodecID codecID = CODEC_ID_MP3;
        if ( codecType == kAAC ) {
            codecID = CODEC_ID_AAC;
        } else if ( codecType == kMP3 ) {
            codecID = CODEC_ID_MP3;
        } else {
            //don't support other codecs
            assert(0);
        }

        decoderInst_ = avcodec_find_decoder( codecID );
        assert(decoderInst_);

        decoderCtx_ = avcodec_alloc_context3( decoderInst_ );
        assert(decoderCtx_);

        decoderCtx_->request_sample_fmt = AV_SAMPLE_FMT_S16;

        //audioSpecificCodecInfo
        if( codecType == kAAC && au->sp == kSeqHeader ) {
            decodeCtx_->extradata_size = au->dataLength();
                                                                                                                                                                  
            if( decodeCtx_->extradata  ) {
                free(decodeCtx_->extradata );
                decodeCtx_->extradata  = 0;
            }
            decodeCtx_->extradata  = (uint8_t*)malloc( au->dataLength() + FF_INPUT_BUFFER_PADDING_SIZE);                
            memcpy( (void*) decodeCtx_->extradata, (void*) au->data(), au->dataLength() );                   
        }

        decoderFrame_ = avcodec_alloc_frame();
        assert(decoderFrame_);
        
        AVDictionary* d = 0;
        if( avcodec_open2( decodeCtx_, decodeCdc_, &d ) < 0 ) {
            LOG( "FAILED to open MP3 decoder, FAILING\n" );
        }
        if( decodeCtx_->sample_fmt != AV_SAMPLE_FMT_S16 ) {
            LOG( "FAILED to open correct sample format for audio decode!, FAILING\n" );
        }
    
        decoderCtx_->channels = (settings_.at == kSndMono) ? 1:2;
        LOG( "Source audio settings: #Channels=%d, #Freq=%d, sampleFmt=%s\n", decoderCtx_->channels, getFreq(settings_.ar), codecID==CODEC_ID_AAC?"aac":"mp3" );
        
    }
    return SmartPtr<SmartBuffer>(NULL);
}


