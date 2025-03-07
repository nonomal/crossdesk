#include "resolution_adapter.h"

#include "libyuv.h"
#include "log.h"

int ResolutionAdapter::GetResolution(int target_bitrate, int current_width,
                                     int current_height, int* target_width,
                                     int* target_height) {
  for (auto& resolution : GetBitrateLimits()) {
    if (target_bitrate >= resolution.min_start_bitrate_bps &&
        target_bitrate <= resolution.max_bitrate_bps) {
      if (current_width * current_height <= resolution.frame_size_pixels) {
        *target_width = current_width;
        *target_height = current_height;
        LOG_INFO("1 source resolution {}x{}, target resolution {}x{}",
                 current_width, current_height, *target_width, *target_height);
        return 0;
      } else {
        *target_width = current_width * 3 / 5;
        *target_height = current_height * 3 / 5;
        LOG_INFO("2 source resolution {}x{}, target resolution {}x{}",
                 current_width, current_height, *target_width, *target_height);
        return 0;
      }
    }
  }

  *target_width = -1;
  *target_height = -1;

  LOG_INFO("3 source resolution {}x{}, target resolution {}x{}", current_width,
           current_height, *target_width, *target_height);

  return -1;
}

int ResolutionAdapter::ResolutionDowngrade(const XVideoFrame* video_frame,
                                           int target_width, int target_height,
                                           XVideoFrame* new_frame) {
  if (target_width <= 0 || target_height <= 0) {
    return -1;
  }

  target_width = video_frame->width * 3 / 5;
  target_height = video_frame->height * 3 / 5;
  new_frame->width = target_width;
  new_frame->height = target_height;
  new_frame->data = new char[target_width * target_height * 3 / 2];

  libyuv::NV12Scale((const uint8_t*)(video_frame->data), video_frame->width,
                    (const uint8_t*)(video_frame->data +
                                     video_frame->width * video_frame->height),
                    video_frame->width, video_frame->width, video_frame->height,
                    (uint8_t*)(new_frame->data), target_width,
                    (uint8_t*)(new_frame->data + target_width * target_height),
                    target_width, target_width, target_height,
                    libyuv::kFilterLinear);

  return 0;
}