#include "rtcp_tcc.h"

RtcpTcc::RtcpTcc() {
  buffer_ = new uint8_t[DEFAULT_RR_SIZE];
  size_ = DEFAULT_RR_SIZE;
}

RtcpTcc::~RtcpTcc() {
  if (buffer_) {
    delete buffer_;
    buffer_ = nullptr;
  }

  size_ = 0;
}

void RtcpTcc::SetReportBlock(RtcpReportBlock &rtcp_report_block) {
  reports_.push_back(rtcp_report_block);
}

void RtcpTcc::SetReportBlock(std::vector<RtcpReportBlock> &rtcp_report_blocks) {
  reports_ = rtcp_report_blocks;
}

const uint8_t *RtcpTcc::Encode() {
  rtcp_header_.Encode(DEFAULT_RTCP_VERSION, 0, DEFAULT_RR_BLOCK_NUM,
                      RTCP_TYPE::RR, DEFAULT_RR_SIZE, buffer_);

  return buffer_;
}