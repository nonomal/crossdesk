/*
 * @Author: DI JUNKUN
 * @Date: 2024-12-11
 * Copyright (c) 2024 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _TRANSPORT_FEEDBACK_H_
#define _TRANSPORT_FEEDBACK_H_

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

#include <array>
#include <vector>

#include "rtcp_header.h"
#include "rtcp_packet.h"
#include "rtcp_typedef.h"

class TransportFeedback : public RtcpPacket {
 public:
  class ReceivedPacket {
   public:
    ReceivedPacket(uint16_t sequence_number, int16_t delta_ticks)
        : sequence_number_(sequence_number), delta_ticks_(delta_ticks) {}
    ReceivedPacket(const ReceivedPacket&) = default;
    ReceivedPacket& operator=(const ReceivedPacket&) = default;

    uint16_t sequence_number() const { return sequence_number_; }
    int16_t delta_ticks() const { return delta_ticks_; }
    int64_t delta() const { return delta_ticks_ * kDeltaTick; }

   private:
    uint16_t sequence_number_;
    int16_t delta_ticks_;
  };

  static constexpr uint8_t kFeedbackMessageType = 15;
  static constexpr int64_t kDeltaTick = 250;  // 0.25ms
  // Maximum number of packets (including missing) TransportFeedback can report.
  static constexpr size_t kMaxReportedPackets = 0xffff;

 private:
  // Size in bytes of a delta time in rtcp packet.
  // Valid values are 0 (packet wasn't received), 1 or 2.
  using DeltaSize = uint8_t;
  // Keeps DeltaSizes that can be encoded into single chunk if it is last chunk.
  class LastChunk {
   public:
    using DeltaSize = TransportFeedback::DeltaSize;
    static constexpr size_t kMaxRunLengthCapacity = 0x1fff;

    LastChunk();

    bool Empty() const;
    void Clear();
    // Return if delta sizes still can be encoded into single chunk with added
    // `delta_size`.
    bool CanAdd(DeltaSize delta_size) const;
    // Add `delta_size`, assumes `CanAdd(delta_size)`,
    void Add(DeltaSize delta_size);
    // Equivalent to calling Add(0) `num_missing` times. Assumes `Empty()`.
    void AddMissingPackets(size_t num_missing);

    // Encode chunk as large as possible removing encoded delta sizes.
    // Assume CanAdd() == false for some valid delta_size.
    uint16_t Emit();
    // Encode all stored delta_sizes into single chunk, pad with 0s if needed.
    uint16_t EncodeLast() const;

    // Decode up to `max_size` delta sizes from `chunk`.
    void Decode(uint16_t chunk, size_t max_size);
    // Appends content of the Lastchunk to `deltas`.
    void AppendTo(std::vector<DeltaSize>* deltas) const;

   private:
    static constexpr size_t kMaxOneBitCapacity = 14;
    static constexpr size_t kMaxTwoBitCapacity = 7;
    static constexpr size_t kMaxVectorCapacity = kMaxOneBitCapacity;
    static constexpr DeltaSize kLarge = 2;

    uint16_t EncodeOneBit() const;
    void DecodeOneBit(uint16_t chunk, size_t max_size);

    uint16_t EncodeTwoBit(size_t size) const;
    void DecodeTwoBit(uint16_t chunk, size_t max_size);

    uint16_t EncodeRunLength() const;
    void DecodeRunLength(uint16_t chunk, size_t max_size);

    std::array<DeltaSize, kMaxVectorCapacity> delta_sizes_;
    size_t size_;
    bool all_same_;
    bool has_large_delta_;
  };

 public:
  TransportFeedback();
  ~TransportFeedback();

 public:
  bool AddReceivedPacket(uint16_t sequence_number, int64_t timestamp);
  bool AddMissingPackets(size_t num_missing_packets);
  bool AddDeltaSize(DeltaSize delta_size);
  void Clear();

  bool Create(std::vector<uint8_t>& packet, size_t max_length,
              PacketReadyCallback callback) const override;

 public:
  void SetBaseSequenceNumber(uint16_t base_sequence_number);
  void SetPacketStatusCount(uint16_t packet_status_count);
  void SetReferenceTime(uint32_t reference_time);
  void SetFeedbackPacketCount(uint8_t feedback_packet_count);

  int64_t BaseTime() const;
  int64_t GetBaseDelta(int64_t prev_timestamp) const;

 private:
  uint16_t base_seq_no_;
  uint16_t pkt_stat_cnt_;
  uint32_t ref_time_;
  uint8_t feedback_pkt_cnt_;

  int64_t last_ts_;

  std::vector<ReceivedPacket> received_packets_;
  std::vector<ReceivedPacket> all_packets_;
  // All but last encoded packet chunks.
  std::vector<uint16_t> encoded_chunks_;
  LastChunk last_chunk_;
  size_t size_bytes_;
};

#endif