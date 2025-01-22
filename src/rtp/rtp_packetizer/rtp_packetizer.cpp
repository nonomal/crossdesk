#include "rtp_packetizer.h"

std::unique_ptr<RtpPacketizer> Create(uint32_t payload_type, uint8_t* payload,
                                      size_t payload_size) {
  switch (payload_type) {
    case RtpPacket::PAYLOAD_TYPE::H264:
      return std::make_unique<RtpPacketizerH264>(payload, payload_size);
    case RtpPacket::PAYLOAD_TYPE::AV1:
      return std::make_unique<RtpPacketizerAv1>(payload, payload_size);
  }
}