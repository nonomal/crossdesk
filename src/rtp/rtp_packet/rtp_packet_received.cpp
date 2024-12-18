#include "rtp_packet_received.h"

RtpPacketReceived::RtpPacketReceived() = default;
RtpPacketReceived::RtpPacketReceived(int64_t arrival_time)
    : arrival_time_(arrival_time) {}
RtpPacketReceived::RtpPacketReceived(const RtpPacketReceived& packet) = default;
RtpPacketReceived::RtpPacketReceived(RtpPacketReceived&& packet) = default;

RtpPacketReceived& RtpPacketReceived::operator=(
    const RtpPacketReceived& packet) = default;
RtpPacketReceived& RtpPacketReceived::operator=(RtpPacketReceived&& packet) =
    default;

RtpPacketReceived::~RtpPacketReceived() {}
