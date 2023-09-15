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

static void get_header(uint8_t high_byte, uint8_t low_byte, uint8_t& flag, uint8_t& namelen, uint8_t& textlen, uint8_t& crc4) {
	uint16_t header(high_byte);//uint16_t(high_byte) << 8 + low_byte);
	header <<= 8;
	header += low_byte;
	std::bitset<16> header_bits(header);
	//std::cout << header_bits << std::endl;
	crc4 = (header & 0x000f) >> (16 - FLAG_LEN - NAMELEN_LEN - TEXTLEN_LEN - CRC4_LEN);
	textlen = (header & 0x01f0) >> (16 - FLAG_LEN - NAMELEN_LEN - TEXTLEN_LEN);
	namelen = (header & 0x1e00) >> (16 - FLAG_LEN - NAMELEN_LEN);
	flag = (header & 0xe000) >> (16 - FLAG_LEN);
}

messenger::msg_t messenger::parse_buff(std::vector<uint8_t>& buff) {
	uint8_t flag;
	uint8_t namelen;
	uint8_t textlen;
	uint8_t crc4;
	size_t packet_n(0);

	uint8_t crc(0);
	uint8_t pos(0);

	for (int i = 0; i < buff.size(); i++) {
		//std::cout << buff[i];
	}
	//std::cout << std::endl;

	size_t buff_size = buff.size();
	if (buff_size < 2)
		throw std::runtime_error("error: invalid buffer");

	get_header(buff[pos + 0], buff[pos + 1], flag, namelen, textlen, crc4);
	messenger::msg_t msg("", "");
	for (int i = 0; i < namelen; i++)
		msg.name += buff[2 + i];
	//std::cout << "message name: " << msg.name << std::endl;
	while (buff.size() > pos) {
		//std::cout << (int)buff.size() << " " << (int)pos << std::endl;
		get_header(buff[pos + 0], buff[pos + 1], flag, namelen, textlen, crc4);

		if (flag != FLAG_VAL)
			throw std::runtime_error("error: invalid flag");

		buff[pos + 1] &= 0xf0;
		crc = CRC::Calculate(static_cast<void*>(&(buff[pos + 0])), HEADER_LEN / CHAR_BIT + namelen + textlen, CRC::CRC_4_ITU());
		//std::cout << "crc: " << (int)crc << std::endl;
		if (crc4 != crc)
			throw std::runtime_error("error: invalid crc");

		for (int i = 0; i < textlen; i++)
			msg.text += buff[pos + 2 + namelen + i];

		packet_n++;
		pos = packet_n * (MAX_MSG_LEN + namelen + HEADER_LEN / CHAR_BIT);
	}

	return msg;
}