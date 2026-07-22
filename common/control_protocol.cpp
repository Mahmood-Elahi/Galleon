#include "control_protocol.hpp"

#include <cstdio>
#include <limits>

namespace galleon {
namespace {

ParseResult parse_field(const char* data, std::size_t length, std::size_t& position,
                        char expected_name, std::uint32_t maximum,
                        std::uint32_t& value) {
    if (position + 2 > length || data[position] != expected_name ||
        data[position + 1] != ':') {
        return ParseResult::malformed;
    }
    position += 2;
    if (position >= length || data[position] < '0' || data[position] > '9') {
        return ParseResult::malformed;
    }

    std::uint32_t parsed = 0;
    while (position < length && data[position] >= '0' && data[position] <= '9') {
        const std::uint32_t digit = static_cast<std::uint32_t>(data[position] - '0');
        if (parsed > maximum / 10U ||
            (parsed == maximum / 10U && digit > maximum % 10U)) {
            return ParseResult::out_of_range;
        }
        parsed = parsed * 10U + digit;
        ++position;
    }
    value = parsed;
    return ParseResult::ok;
}

}  // namespace

bool format_control_packet(const ControlPacket& packet, char* buffer,
                           std::size_t buffer_size, std::size_t& output_length) {
    output_length = 0;
    if (buffer == nullptr || buffer_size == 0) {
        return false;
    }
    const int written = std::snprintf(buffer, buffer_size, "L:%u,R:%u,B:%u",
                                      static_cast<unsigned>(packet.left_adc),
                                      static_cast<unsigned>(packet.right_adc),
                                      packet.button_pressed ? 1U : 0U);
    if (written < 0 || static_cast<std::size_t>(written) >= buffer_size) {
        buffer[0] = '\0';
        return false;
    }
    output_length = static_cast<std::size_t>(written);
    return true;
}

ParseResult parse_control_packet(const char* data, std::size_t length,
                                 ControlPacket& packet) {
    if (data == nullptr || length == 0) {
        return ParseResult::empty;
    }
    if (length > kMaxControlPacketLength) {
        return ParseResult::oversized;
    }

    std::size_t position = 0;
    std::uint32_t left = 0;
    std::uint32_t right = 0;
    std::uint32_t button = 0;

    ParseResult result = parse_field(data, length, position, 'L', 65535U, left);
    if (result != ParseResult::ok) return result;
    if (position >= length || data[position++] != ',') return ParseResult::malformed;

    result = parse_field(data, length, position, 'R', 65535U, right);
    if (result != ParseResult::ok) return result;
    if (position >= length || data[position++] != ',') return ParseResult::malformed;

    result = parse_field(data, length, position, 'B', 1U, button);
    if (result != ParseResult::ok) return result;
    if (position != length) return ParseResult::malformed;

    const ControlPacket parsed{
        static_cast<std::uint16_t>(left),
        static_cast<std::uint16_t>(right),
        button == 1U,
    };
    packet = parsed;
    return ParseResult::ok;
}

const char* parse_result_name(ParseResult result) {
    switch (result) {
        case ParseResult::ok: return "ok";
        case ParseResult::empty: return "empty";
        case ParseResult::oversized: return "oversized";
        case ParseResult::malformed: return "malformed";
        case ParseResult::out_of_range: return "out_of_range";
    }
    return "unknown";
}

}  // namespace galleon
