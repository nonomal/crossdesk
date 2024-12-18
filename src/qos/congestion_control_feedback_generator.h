/*
 * @Author: DI JUNKUN
 * @Date: 2024-12-18
 * Copyright (c) 2024 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _CONGESTION_CONTROL_FEEDBACK_GENERATOR_H_
#define _CONGESTION_CONTROL_FEEDBACK_GENERATOR_H_

#include "rtp_packet_received.h"

class CongestionControlFeedbackGenerator
    : public RtpTransportFeedbackGenerator {
 public:
  CongestionControlFeedbackGenerator(
      RtpTransportFeedbackGenerator::RtcpSender feedback_sender);
  ~CongestionControlFeedbackGenerator() = default;

  void OnReceivedPacket(const RtpPacketReceived& packet) override;

  void OnSendBandwidthEstimateChanged(DataRate estimate) override;

  int64_t Process(int64_t now_ms) override;

  void SetTransportOverhead(DataSize overhead_per_packet) override;

 private:
  int64_t NextFeedbackTime() const;

  void SendFeedback(int64_t now_ms);

  void CalculateNextPossibleSendTime(int64_t feedback_size, int64_t now_ms);

 private:
  // Feedback should not use more than 5% of the configured send bandwidth
  // estimate. Min and max duration between feedback is configurable using field
  // trials, but per default, min is 25ms and max is 250ms.
  // If possible, given the other constraints, feedback will be sent when a
  // packet with marker bit is received in order to provide feedback as soon as
  // possible after receiving a complete video frame. If no packet with marker
  // bit is received, feedback can be delayed up to 25ms after the first packet
  // since the last sent feedback. On good networks, this means that a sender
  // may receive feedback for every sent frame.
  int64_t min_time_between_feedback_ = 25;
  int64_t max_time_between_feedback_ = 250;
  int64_t max_time_to_wait_for_packet_with_marker_ = 25;

  int64_t max_feedback_rate_ = 1000;  // kbps
  int64_t packet_overhead_ = 0;
  int64_t send_rate_debt_ = 0;

  std::optional<int64_t> first_arrival_time_since_feedback_;
  int64_t next_possible_feedback_send_time_ = 0;
  int64_t last_feedback_sent_time_ = 0;

  bool marker_bit_seen_ = false;
};

#endif