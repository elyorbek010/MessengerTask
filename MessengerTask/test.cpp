#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <vector>
#include <bitset>
#include <iostream>

#include "task1_messenger.hpp"

unsigned short bits2ushort(bool bits[16]) {
    unsigned short decimal(0);

    for (size_t i = 0; i != 16; ++i) {
        header_val |= (static_cast<unsigned short>(arr[i]) << 15 - i);
    }

    return header_val;
}

TEST_CASE("Basic test", "make buff") {
    std::vector<uint8_t> buff = messenger::make_buff(messenger::msg_t("Elyorbek", "Hi"));

    bool header[16] = { /*flag*/1, 0, 1,/*namelen*/ 1, 0, 0, 0, /*msglen*/ 0, 0, 0, 1, 0, /*crc4*/ 0, 0, 0, 0};
    unsigned short buff_header = static_cast<short>buff[1] << 8 + static_cast<short>buff[2];

    REQUIRE(buff_header == bits2ushort(header));
}

TEST_CASE("Basic test", "parse buff") {
    std::vector<uint8_t> buff = messenger::make_buff(messenger::msg_t("Elyorbek", "Hi"));
    messenger::msg_t msg = messenger::parse_buff(buff);

    REQUIRE(msg.name == "Elyorbek");
    REQUIRE(msg.text == "Hi");
}