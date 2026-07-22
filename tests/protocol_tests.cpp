#include "control_protocol.hpp"

#include <array>
#include <cassert>
#include <cstring>
#include <iostream>

namespace {

using galleon::ControlPacket;
using galleon::ParseResult;

ParseResult parse(const char* text, ControlPacket& packet) {
    return galleon::parse_control_packet(text, std::strlen(text), packet);
}

void expect_rejected(const char* text) {
    ControlPacket packet{111, 222, true};
    const ControlPacket before = packet;
    assert(parse(text, packet) != ParseResult::ok);
    assert(packet.left_adc == before.left_adc);
    assert(packet.right_adc == before.right_adc);
    assert(packet.button_pressed == before.button_pressed);
}

}  // namespace

int main() {
    ControlPacket packet{};
    assert(parse("L:33320,R:32455,B:1", packet) == ParseResult::ok);
    assert(packet.left_adc == 33320 && packet.right_adc == 32455 && packet.button_pressed);

    assert(parse("L:0,R:0,B:0", packet) == ParseResult::ok);
    assert(packet.left_adc == 0 && packet.right_adc == 0 && !packet.button_pressed);

    assert(parse("L:65535,R:65535,B:1", packet) == ParseResult::ok);
    assert(packet.left_adc == 65535 && packet.right_adc == 65535 && packet.button_pressed);

    assert(parse("L:1,R:2,B:0", packet) == ParseResult::ok);
    assert(!packet.button_pressed);
    assert(parse("L:1,R:2,B:1", packet) == ParseResult::ok);
    assert(packet.button_pressed);

    expect_rejected("R:2,B:0");
    expect_rejected("L:1,B:0");
    expect_rejected("L:1,R:2");
    expect_rejected("X:1,R:2,B:0");
    expect_rejected("L:1,X:2,B:0");
    expect_rejected("L:1,R:2,X:0");
    expect_rejected("L=1,R:2,B:0");
    expect_rejected("L:1;R:2,B:0");
    expect_rejected("L:1,R:2;B:0");
    expect_rejected("L:-1,R:2,B:0");
    expect_rejected("L:1,R:-2,B:0");
    expect_rejected("L:65536,R:2,B:0");
    expect_rejected("L:1,R:70000,B:0");
    expect_rejected("L:1,R:2,B:-1");
    expect_rejected("L:1,R:2,B:2");
    expect_rejected("L:1,R:2,B:0junk");
    expect_rejected("L:1,L:2,R:3,B:0");
    expect_rejected("");

    std::array<char, galleon::kMaxControlPacketLength + 2> oversized{};
    oversized.fill('1');
    ControlPacket unchanged{4, 5, true};
    assert(galleon::parse_control_packet(oversized.data(), oversized.size(), unchanged) ==
           ParseResult::oversized);
    assert(unchanged.left_adc == 4 && unchanged.right_adc == 5 && unchanged.button_pressed);

    char formatted[galleon::kControlPacketBufferSize]{};
    std::size_t length = 0;
    assert(galleon::format_control_packet(ControlPacket{65535, 0, true}, formatted,
                                           sizeof(formatted), length));
    assert(std::strcmp(formatted, "L:65535,R:0,B:1") == 0);
    assert(length == std::strlen(formatted));

    char too_small[4]{};
    assert(!galleon::format_control_packet(packet, too_small, sizeof(too_small), length));

    std::cout << "protocol_tests: all assertions passed\n";
    return 0;
}
