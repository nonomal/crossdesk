#include "video_frame.h"

#include <cstring>
#include <utility>

VideoFrame::VideoFrame() {}

VideoFrame::VideoFrame(size_t size) : buffer_(size) {
  size_ = size;
  width_ = 0;
  height_ = 0;
}

VideoFrame::VideoFrame(size_t size, uint32_t width, uint32_t height)
    : buffer_(size) {
  size_ = size;
  width_ = width;
  height_ = height;
}

VideoFrame::VideoFrame(const uint8_t *buffer, size_t size)
    : buffer_(buffer, size) {
  size_ = size;
  width_ = 0;
  height_ = 0;
}

VideoFrame::VideoFrame(const uint8_t *buffer, size_t size, uint32_t width,
                       uint32_t height)
    : buffer_(buffer, size) {
  size_ = size;
  width_ = width;
  height_ = height;
}

VideoFrame::VideoFrame(const VideoFrame &video_frame) = default;

VideoFrame::VideoFrame(VideoFrame &&video_frame) = default;

VideoFrame &VideoFrame::operator=(const VideoFrame &video_frame) = default;

VideoFrame &VideoFrame::operator=(VideoFrame &&video_frame) = default;

VideoFrame::~VideoFrame() = default;