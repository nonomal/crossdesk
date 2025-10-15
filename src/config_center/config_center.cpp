#include "config_center.h"

#include "rd_log.h"

ConfigCenter::ConfigCenter() {}

ConfigCenter::~ConfigCenter() {}

int ConfigCenter::SetLanguage(LANGUAGE language) {
  language_ = language;
  return 0;
}

int ConfigCenter::SetVideoQuality(VIDEO_QUALITY video_quality) {
  video_quality_ = video_quality;
  return 0;
}

int ConfigCenter::SetVideoFrameRate(VIDEO_FRAME_RATE video_frame_rate) {
  video_frame_rate_ = video_frame_rate;
  return 0;
}

int ConfigCenter::SetVideoEncodeFormat(
    VIDEO_ENCODE_FORMAT video_encode_format) {
  video_encode_format_ = video_encode_format;
  return 0;
}

int ConfigCenter::SetHardwareVideoCodec(bool hardware_video_codec) {
  hardware_video_codec_ = hardware_video_codec;
  return 0;
}

int ConfigCenter::SetTurn(bool enable_turn) {
  enable_turn_ = enable_turn;
  return 0;
}

int ConfigCenter::SetSrtp(bool enable_srtp) {
  enable_srtp_ = enable_srtp;
  return 0;
}

ConfigCenter::LANGUAGE ConfigCenter::GetLanguage() { return language_; }

ConfigCenter::VIDEO_QUALITY ConfigCenter::GetVideoQuality() {
  return video_quality_;
}

int ConfigCenter::GetVideoFrameRate() {
  int fps = video_frame_rate_ == VIDEO_FRAME_RATE::FPS_30 ? 30 : 60;
  return fps;
}

ConfigCenter::VIDEO_ENCODE_FORMAT ConfigCenter::GetVideoEncodeFormat() {
  return video_encode_format_;
}

bool ConfigCenter::IsHardwareVideoCodec() { return hardware_video_codec_; }

bool ConfigCenter::IsEnableTurn() { return enable_turn_; }

bool ConfigCenter::IsEnableSrtp() { return enable_srtp_; }