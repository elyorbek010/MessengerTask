#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <stdexcept>
#include <vector>
#include <bitset>
#include <iostream>

#include "task1_messenger.hpp"

#define CRCPP_INCLUDE_ESOTERIC_CRC_DEFINITIONS
#include "CRC.h"

#define FLAG_LEN (3)		// in bits
#define FLAG_VAL (0b101)

#define NAMELEN_LEN (4)		// in bits
#define MAX_NAME_LEN (15)	// in bytes

#define TEXTLEN_LEN (5)		// in bits
#define MAX_MSG_LEN (31)	// in bytes

#define CRC_LEN (4)			// in bits
#define CRC_MASK (0b1111)
#define CRC_PLACEHOLDER (0b0000)

#define HEADER_SIZE (2)		// in bytes

#define N_BIT_MASK(num) (0xffff >> (16 - num))

TEST_CASE("MakeBuff_NameLen0", "MakeBuff") 
{
	std::string name("");
	std::string text("Hi");

	bool caught_error = false;

	try
	{
		const std::vector<uint8_t>& buff = messenger::make_buff(messenger::msg_t(name, text));
	}
	catch (const std::length_error& error) 
	{
		caught_error = true;
	}

	REQUIRE(caught_error == true);
}

TEST_CASE("MakeBuff_NameLen1", "MakeBuff") 
{
	std::string name("E");
	std::string text("Hi");

	std::vector<uint8_t> buff1{ 0b10100010, 0b00100000 };// insert header
	buff1.insert(buff1.end(), name.begin(), name.end()); // insert name
	buff1.insert(buff1.end(), text.begin(), text.end()); // insert text

	uint8_t crc = CRC::Calculate(static_cast<void*>(&buff1[0]), // calculate crc value
		HEADER_SIZE + name.size() + text.size(),
		CRC::CRC_4_ITU());

	buff1[1] |= crc; // set crc value

	const std::vector<uint8_t>& buff2 = messenger::make_buff(messenger::msg_t(name, text)); // get buffer from make_buff

	REQUIRE(buff1 == buff2);
}

TEST_CASE("MakeBuff_NameLen15", "MakeBuff") 
{
	std::string name("ElyorbekElyorbe");
	std::string text("Hi");

	std::vector<uint8_t> buff1{ 0b10111110, 0b00100000 };// insert header
	buff1.insert(buff1.end(), name.begin(), name.end()); // insert name
	buff1.insert(buff1.end(), text.begin(), text.end()); // insert text

	uint8_t crc = CRC::Calculate(static_cast<void*>(&buff1[0]), // calculate crc value
		HEADER_SIZE + name.size() + text.size(),
		CRC::CRC_4_ITU());

	buff1[1] |= crc; // set crc value

	const std::vector<uint8_t>& buff2 = messenger::make_buff(messenger::msg_t(name, text)); // get buffer from make_buff

	REQUIRE(buff1 == buff2);
}

TEST_CASE("MakeBuff_NameLen16", "MakeBuff") 
{
	std::string name("ElyorbekElyorbek");
	std::string text("Hi");

	bool caught_error = false;

	try
	{
		const std::vector<uint8_t>& buff = messenger::make_buff(messenger::msg_t(name, text));
	}
	catch (const std::length_error& error) 
	{
		caught_error = true;
	}

	REQUIRE(caught_error == true);
}

TEST_CASE("MakeBuff_MsgLen0", "MakeBuff") 
{
	std::string name("Elyorbek");
	std::string text("");

	bool caught_error = false;

	try
	{
		const std::vector<uint8_t>& buff = messenger::make_buff(messenger::msg_t(name, text));
	}
	catch (const std::length_error& error) 
	{
		caught_error = true;
	}

	REQUIRE(caught_error == true);
}

TEST_CASE("MakeBuff_MsgLen1", "MakeBuff") 
{
	std::string name("Elyorbek");
	std::string text("H");

	std::vector<uint8_t> buff1{ 0b10110000, 0b00010000 };// insert header
	buff1.insert(buff1.end(), name.begin(), name.end()); // insert name
	buff1.insert(buff1.end(), text.begin(), text.end()); // insert text

	uint8_t crc = CRC::Calculate(static_cast<void*>(&buff1[0]), // calculate crc value
		HEADER_SIZE + name.size() + text.size(),
		CRC::CRC_4_ITU());

	buff1[1] |= crc; // set crc value

	const std::vector<uint8_t>& buff2 = messenger::make_buff(messenger::msg_t(name, text)); // get buffer from make_buff

	REQUIRE(buff1 == buff2);
}

TEST_CASE("MakeBuff_MsgLen31", "MakeBuff") 
{
	std::string name("Elyorbek");
	std::string text("this message contains 31 chars ");

	std::vector<uint8_t> buff1{ 0b10110001, 0b11110000 };// insert header
	buff1.insert(buff1.end(), name.begin(), name.end()); // insert name
	buff1.insert(buff1.end(), text.begin(), text.end()); // insert text

	uint8_t crc = CRC::Calculate(static_cast<void*>(&buff1[0]), // calculate crc value
		HEADER_SIZE + name.size() + text.size(),
		CRC::CRC_4_ITU());

	buff1[1] |= crc; // set crc value

	const std::vector<uint8_t>& buff2 = messenger::make_buff(messenger::msg_t(name, text)); // get buffer from make_buff

	REQUIRE(buff1 == buff2);
}

TEST_CASE("MakeBuff_1PacketMaxNameMaxMsg", "MakeBuff") 
{
	std::string name("ElyorbekElyorbe");
	std::string text("this message contains 31 chars ");

	std::vector<uint8_t> buff1{ 0b10111111, 0b11110000 };// insert header
	buff1.insert(buff1.end(), name.begin(), name.end()); // insert name
	buff1.insert(buff1.end(), text.begin(), text.end()); // insert text

	uint8_t crc = CRC::Calculate(static_cast<void*>(&buff1[0]), // calculate crc value
		HEADER_SIZE + name.size() + text.size(),
		CRC::CRC_4_ITU());

	buff1[1] |= crc; // set crc value

	const std::vector<uint8_t>& buff2 = messenger::make_buff(messenger::msg_t(name, text)); // get buffer from make_buff

	REQUIRE(buff1 == buff2);
}

TEST_CASE("MakeBuff_MsgLen32", "MakeBuff") 
{
	std::string name("Elyorbek");
	std::string text("this message contains 32 chars  ");

	uint8_t header1_h = 0b10110001;
	uint8_t header1_l = 0b11110000;

	uint8_t header2_h = 0b10110000;
	uint8_t header2_l = 0b00010000;

	std::vector<uint8_t> buff1;
	buff1.push_back(header1_h);	// insert header
	buff1.push_back(header1_l);
	buff1.insert(buff1.end(), name.begin(), name.end()); // insert name
	buff1.insert(buff1.end(), text.begin(), text.begin() + MAX_MSG_LEN); // insert text

	buff1.push_back(header2_h);	// insert header
	buff1.push_back(header2_l);
	buff1.insert(buff1.end(), name.begin(), name.end()); // insert name
	buff1.insert(buff1.end(), text.begin() + MAX_MSG_LEN, text.end()); // insert text

	uint8_t crc = CRC::Calculate(static_cast<void*>(&buff1[0]), // calculate crc value
		HEADER_SIZE + name.size() + MAX_MSG_LEN,
		CRC::CRC_4_ITU());

	buff1[1] |= crc; // set crc value

	crc = CRC::Calculate(static_cast<void*>(&buff1[HEADER_SIZE + name.size() + MAX_MSG_LEN]), // calculate crc value
		HEADER_SIZE + name.size() + 1,
		CRC::CRC_4_ITU());

	buff1[HEADER_SIZE + name.size() + MAX_MSG_LEN + 1] |= crc; // set crc value

	const std::vector<uint8_t>& buff2 = messenger::make_buff(messenger::msg_t(name, text)); // get buffer from make_buff

	REQUIRE(buff1 == buff2);
}

TEST_CASE("MakeBuff_MsgLen62", "MakeBuff") 
{
	std::string name("Elyorbek");
	std::string text("this message contains 62 chars,this message contains 62 chars ");

	uint8_t header_h = 0b10110001;
	uint8_t header_l = 0b11110000;

	std::vector<uint8_t> buff1;
	buff1.push_back(header_h);	// insert header
	buff1.push_back(header_l);
	buff1.insert(buff1.end(), name.begin(), name.end()); // insert name
	buff1.insert(buff1.end(), text.begin(), text.begin() + MAX_MSG_LEN); // insert text

	buff1.push_back(header_h);	// insert header
	buff1.push_back(header_l);
	buff1.insert(buff1.end(), name.begin(), name.end()); // insert name
	buff1.insert(buff1.end(), text.begin() + MAX_MSG_LEN, text.end()); // insert text

	uint8_t crc = CRC::Calculate(static_cast<void*>(&buff1[0]), // calculate crc value
		HEADER_SIZE + name.size() + MAX_MSG_LEN,
		CRC::CRC_4_ITU());

	buff1[1] |= crc; // set crc value

	crc = CRC::Calculate(static_cast<void*>(&buff1[HEADER_SIZE + name.size() + MAX_MSG_LEN]), // calculate crc value
		HEADER_SIZE + name.size() + MAX_MSG_LEN,
		CRC::CRC_4_ITU());

	buff1[HEADER_SIZE + name.size() + MAX_MSG_LEN + 1] |= crc; // set crc value

	const std::vector<uint8_t>& buff2 = messenger::make_buff(messenger::msg_t(name, text)); // get buffer from make_buff
	
	REQUIRE(buff1 == buff2);
}

TEST_CASE("ParseBuff_WrongFlag", "ParseBuff") 
{
	std::string name("Elyorbek");
	std::string text("Hi");

	std::vector<uint8_t> buff = messenger::make_buff(messenger::msg_t(name, text));

	buff[0] &= 0x1f; // clear flag field

	bool caught_error = false;

	try 
	{
		const messenger::msg_t& text = messenger::parse_buff(buff);
	}
	catch (const std::runtime_error& error) 
	{
		caught_error = true;
	}

	REQUIRE(caught_error == true);
}

TEST_CASE("ParseBuff_WrongCRC", "ParseBuff") 
{
	std::string name("Elyorbek");
	std::string text("Hi");

	const std::vector<uint8_t>& buff = messenger::make_buff(messenger::msg_t(name, text));
	std::vector<uint8_t> buff_cpy = buff;

	buff_cpy[1] &= 0xf0; // clear crc field

	bool caught_error = false;

	try {
		const messenger::msg_t& message = messenger::parse_buff(buff_cpy);
	}
	catch (const std::runtime_error& error) 
	{
		caught_error = true;
	}

	REQUIRE(caught_error == true);
}

TEST_CASE("ParseBuff_NameLen1", "ParseBuff") 
{
	std::string name("E");
	std::string text("Hi");
	
	const std::vector<uint8_t>& buff = messenger::make_buff(messenger::msg_t(name, text));
	std::vector<uint8_t> buff_cpy = buff;
	
	const messenger::msg_t& message = messenger::parse_buff(buff_cpy);
	
	REQUIRE(message.name == name);
	REQUIRE(message.text == text);
}

TEST_CASE("ParseBuff_NameLen15", "ParseBuff") 
{
	std::string name("ElyorbekElyorbe");
	std::string text("Hi");

	const std::vector<uint8_t>& buff = messenger::make_buff(messenger::msg_t(name, text));
	std::vector<uint8_t> buff_cpy = buff;

	const messenger::msg_t& message = messenger::parse_buff(buff_cpy);

	REQUIRE(message.name == name);
	REQUIRE(message.text == text);
}

TEST_CASE("ParseBuff_MsgLen1", "ParseBuff") 
{
	std::string name("Elyorbek");
	std::string text("H");

	const std::vector<uint8_t>& buff = messenger::make_buff(messenger::msg_t(name, text));
	std::vector<uint8_t> buff_cpy = buff;

	const messenger::msg_t& message = messenger::parse_buff(buff_cpy);

	REQUIRE(message.name == name);
	REQUIRE(message.text == text);
}

TEST_CASE("ParseBuff_MsgLen31", "ParseBuff") 
{
	
	std::string name("Elyorbek");
	std::string text("this message contains 31 chars ");

	const std::vector<uint8_t>& buff = messenger::make_buff(messenger::msg_t(name, text));
	std::vector<uint8_t> buff_cpy = buff;

	const messenger::msg_t& message = messenger::parse_buff(buff_cpy);

	REQUIRE(message.name == name);
	REQUIRE(message.text == text);
}

TEST_CASE("ParseBuff_1PacketMaxNameMaxMsg", "ParseBuff") 
{
	std::string name("ElyorbekElyorbe");
	std::string text("this message contains 31 chars ");

	const std::vector<uint8_t>& buff = messenger::make_buff(messenger::msg_t(name, text));
	std::vector<uint8_t> buff_cpy = buff;

	const messenger::msg_t& message = messenger::parse_buff(buff_cpy);

	REQUIRE(message.name == name);
	REQUIRE(message.text == text);
}

TEST_CASE("ParseBuff_MsgLen32", "ParseBuff") 
{
	std::string name("Elyorbek");
	std::string text("this message contains 32 chars  ");

	const std::vector<uint8_t>& buff = messenger::make_buff(messenger::msg_t(name, text));
	std::vector<uint8_t> buff_cpy = buff;

	const messenger::msg_t& message = messenger::parse_buff(buff_cpy);

	REQUIRE(message.name == name);
	REQUIRE(message.text == text);
}

TEST_CASE("ParseBuff_MsgLen62", "ParseBuff") 
{
	std::string name("Elyorbek");
	std::string text("this message contains 62 chars,this message contains 62 chars ");

	const std::vector<uint8_t>& buff = messenger::make_buff(messenger::msg_t(name, text));
	std::vector<uint8_t> buff_cpy = buff;

	const messenger::msg_t& message = messenger::parse_buff(buff_cpy);

	REQUIRE(message.name == name);
	REQUIRE(message.text == text);
}