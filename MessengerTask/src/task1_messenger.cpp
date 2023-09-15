// task1_messenger.cpp : Defines the entry point for the application.
//
#include <bitset>
#include <cassert>

#include <iostream>
#define CRCPP_INCLUDE_ESOTERIC_CRC_DEFINITIONS

#include "../inc/task1_messenger.hpp"
#include "../inc/CRC.h"

#define FLAG_LEN (3)		// in bits
#define FLAG_VAL (0b101)

#define NAMELEN_LEN (4)		// in bits
#define MAX_NAME_LEN (15)	// in bytes

#define TEXTLEN_LEN (5)		// in bits
#define MAX_MSG_LEN (31)	// in bytes

#define CRC_LEN (4)		// in bits
#define CRC_MASK (0b1111)
#define CRC_PLACEHOLDER (0b0000)

#define HEADER_SIZE (2)		// in bytes

#define N_BIT_MASK(num) (0xffff >> (16 - num))

static uint16_t pack_header(uint8_t flag, uint8_t namelen, uint8_t textlen, uint8_t crc4) 
{
	assert(flag == FLAG_VAL);
	assert(namelen > 0 && namelen <= MAX_NAME_LEN);
	assert(textlen > 0 && textlen <= MAX_MSG_LEN);
	assert(crc4 == 0);

	uint16_t header(0);
	header <<= FLAG_LEN;
	header |= flag;

	header <<= NAMELEN_LEN;
	header |= namelen;

	header <<= TEXTLEN_LEN;
	header |= textlen;

	header <<= CRC_LEN;
	header |= crc4;

	return header;
}

std::vector<uint8_t> messenger::make_buff(const messenger::msg_t& msg) 
{
	std::string::size_type namelen = msg.name.length();
	std::string::size_type textlen = msg.text.length();

	if (namelen == 0) 
	{
		throw std::length_error("error: name is empty");
	}
	else if (textlen == 0) 
	{
		throw std::length_error("error: text is empty");
	}
	else if (namelen > 15) 
	{
		throw std::length_error("error: name is too long");
	}

	std::vector<uint8_t> buff;
	std::string::size_type cur_textlen(0);
	uint16_t header(0);
	uint8_t packet_n(0);
	uint8_t crc(0);

	while (textlen > 0) 
	{
		cur_textlen = textlen > MAX_MSG_LEN ? MAX_MSG_LEN : textlen;
		textlen -= cur_textlen;

		header = pack_header(FLAG_VAL, namelen, cur_textlen, CRC_PLACEHOLDER);

		buff.push_back(static_cast<uint8_t>(header >> 8));		// higher byte of header
		buff.push_back(static_cast<uint8_t>(header & 0x00ff));	// lower byte of header

		for (size_t i = 0; i < namelen; i++)
			buff.push_back(msg.name[i]);

		for (size_t i = 0; i < cur_textlen; i++)
			buff.push_back(msg.text[packet_n * MAX_MSG_LEN + i]);

		crc = CRC::Calculate(static_cast<void*>(&buff[packet_n * (HEADER_SIZE + namelen + MAX_MSG_LEN)]),
							HEADER_SIZE + namelen + cur_textlen, 
							CRC::CRC_4_ITU());
		
		buff[packet_n * (MAX_MSG_LEN + namelen + HEADER_SIZE) + 1] &= 0xf0; // clear the crc field
		buff[packet_n * (MAX_MSG_LEN + namelen + HEADER_SIZE) + 1] |= crc;  // set the crc value

		packet_n++;
	}

	return buff;
}

static void unpack_header(uint8_t high_byte, uint8_t low_byte, uint8_t& flag, uint8_t& namelen, uint8_t& textlen, uint8_t& crc) 
{
	uint16_t header = (static_cast<unsigned short>(high_byte) << 8) + (static_cast<unsigned short>(low_byte));

	crc = header & N_BIT_MASK(CRC_LEN);
	header >>= CRC_LEN;

	textlen = header & N_BIT_MASK(TEXTLEN_LEN);
	header >>= TEXTLEN_LEN;

	namelen = header & N_BIT_MASK(NAMELEN_LEN);
	header >>= NAMELEN_LEN;

	flag = header & N_BIT_MASK(FLAG_LEN);
	header >>= FLAG_LEN;
}

messenger::msg_t messenger::parse_buff(std::vector<uint8_t>& buff) 
{
	uint8_t flag(0);
	uint8_t namelen(0);
	uint8_t textlen(0);
	uint8_t header_crc(0);
	uint8_t calculated_crc(0);
	uint8_t buff_idx(0);

	size_t buff_size = buff.size();
	if(buff_size < HEADER_SIZE)
		throw std::runtime_error("error: invalid buffer");

	unpack_header(buff[0], buff[1], flag, namelen, textlen, header_crc);

	if (namelen == 0)
	{
		throw std::length_error("error: name is empty");
	}
	else if (textlen == 0) 
	{
		throw std::length_error("error: text is empty");
	}
	
	messenger::msg_t msg("", "");

	for (size_t i = 0; i < namelen; i++)
		msg.name += buff[HEADER_SIZE + i];

	while (buff.size() > buff_idx) 
	{
		unpack_header(buff[buff_idx], buff[buff_idx + 1], flag, namelen, textlen, header_crc);

		if (flag != FLAG_VAL)
			throw std::runtime_error("error: invalid flag");

		buff[buff_idx + 1] &= 0xf0; // clear the crc value in the buffer header

		calculated_crc = CRC::Calculate(static_cast<void*>(&(buff[buff_idx])), 
										HEADER_SIZE + namelen + textlen, 
										CRC::CRC_4_ITU());

		if (header_crc != calculated_crc)
			throw std::runtime_error("error: invalid crc");

		for (size_t i = 0; i < textlen; i++)
			msg.text += buff[buff_idx + HEADER_SIZE + namelen + i];

		buff_idx += MAX_MSG_LEN + namelen + HEADER_SIZE;
	}

	return msg;
}