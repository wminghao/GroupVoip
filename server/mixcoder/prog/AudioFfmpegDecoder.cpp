#include "AudioFfmpegDecoder.h"
#include "fwk/log.h"
#include <assert.h>

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
void  AudioFfmpegDecoder::newAccessUnit( SmartPtr<AccessUnit> au, AudioStreamSetting* rawAudioSetting)
{
    if( !hasFirstFrameDecoded_ ) {
        hasFirstFrameDecoded_ = true;

        CodecID codecID = CODEC_ID_MP3;
        if ( setting_.acid == kAAC ) {
            codecID = CODEC_ID_AAC;
        } else if ( setting_.acid == kMP3 ) {
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
        if( codecID == CODEC_ID_AAC && au->sp == kSeqHeader ) {
            decoderCtx_->extradata_size = au->payload->dataLength();
                                                                                                                                                                  
            if( decoderCtx_->extradata  ) {
                free(decoderCtx_->extradata );
                decoderCtx_->extradata  = 0;
            }
            decoderCtx_->extradata  = (uint8_t*)malloc( au->payload->dataLength() + FF_INPUT_BUFFER_PADDING_SIZE);                
            memcpy( (void*) decoderCtx_->extradata, (void*) au->payload->data(), au->payload->dataLength() );                   
        }

        decoderFrame_ = avcodec_alloc_frame();
        assert(decoderFrame_);
        
        AVDictionary* d = 0;
        if( avcodec_open2( decoderCtx_, decoderInst_, &d ) < 0 ) {
            LOG( "FAILED to open MP3 decoder, FAILING\n" );
        }
        if( decoderCtx_->sample_fmt != AV_SAMPLE_FMT_S16 ) {
            LOG( "FAILED to open correct sample format for audio decode!, FAILING\n" );
        }
    
        decoderCtx_->channels = (setting_.at == kSndMono) ? 1:2;
        LOG( "Source audio settings: #Channels=%d, #Freq=%d, sampleFmt=%s\n", decoderCtx_->channels, getFreq(setting_.ar), codecID==CODEC_ID_AAC?"aac":"mp3" );
    }
    
    
    AVPacket decodePkt;
    av_init_packet( &decodePkt );
    decodePkt.data = au->payload->data();
    decodePkt.size = au->payload->dataLength();

    int gotFrame = 0;
    avcodec_decode_audio4( decoderCtx_, decoderFrame_, &gotFrame, &decodePkt );
    if( gotFrame ) {
        sampleSize_ = decoderFrame_->linesize[0]/(sizeof(short) * getNumChannels(setting_.at));
        resampleFrame( rawAudioSetting, sampleSize_, decoderFrame_->data[0]);
    }
}
