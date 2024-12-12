#include "transport_feedback.h"

#include "byte_io.h"
#include "log.h"
#include "sequence_number_compare.h"

// Header size:
// * 4 bytes Common RTCP Packet Header
// * 8 bytes Common Packet Format for RTCP Feedback Messages
// * 8 bytes FeedbackPacket header
constexpr size_t kTransportFeedbackHeaderSizeBytes = 4 + 8 + 8;
constexpr size_t kChunkSizeBytes = 2;
// TODO(sprang): Add support for dynamic max size for easier fragmentation,
// eg. set it to what's left in the buffer or IP_PACKET_SIZE.
// Size constraint imposed by RTCP common header: 16bit size field interpreted
// as number of four byte words minus the first header word.
constexpr size_t kMaxSizeBytes = (1 << 16) * 4;
// Payload size:
// * 8 bytes Common Packet Format for RTCP Feedback Messages
// * 8 bytes FeedbackPacket header.
// * 2 bytes for one chunk.
constexpr size_t kMinPayloadSizeBytes = 8 + 8 + 2;
constexpr int64_t kBaseTimeTick =
    TransportFeedback::kDeltaTick * (1 << 8);                   // 64ms
constexpr int64_t kTimeWrapPeriod = kBaseTimeTick * (1 << 24);  // 12.43days

TransportFeedback::TransportFeedback()
    : base_seq_no_(0), pkt_stat_cnt_(0), ref_time_(0), feedback_pkt_cnt_(0) {}

TransportFeedback::~TransportFeedback() {}

bool TransportFeedback::AddReceivedPacket(uint16_t sequence_number,
                                          int64_t timestamp) {
  int16_t delta = 0;
  // Convert to ticks and round.
  if (last_ts_ > timestamp) {
    timestamp += (last_ts_ - timestamp) + kTimeWrapPeriod;
  }

  int64_t delta_full = timestamp - last_ts_ % kTimeWrapPeriod;
  if (delta_full > kTimeWrapPeriod / 2) {
    delta_full -= kTimeWrapPeriod;
    delta_full -= kDeltaTick / 2;
  } else {
    delta_full += kDeltaTick / 2;
  }
  delta_full /= kDeltaTick;

  delta = static_cast<int16_t>(delta_full);
  // If larger than 16bit signed, we can't represent it - need new fb packet.
  if (delta != delta_full) {
    LOG_WARN("Delta value too large ( >= 2^16 ticks )");
    return false;
  }

  uint16_t next_seq_no = base_seq_no_ + pkt_stat_cnt_;
  if (sequence_number != next_seq_no) {
    uint16_t last_seq_no = next_seq_no - 1;
    if (!IsNewerSequenceNumber(sequence_number, last_seq_no)) return false;
    uint16_t num_missing_packets = sequence_number - next_seq_no;
    if (!AddMissingPackets(num_missing_packets)) return false;
  }

  DeltaSize delta_size = (delta >= 0 && delta <= 0xff) ? 1 : 2;
  if (!AddDeltaSize(delta_size)) return false;

  received_packets_.emplace_back(sequence_number, delta);
  last_ts_ += delta * kDeltaTick;
  size_bytes_ += delta_size;

  return true;
}

bool TransportFeedback::AddMissingPackets(size_t num_missing_packets) {
  size_t new_num_seq_no = pkt_stat_cnt_ + num_missing_packets;
  if (new_num_seq_no > kMaxReportedPackets) {
    return false;
  }

  if (!last_chunk_.Empty()) {
    while (num_missing_packets > 0 && last_chunk_.CanAdd(0)) {
      last_chunk_.Add(0);
      --num_missing_packets;
    }
    if (num_missing_packets == 0) {
      pkt_stat_cnt_ = new_num_seq_no;
      return true;
    }
    encoded_chunks_.push_back(last_chunk_.Emit());
  }

  size_t full_chunks = num_missing_packets / LastChunk::kMaxRunLengthCapacity;
  size_t partial_chunk = num_missing_packets % LastChunk::kMaxRunLengthCapacity;
  size_t num_chunks = full_chunks + (partial_chunk > 0 ? 1 : 0);
  if (size_bytes_ + kChunkSizeBytes * num_chunks > kMaxSizeBytes) {
    pkt_stat_cnt_ = (new_num_seq_no - num_missing_packets);
    return false;
  }
  size_bytes_ += kChunkSizeBytes * num_chunks;
  // T = 0, S = 0, run length = kMaxRunLengthCapacity, see EncodeRunLength().
  encoded_chunks_.insert(encoded_chunks_.end(), full_chunks,
                         LastChunk::kMaxRunLengthCapacity);
  last_chunk_.AddMissingPackets(partial_chunk);
  pkt_stat_cnt_ = new_num_seq_no;
  return true;
}

bool TransportFeedback::AddDeltaSize(DeltaSize delta_size) {
  if (pkt_stat_cnt_ == kMaxReportedPackets) return false;
  size_t add_chunk_size = last_chunk_.Empty() ? kChunkSizeBytes : 0;
  if (size_bytes_ + delta_size + add_chunk_size > kMaxSizeBytes) return false;

  if (last_chunk_.CanAdd(delta_size)) {
    size_bytes_ += add_chunk_size;
    last_chunk_.Add(delta_size);
    ++pkt_stat_cnt_;
    return true;
  }
  if (size_bytes_ + delta_size + kChunkSizeBytes > kMaxSizeBytes) return false;

  encoded_chunks_.push_back(last_chunk_.Emit());
  size_bytes_ += kChunkSizeBytes;
  last_chunk_.Add(delta_size);
  ++pkt_stat_cnt_;
  return true;
}

void TransportFeedback::Clear() {
  pkt_stat_cnt_ = 0;
  last_ts_ = BaseTime();
  received_packets_.clear();
  all_packets_.clear();
  encoded_chunks_.clear();
  last_chunk_.Clear();
  size_bytes_ = kTransportFeedbackHeaderSizeBytes;
}

// Serialize packet.
bool TransportFeedback::Create(uint8_t* packet, size_t* position,
                               size_t max_length,
                               PacketReadyCallback callback) const {
  if (pkt_stat_cnt_ == 0) return false;

  while (*position + BlockLength() > max_length) {
    if (!OnBufferFull(packet, position, callback)) return false;
  }
  const size_t position_end = *position + BlockLength();
  const size_t padding_length = PaddingLength();
  bool has_padding = padding_length > 0;
  CreateHeader(kFeedbackMessageType, kPacketType, HeaderLength(), has_padding,
               packet, position);
  CreateCommonFeedback(packet + *position);
  *position += kCommonFeedbackLength;

  ByteWriter<uint16_t>::WriteBigEndian(&packet[*position], base_seq_no_);
  *position += 2;

  ByteWriter<uint16_t>::WriteBigEndian(&packet[*position], pkt_stat_cnt_);
  *position += 2;

  ByteWriter<uint32_t, 3>::WriteBigEndian(&packet[*position], ref_time_);
  *position += 3;

  packet[(*position)++] = feedback_pkt_cnt_;

  for (uint16_t chunk : encoded_chunks_) {
    ByteWriter<uint16_t>::WriteBigEndian(&packet[*position], chunk);
    *position += 2;
  }
  if (!last_chunk_.Empty()) {
    uint16_t chunk = last_chunk_.EncodeLast();
    ByteWriter<uint16_t>::WriteBigEndian(&packet[*position], chunk);
    *position += 2;
  }

  for (const auto& received_packet : received_packets_) {
    int16_t delta = received_packet.delta_ticks();
    if (delta >= 0 && delta <= 0xFF) {
      packet[(*position)++] = delta;
    } else {
      ByteWriter<int16_t>::WriteBigEndian(&packet[*position], delta);
      *position += 2;
    }
  }

  if (padding_length > 0) {
    for (size_t i = 0; i < padding_length - 1; ++i) {
      packet[(*position)++] = 0;
    }
    packet[(*position)++] = padding_length;
  }

  if (*position != position_end) {
    LOG_FATAL("padding_length is too small");
  }
  return true;
}

void TransportFeedback::SetBaseSequenceNumber(uint16_t base_sequence) {
  base_seq_no_ = base_sequence;
}

void TransportFeedback::SetPacketStatusCount(uint16_t packet_status_count) {
  pkt_stat_cnt_ = packet_status_count;
}

void TransportFeedback::SetReferenceTime(uint32_t reference_time) {
  ref_time_ = reference_time;
}

void TransportFeedback::SetFeedbackPacketCount(uint8_t feedback_packet_count) {
  feedback_pkt_cnt_ = feedback_packet_count;
}

int64_t TransportFeedback::BaseTime() const {
  // Add an extra kTimeWrapPeriod to allow add received packets arrived earlier
  // than the first added packet (and thus allow to record negative deltas)
  // even when ref_time_ == 0.
  return 0 + kTimeWrapPeriod + int64_t{ref_time_} * kBaseTimeTick;
}

int64_t TransportFeedback::GetBaseDelta(int64_t prev_timestamp) const {
  int64_t delta = BaseTime() - prev_timestamp;
  // Compensate for wrap around.
  if (std::abs(delta - kTimeWrapPeriod) < std::abs(delta)) {
    delta -= kTimeWrapPeriod;  // Wrap backwards.
  } else if (std::abs(delta + kTimeWrapPeriod) < std::abs(delta)) {
    delta += kTimeWrapPeriod;  // Wrap forwards.
  }
  return delta;
}

size_t TransportFeedback::BlockLength() const {
  // Round size_bytes_ up to multiple of 32bits.
  return (size_bytes_ + 3) & (~static_cast<size_t>(3));
}

size_t TransportFeedback::PaddingLength() const {
  return BlockLength() - size_bytes_;
}

void TransportFeedback::ParseCommonFeedback(const uint8_t* payload) {
  SetSenderSsrc(ByteReader<uint32_t>::ReadBigEndian(&payload[0]));
  SetMediaSsrc(ByteReader<uint32_t>::ReadBigEndian(&payload[4]));
}

void TransportFeedback::CreateCommonFeedback(uint8_t* payload) const {
  ByteWriter<uint32_t>::WriteBigEndian(&payload[0], sender_ssrc());
  ByteWriter<uint32_t>::WriteBigEndian(&payload[4], media_ssrc());
}