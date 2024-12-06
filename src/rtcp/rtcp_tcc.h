/*
 * @Author: DI JUNKUN
 * @Date: 2024-12-06
 * Copyright (c) 2024 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _RTCP_TCC_H_
#define _RTCP_TCC_H_

// RR
//  0                   1                   2                   3
//  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |V=2|P|  FMT=15 |    PT=205     |           length              |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                     SSRC of packet sender                     |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                      SSRC of media source                     |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |      base sequence number     |      packet status count      |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                 reference time                | fb pkt. count |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |          packet chunk         |         packet chunk          |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// .                                                               .
// .                                                               .
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |         packet chunk          |  recv delta   |  recv delta   |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// .                                                               .
// .                                                               .
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |           recv delta          |  recv delta   | zero padding  |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

// RTP transport sequence number
// 0                   1                   2                   3
//  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |       0xBE    |    0xDE       |           length=1            |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |  ID   | L=1   |transport-wide sequence number | zero padding  |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

#include <vector>

#include "rtcp_header.h"
#include "rtcp_typedef.h"

class RtcpTcc {
 public:
  RtcpTcc();
  ~RtcpTcc();

 public:
  void SetReportBlock(RtcpReportBlock &rtcp_report_block);
  void SetReportBlock(std::vector<RtcpReportBlock> &rtcp_report_blocks);

 public:
  const uint8_t *Encode();
  size_t Decode();

  // Entire RTP buffer
  const uint8_t *Buffer() const { return buffer_; }
  size_t Size() const { return size_; }

 private:
  RtcpHeader rtcp_header_;
  std::vector<RtcpReportBlock> reports_;

  // Entire RTCP buffer
  uint8_t *buffer_ = nullptr;
  size_t size_ = 0;
};

#endif