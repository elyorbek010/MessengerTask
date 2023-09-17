// task1_messenger.cpp : Defines the entry point for the application.
//
#include <bitset>
#include <cassert>

#include <iostream>
#define CRCPP_INCLUDE_ESOTERIC_CRC_DEFINITIONS

#include "task1_messenger.hpp"
#include "CRC.h"

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

static void unpack_header(std::vector<uint8_t>::const_iterator header_iter, uint8_t& flag, uint8_t& namelen, uint8_t& textlen, uint8_t& crc)
{
	uint16_t header = (static_cast<unsigned short>(*header_iter) << 8) + (static_cast<unsigned short>(*(header_iter+1)));
	
	crc = header & N_BIT_MASK(CRC_LEN);
	header >>= CRC_LEN;

	textlen = header & N_BIT_MASK(TEXTLEN_LEN);
	header >>= TEXTLEN_LEN;

	namelen = header & N_BIT_MASK(NAMELEN_LEN);
	header >>= NAMELEN_LEN;

	flag = header & N_BIT_MASK(FLAG_LEN);
	header >>= FLAG_LEN;
}

static inline void clear_crc(std::vector<uint8_t>::iterator header_iter) 
{
	*(header_iter + 1) &= 0xf0; // clear the crc value of the header
}

messenger::msg_t messenger::parse_buff(std::vector<uint8_t>& buff)
{
	uint8_t flag(0);
	uint8_t namelen(0);
	uint8_t textlen(0);
	uint8_t header_crc(0);
	uint8_t calculated_crc(0);
	uint8_t max_packet_len(0);
	std::vector<uint8_t>::iterator buff_iter;
	messenger::msg_t msg("", "");

	unpack_header(buff.begin(), flag, namelen, textlen, header_crc);
	max_packet_len = HEADER_SIZE + namelen + MAX_MSG_LEN;

	// Read first packet and assign NAME to msg.name
	msg.name.assign(buff.begin() + HEADER_SIZE, buff.begin() + HEADER_SIZE + namelen); 

	// Read packets and append messages to msg.text
	for (size_t packet_n = 0; packet_n < buff.size() / max_packet_len + 1; packet_n++)
	{
		buff_iter = buff.begin() + packet_n * max_packet_len; // put iterator to the beginning of the n'th packet

		unpack_header(buff_iter, flag, namelen, textlen, header_crc);

		if (flag != FLAG_VAL)
			throw std::runtime_error("error: invalid flag");

		clear_crc(buff_iter);

		calculated_crc = CRC::Calculate(static_cast<void*>(&*buff_iter),
			HEADER_SIZE + namelen + textlen,
			CRC::CRC_4_ITU());

		if (header_crc != calculated_crc)
			throw std::runtime_error("error: invalid crc");

		buff_iter += HEADER_SIZE + namelen; // skip to text field

		msg.text.append(buff_iter, buff_iter + textlen); // append the text
	}

	return msg;
}