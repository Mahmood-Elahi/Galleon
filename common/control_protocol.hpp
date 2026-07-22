#pragma once

#include <cstddef>
#include <cstdint>

namespace galleon {

struct ControlPacket {
    std::uint16_t left_adc;
    std::uint16_t right_adc;
    bool button_pressed;
};

enum class ParseResult {
    ok,
    empty,
    oversized,
    malformed,
    out_of_range,
};

inline constexpr std::size_t kMaxControlPacketLength = 31;
inline constexpr std::size_t kControlPacketBufferSize = kMaxControlPacketLength + 1;

bool format_control_packet(const ControlPacket& packet, char* buffer,
                           std::size_t buffer_size, std::size_t& output_length);

ParseResult parse_control_packet(const char* data, std::size_t length,
                                 ControlPacket& packet);

const char* parse_result_name(ParseResult result);

}  // namespace galleon
