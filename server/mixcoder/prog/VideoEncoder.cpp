#include "VideoEncoder.h"
#include <assert.h>

#define interface (vpx_codec_vp8_cx())

#ifdef DEBUG_SAVE_IVF
//test code to save into a ivf file
#define fourcc    0x30385056
#define IVF_FILE_HDR_SZ  (32)
#define IVF_FRAME_HDR_SZ (12)

static void mem_put_le16(char *mem, unsigned int val) {
    mem[0] = val;
    mem[1] = val>>8;
}
static void mem_put_le32(char *mem, unsigned int val) {
    mem[0] = val;
    mem[1] = val>>8;
    mem[2] = val>>16;
    mem[3] = val>>24;
}
static void write_ivf_file_header(FILE *outfile,
                                  const vpx_codec_enc_cfg_t *cfg,
                                  int frame_cnt) {
    char header[32];

    if(cfg->g_pass != VPX_RC_ONE_PASS && cfg->g_pass != VPX_RC_LAST_PASS)
        return;
    header[0] = 'D';
    header[1] = 'K';
    header[2] = 'I';
    header[3] = 'F';
    mem_put_le16(header+4,  0);                   /* version */
    mem_put_le16(header+6,  32);                  /* headersize */
    mem_put_le32(header+8,  fourcc);              /* headersize */
    mem_put_le16(header+12, cfg->g_w);            /* width */
    mem_put_le16(header+14, cfg->g_h);            /* height */
    mem_put_le32(header+16, cfg->g_timebase.den); /* rate */
    mem_put_le32(header+20, cfg->g_timebase.num); /* scale */
    mem_put_le32(header+24, frame_cnt);           /* length */
    mem_put_le32(header+28, 0);                   /* unused */

    (void) fwrite(header, 1, 32, outfile);
}

static void write_ivf_frame_header(FILE *outfile,
                                   const vpx_codec_cx_pkt_t *pkt)
{
    char             header[12];
    vpx_codec_pts_t  pts;

    if(pkt->kind != VPX_CODEC_CX_FRAME_PKT)
        return;

    pts = pkt->data.frame.pts;
    mem_put_le32(header, pkt->data.frame.sz);
    mem_put_le32(header+4, pts&0xFFFFFFFF);
    mem_put_le32(header+8, pts >> 32);

    (void) fwrite(header, 1, 12, outfile);
}
#endif //DEBUG_SAVE_IVF

//we choose to use 5 layers of temporal scalability
#define NUM_LAYERS 5

VideoEncoder::VideoEncoder( VideoStreamSetting* setting, int vBaseLayerBitrate ) : vBaseLayerBitrate_(vBaseLayerBitrate), frameInputCnt_(0), frameOutputCnt_(0), timestampTick_(0)
{
    vpx_codec_err_t      res;

    //vp8 encoder
    memcpy(&vSetting_, setting, sizeof(VideoStreamSetting));

    if ( !vpx_img_alloc (&raw_, VPX_IMG_FMT_I420, setting->width, setting->height, 32)) {
        fprintf(stderr, "Failed to allocate frame\n");
        assert(0);
        return;
    }
    /* Populate encoder configuration */
    res = vpx_codec_enc_config_default(interface, &cfg_, 0);
    if(res) {
        fprintf(stderr, "Failed to get config: %s\n", vpx_codec_err_to_string(res));
        return;
    }

    /* Update the default configuration with our settings */
    cfg_.g_w = setting->width;
    cfg_.g_h = setting->height;

    /* Timebase format e.g. 30fps: numerator=1, demoninator=30 */
    cfg_.g_timebase.num = 1;
    cfg_.g_timebase.den = 30;

    /* Target bit rate for each layer*/
    for (int i=0; i<NUM_LAYERS; i++) {
        cfg_.ts_target_bitrate[i] = vBaseLayerBitrate_ + i*10;
    }

    /* Real time parameters */
    cfg_.rc_dropframe_thresh = 0;
    cfg_.rc_end_usage        = VPX_CBR;
    cfg_.rc_resize_allowed   = 0;
    cfg_.rc_min_quantizer    = 2;
    cfg_.rc_max_quantizer    = 56;
    cfg_.rc_undershoot_pct   = 100;
    cfg_.rc_overshoot_pct    = 15;
    cfg_.rc_buf_initial_sz   = 500;
    cfg_.rc_buf_optimal_sz   = 600;
    cfg_.rc_buf_sz           = 1000;

    /* Enable error resilient mode */
    cfg_.g_error_resilient = 1;
    cfg_.g_lag_in_frames   = 0;
    cfg_.kf_mode           = VPX_KF_DISABLED;

    /* Disable automatic keyframe placement */
    cfg_.kf_min_dist = cfg_.kf_max_dist = 3000;

    /* Default setting for bitrate: used in special case of 1 layer (case 0). */
    cfg_.rc_target_bitrate = cfg_.ts_target_bitrate[0];

    /* 5-layers, 16-frame period */
    int ids[16] = {0,4,3,4,2,4,3,4,1,4,3,4,2,4,3,4};
    cfg_.ts_number_layers     = NUM_LAYERS;
    cfg_.ts_periodicity       = 16;
    cfg_.ts_rate_decimator[0] = 16;
    cfg_.ts_rate_decimator[1] = 8;
    cfg_.ts_rate_decimator[2] = 4;
    cfg_.ts_rate_decimator[3] = 2;
    cfg_.ts_rate_decimator[4] = 1;
    memcpy(cfg_.ts_layer_id, ids, sizeof(ids));
    
    layerFlags_[0]  = VPX_EFLAG_FORCE_KF;
    layerFlags_[1]  =
        layerFlags_[3]  =
        layerFlags_[5]  =
        layerFlags_[7]  =
        layerFlags_[9]  =
        layerFlags_[11] =
        layerFlags_[13] =
        layerFlags_[15] = VP8_EFLAG_NO_UPD_LAST | VP8_EFLAG_NO_UPD_GF | VP8_EFLAG_NO_UPD_ARF;
    layerFlags_[2]  =
        layerFlags_[6]  =
        layerFlags_[10] =
        layerFlags_[14] = VP8_EFLAG_NO_UPD_ARF | VP8_EFLAG_NO_UPD_GF;
    layerFlags_[4]  =
        layerFlags_[12] = VP8_EFLAG_NO_REF_LAST | VP8_EFLAG_NO_UPD_ARF;
    layerFlags_[8]  = VP8_EFLAG_NO_REF_LAST | VP8_EFLAG_NO_REF_GF;

    /* Initialize codec */
    if (vpx_codec_enc_init (&codec_, interface, &cfg_, 0)) {
        fprintf(stderr, "Failed to initialize encoder");
        assert(0);
    }

    /* Cap CPU & first I-frame size */
    vpx_codec_control(&codec_, VP8E_SET_CPUUSED,                -6);
    vpx_codec_control(&codec_, VP8E_SET_STATIC_THRESHOLD,      1);
    vpx_codec_control(&codec_, VP8E_SET_NOISE_SENSITIVITY,       1);
    vpx_codec_control(&codec_, VP8E_SET_TOKEN_PARTITIONS,       (vp8e_token_partitions)1);

    int max_intra_size_pct = (int) (((double)cfg_.rc_buf_optimal_sz * 0.5)
                                * ((double) cfg_.g_timebase.den / cfg_.g_timebase.num)
                                / 10.0);
    /* fprintf (stderr, "max_intra_size_pct=%d\n", max_intra_size_pct); */

    vpx_codec_control(&codec_, VP8E_SET_MAX_INTRA_BITRATE_PCT,
                      max_intra_size_pct);

#ifdef DEBUG_SAVE_IVF
    outFile_ = fopen("test_result.ivf", "wb");
    write_ivf_file_header(outFile_, &cfg_, 0);
#endif
}

VideoEncoder::~VideoEncoder()
{
    if (vpx_codec_destroy(&codec_)) {
        fprintf(stderr, "Failed to destroy codec");
    }
#ifdef DEBUG_SAVE_IVF
    if (!fseek(outFile_, 0, SEEK_SET)) {
        write_ivf_file_header (outFile_, &cfg_, frameOutputCnt_);
    }
    fclose (outFile_);
#endif
}

SmartPtr<SmartBuffer> VideoEncoder::encodeAFrame(SmartPtr<SmartBuffer> input, bool* bIsKeyFrame)
{
    SmartPtr<SmartBuffer> result;
    if ( input && input->dataLength() > 0 ) {
        vpx_codec_iter_t iter = NULL;
        const vpx_codec_cx_pkt_t *pkt;
        int flags = layerFlags_[frameInputCnt_ % cfg_.ts_periodicity];
        memcpy( raw_.planes[0], input->data(), input->dataLength() );

        if(vpx_codec_encode(&codec_, &raw_, timestampTick_, 1, flags, VPX_DL_REALTIME)) {
            fprintf(stderr, "!!!Failed to encode frame");
            return NULL;
        }
        while ( (pkt = vpx_codec_get_cx_data(&codec_, &iter)) ) {
            switch (pkt->kind) {
            case VPX_CODEC_CX_FRAME_PKT:
                {
#ifdef DEBUG_SAVE_IVF
                    write_ivf_frame_header(outFile_, pkt);
                    (void) fwrite(pkt->data.frame.buf, 1, pkt->data.frame.sz, outFile_);
#endif
                    *bIsKeyFrame = (flags == VPX_EFLAG_FORCE_KF); 
                    result = new SmartBuffer(pkt->data.frame.sz, (const char*)pkt->data.frame.buf);
                    /*
                    char* buf = (char*)pkt->data.frame.buf;
                    int keyframe  = !(buf[0] & 1);
                    int profile   =  (buf[0]>>1) & 7;
                    int invisible = !(buf[0] & 0x10);
                    fprintf(stderr, "-----------------video encoded frame size=%ld, keyframe=%d, profile=%d, invisible=%d\r\n", pkt->data.frame.sz, keyframe, profile, invisible);
                    */
                    frameOutputCnt_++;
                    break;
                }
            default:
                break;
            }
        }
        timestampTick_++;
        frameInputCnt_++;
    }
    return result;
}
