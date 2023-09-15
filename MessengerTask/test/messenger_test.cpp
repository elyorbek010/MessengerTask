#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <stdexcept>
#include <vector>
#include <bitset>
#include <iostream>

#include "../inc/task1_messenger.hpp"

#define HEADER_SIZE (2)
#define MAX_MESSAGE_SIZE (31)

TEST_CASE("EmptyName", "MakeBuff") {
	std::string name("");
	std::string message("Hi");

	try 
	{
		const std::vector<uint8_t>& buff = messenger::make_buff(messenger::msg_t(name, message));
	}
	catch (const std::length_error& error) {
		std::cout << error.what() << std::endl;
	}
}

TEST_CASE("EmptyMessage", "MakeBuff") {
	std::string name("Elyorbek");
	std::string message("");

	try
	{
		const std::vector<uint8_t>& buff = messenger::make_buff(messenger::msg_t(name, message));
	}
	catch (const std::length_error& error) {
		std::cout << error.what() << std::endl;
	}
}

TEST_CASE("NameLimit", "MakeBuff") {
	std::string name("ElyorbekElyorbek");
	std::string message("Hi");

	try
	{
		const std::vector<uint8_t>& buff = messenger::make_buff(messenger::msg_t(name, message));
	}
	catch (const std::length_error& error) {
		std::cout << error.what() << std::endl;
	}
}

TEST_CASE("CRCCheck", "ParseBuff") {
	std::string name("Elyorbek");
	std::string text("Hi");

	const std::vector<uint8_t>& buff = messenger::make_buff(messenger::msg_t(name, text));
	std::vector<uint8_t> buff_cpy = buff;

	buff_cpy[1] &= 0xf0; // clear crc field

	try {
		const messenger::msg_t& message = messenger::parse_buff(buff_cpy);
	}
	catch (const std::runtime_error& error) {
		std::cout << error.what() << std::endl;
	}
}

TEST_CASE("1PacketTestMakeBuff", "MakeBuff") {
	std::string name("Elyorbek");
	std::string message("Hi");

    const std::vector<uint8_t> &buff = messenger::make_buff(messenger::msg_t(name, message));
	// flag			5  = 0b101
	// namelen		8  = 0b1000
	// msglen		2  = 0b00010
	// crc4			10 = 0b1010
	// header 1011 0000 0010 1010
	unsigned short correct_header = 0b1011000000101010;

	unsigned short buffer_header = (static_cast<unsigned short>(buff[0]) << 8) +
								  (static_cast<unsigned short>(buff[1]));

    REQUIRE(buffer_header == correct_header);

	for (size_t i = 0; i < name.size(); i++)
		REQUIRE(buff[HEADER_SIZE + i] == name[i]);

	for (size_t i = 0; i < message.size(); i++)
		REQUIRE(buff[HEADER_SIZE + name.size() + i] == message[i]);
}

TEST_CASE("2PacketTestMakeBuff", "MakeBuff") {
	std::string name("Elyorbek");
	std::string text("This is a pretty long text that takes 2 packets");

	const std::vector<uint8_t>& buff = messenger::make_buff(messenger::msg_t(name, text));
	// PACKET 1
	// flag			5  = 0b101
	// namelen		8  = 0b1000
	// msglen		31 = 0b11111
	// crc4			10 = 0b1100
	// header 1011 0001 1111 1100
	// 
	// PACKET 2
	// flag			5  = 0b101
	// namelen		8  = 0b1000
	// msglen		16 = 0b10000
	// crc4			11 = 0b1011
	// header 1011 0001 0000 1011
	unsigned long correct_header_1 = 0b1011000111111100;
	unsigned long correct_header_2 = 0b1011000100001011;

	SECTION("Packet number 1") {
		unsigned short buffer_header_1 = (static_cast<unsigned short>(buff[0]) << 8) +
			(static_cast<unsigned short>(buff[1]));

		REQUIRE(buffer_header_1 == correct_header_1);

		for (size_t i = 0; i < name.size(); i++)
			REQUIRE(buff[HEADER_SIZE + i] == name[i]);

		for (size_t i = 0; i < MAX_MESSAGE_SIZE; i++)
			REQUIRE(buff[HEADER_SIZE + name.size() + i] == text[i]);
	}

	SECTION("Packet number 2") {
		unsigned short buffer_header_2 = (static_cast<unsigned short>(buff[41]) << 8) +
			(static_cast<unsigned short>(buff[42]));

		REQUIRE(buffer_header_2 == correct_header_2);

		for (size_t i = 0; i < name.size(); i++)
			REQUIRE(buff[2 * HEADER_SIZE + name.size() + MAX_MESSAGE_SIZE + i] == name[i]);

		for (size_t i = 0; i < text.size() - MAX_MESSAGE_SIZE; i++)
			REQUIRE(buff[2 * HEADER_SIZE + 2 * name.size() + MAX_MESSAGE_SIZE + i] == text[MAX_MESSAGE_SIZE + i]);
	}
}

TEST_CASE("1PacketTestParseBuff", "ParseBuff") {
	std::string name("Elyorbek");
	std::string text("Hi");

	const std::vector<uint8_t>& buff = messenger::make_buff(messenger::msg_t(name, text));
	std::vector<uint8_t> buff_cpy = buff;

	const messenger::msg_t& message = messenger::parse_buff(buff_cpy);

	REQUIRE(message.name == name);
	REQUIRE(message.text == text);
}

TEST_CASE("2PacketTestParseBuff", "ParseBuff") {
	std::string name("Elyorbek");
	std::string text("This is a pretty long text that takes 2 packets");

	const std::vector<uint8_t>& buff = messenger::make_buff(messenger::msg_t(name, text));
	std::vector<uint8_t> buff_cpy = buff;

	const messenger::msg_t& message = messenger::parse_buff(buff_cpy);

	REQUIRE(message.name == name);
	REQUIRE(message.text == text);
}