#include "aom_av1_decoder.h"

#include "log.h"

// #define SAVE_DECODED_NV12_STREAM
// #define SAVE_RECEIVED_AV1_STREAM

AomAv1Decoder::AomAv1Decoder(std::shared_ptr<SystemClock> clock)
    : clock_(clock) {}

AomAv1Decoder::~AomAv1Decoder() {
#ifdef SAVE_DECODED_NV12_STREAM
  if (file_nv12_) {
    fflush(file_nv12_);
    fclose(file_nv12_);
    file_nv12_ = nullptr;
  }
#endif

#ifdef SAVE_RECEIVED_AV1_STREAM
  if (file_av1_) {
    fflush(file_av1_);
    fclose(file_av1_);
    file_av1_ = nullptr;
  }
#endif

  if (nv12_frame_) {
    delete[] nv12_frame_;
    nv12_frame_ = nullptr;
  }
}

int AomAv1Decoder::Init() {
  aom_av1_decoder_config_.threads = 8;
  aom_av1_decoder_config_.w = 2560;
  aom_av1_decoder_config_.h = 1440;
  aom_av1_decoder_config_.allow_lowbitdepth = 1;

  extern aom_codec_iface_t my_codec;
  if (AOM_CODEC_OK != aom_codec_dec_init(&aom_av1_decoder_ctx_,
                                         aom_codec_av1_dx(),
                                         &aom_av1_decoder_config_, 0)) {
    LOG_ERROR("Failed to initialize decoder");
    return -1;
  }

  aom_codec_control(&aom_av1_decoder_ctx_, AV1D_GET_IMG_FORMAT,
                    AOM_IMG_FMT_NV12);

#ifdef SAVE_DECODED_NV12_STREAM
  file_nv12_ = fopen("decoded_nv12_stream.yuv", "w+b");
  if (!file_nv12_) {
    LOG_WARN("Fail to open decoded_nv12_stream.yuv");
  }
#endif

#ifdef SAVE_RECEIVED_AV1_STREAM
  file_av1_ = fopen("received_av1_stream.ivf", "w+b");
  if (!file_av1_) {
    LOG_WARN("Fail to open received_av1_stream.ivf");
  }
#endif

  return 0;
}

int AomAv1Decoder::Decode(
    std::unique_ptr<ReceivedFrame> received_frame,
    std::function<void(const DecodedFrame &)> on_receive_decoded_frame) {
  const uint8_t *data = received_frame->Buffer();
  size_t size = received_frame->Size();

#ifdef SAVE_RECEIVED_AV1_STREAM
  fwrite((unsigned char *)data, 1, size, file_av1_);
#endif

  aom_codec_iter_t iter = nullptr;
  aom_codec_err_t ret =
      aom_codec_decode(&aom_av1_decoder_ctx_, data, size, nullptr);
  if (AOM_CODEC_OK != ret) {
    const char *error = aom_codec_error(&aom_av1_decoder_ctx_);
    const char *detail = aom_codec_error_detail(&aom_av1_decoder_ctx_);
    if (detail && error) {
      LOG_ERROR("Failed to decode frame [{}], additional information [{}]",
                error, detail);
    } else {
      LOG_ERROR("Failed to decode frame, ret [{}]",
                aom_codec_err_to_string(ret));
    }
    return -1;
  }

  img_ = aom_codec_get_frame(&aom_av1_decoder_ctx_, &iter);
  if (img_) {
    {
      aom_codec_frame_flags_t flags;
      ret = aom_codec_control(&aom_av1_decoder_ctx_, AOMD_GET_FRAME_FLAGS,
                              &flags);
      if (ret == AOM_CODEC_OK) {
        if (flags & AOM_FRAME_IS_KEY) {
          LOG_ERROR("Key frame");
        } else {
          LOG_ERROR("Inter frame");
        }
        if (flags & (AOM_FRAME_IS_KEY | AOM_FRAME_IS_INTRAONLY)) {
          LOG_ERROR("Key frame");
        } else if (flags & AOM_FRAME_IS_SWITCH) {
          LOG_ERROR("Switch frame");
        } else {
          LOG_ERROR("P frame");
        }
      }
    }
    int corrupted = 0;
    ret = aom_codec_control(&aom_av1_decoder_ctx_, AOMD_GET_FRAME_CORRUPTED,
                            &corrupted);
    if (ret != AOM_CODEC_OK) {
      LOG_ERROR("Failed to get frame corrupted");
      return -1;
    }

    size_t nv12_size = img_->d_w * img_->d_h + img_->d_w * img_->d_h / 2;
    std::vector<uint8_t> nv12_data(nv12_size);

    uint8_t *y_data = nv12_data.data();
    memcpy(y_data, img_->planes[0], img_->d_w * img_->d_h);

    uint8_t *uv_data = nv12_data.data() + img_->d_w * img_->d_h;
    uint8_t *u_plane = img_->planes[1];
    uint8_t *v_plane = img_->planes[2];

    for (int i = 0; i < img_->d_w * img_->d_h / 2; i++) {
      uv_data[2 * i] = u_plane[i];
      uv_data[2 * i + 1] = v_plane[i];
    }

    DecodedFrame decode_frame(nv12_data.data(), nv12_size, img_->d_w,
                              img_->d_h);

    decode_frame.SetReceivedTimestamp(received_frame->ReceivedTimestamp());
    decode_frame.SetCapturedTimestamp(received_frame->CapturedTimestamp());
    decode_frame.SetDecodedTimestamp(clock_->CurrentTime());

#ifdef SAVE_DECODED_NV12_STREAM
    fwrite((unsigned char *)decode_frame.Buffer(), 1, decode_frame.Size(),
           file_nv12_);
#endif
    on_receive_decoded_frame(decode_frame);

    return 0;
  }

  return -1;
}