// task1_messenger.cpp : Defines the entry point for the application.
//
#include "task1_messenger.hpp"
#include <netinet/in.h>
#include <bitset>
#include <cassert>
#include <iostream>
#define CRCPP_INCLUDE_ESOTERIC_CRC_DEFINITIONS

#include "CRC.h"

#define FLAG_LEN (3)		// in bits
#define FLAG_VAL (0b101)

#define NAMELEN_LEN (4)		// in bits
#define MAX_NAME_LEN (15)	// in bytes

#define TEXTLEN_LEN (5)		// in bits
#define MAX_MSG_LEN (31)	// in bytes

#define CRC4_LEN (4)		// in bits
#define CRC4_MASK (0b1111)
#define CRC4_PLACEHOLDER (0b0000)

#define HEADER_LEN (16)		// in bits

static uint16_t set_header(uint8_t flag, uint8_t namelen, uint8_t textlen, uint8_t crc4) {
	assert(flag == 0b101);
	assert(namelen > 0 && namelen < 16);
	assert(textlen > 0 && textlen < 32);
	assert(crc4 == 0);

	uint16_t header(0);
	header <<= FLAG_LEN;
	header |= flag;

	header <<= NAMELEN_LEN;
	header |= namelen;

	header <<= TEXTLEN_LEN;
	header |= textlen;

	header <<= CRC4_LEN;
	header |= crc4;

	return header;
}

std::vector<uint8_t> messenger::make_buff(const messenger::msg_t& msg) {
	std::string::size_type namelen = msg.name.length();
	std::string::size_type textlen = msg.text.length();

	if (namelen == 0) {
		throw std::length_error("error: name is empty");
	}
	else if (textlen == 0) {
		throw std::length_error("error: text is empty");
	}
	else if (namelen > 15) {
		throw std::length_error("error: name is too long");
	}

	std::vector<uint8_t> buff;
	std::string::size_type cur_textlen(0);
	uint16_t header(0);
	size_t pos(0);
	size_t packet_n(0);
	uint8_t crc(0);

	while (textlen > 0) {
		cur_textlen = textlen > MAX_MSG_LEN ? MAX_MSG_LEN : textlen;
		textlen -= cur_textlen;

		header = set_header(FLAG_VAL, namelen, cur_textlen, CRC4_PLACEHOLDER);

		buff.push_back(static_cast<uint8_t>(header >> 8));		// higher byte of header
		buff.push_back(static_cast<uint8_t>(header & 0x00ff));	// lower byte of header

		for (size_t i = 0; i < namelen; i++)
			buff.push_back(msg.name[i]);

		for (int i = 0; i < cur_textlen; i++)
			buff.push_back(msg.text[packet_n * MAX_MSG_LEN + i]);

		crc = CRC::Calculate(static_cast<void*>(&buff[packet_n * (MAX_MSG_LEN + namelen + 2)]), HEADER_LEN / CHAR_BIT + namelen + cur_textlen, CRC::CRC_4_ITU());
		
		buff[packet_n * (MAX_MSG_LEN + namelen + 2) + 1] &= 0xf0; // clear crc4 field
		buff[packet_n * (MAX_MSG_LEN + namelen + 2) + 1] |= crc & CRC4_MASK;  // set new crc4 value

		packet_n++;

	}

	return buff;
}