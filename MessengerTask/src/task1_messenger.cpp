// task1_messenger.cpp : Defines the entry point for the application.
//
#include <cassert>
#include <iostream>
#include <algorithm>

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

class Text {
private:
	std::string::const_iterator begin_iter;
	std::string::const_iterator end_iter;

public:
	Text(std::string::const_iterator begin, std::string::const_iterator end)
		: begin_iter(begin)
		, end_iter(end)
	{ }

	std::string::const_iterator cbegin() const {
		return begin_iter;
	}

	std::string::const_iterator cend() const {
		return end_iter;
	}
};

std::vector<Text> text_splitter(std::string::const_iterator text_begin, std::string::const_iterator text_end, uint8_t split_length) {
	assert(text_begin != text_end);
	std::vector<Text> text_splits;

	std::string::const_iterator begin = text_begin, end = text_begin;
	while (end != text_end) {
		if (end - begin == split_length) {
			text_splits.push_back({ begin, end });
			begin = end;
		}

		end++;
	}

	text_splits.push_back({ begin, end });

	return text_splits;
}

class Packet {
public:
	uint8_t flag;
	uint8_t namelen;
	uint8_t msglen;
	uint8_t crc4;

	std::string name;
	std::string message;
	
public:

	Packet(std::string name, std::string::const_iterator msg_begin, std::string::const_iterator msg_end) {
		if (name.empty()) throw std::length_error("error: name cannot be empty");
		if (name.size() > MAX_NAME_LEN) throw std::length_error("error: name is too long");
		if (msg_end - msg_begin > MAX_MSG_LEN) throw std::length_error("error: message is too long");
		if (msg_end == msg_begin) throw std::length_error("error: message cannot be empty");

		flag = FLAG_VAL;
		this->name = name;
		namelen = name.size();
		message.assign(msg_begin, msg_end);
		msglen = message.size();
		crc4 = CRC_PLACEHOLDER;
	}
};

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
	if (msg.name.empty()) throw std::length_error("error: name is empty");
	if (msg.name.size() > MAX_NAME_LEN) throw std::length_error("error: name is too long");
	if (msg.text.empty()) throw std::length_error("error: message is empty");

	auto msgs_list = text_splitter(msg.text.cbegin(), msg.text.cend(), MAX_MSG_LEN);

	std::vector<Packet> packets_list;
	std::vector<uint8_t> buff;

	for (auto text : msgs_list) 
	{
		packets_list.push_back({msg.name, text.cbegin(), text.cend()});
	}

	for (auto packet : packets_list) {
		uint16_t header = pack_header(packet.flag, packet.namelen, packet.msglen, packet.crc4);

		buff.push_back(static_cast<uint8_t>(header >> 8));			// higher byte of header
		buff.push_back(static_cast<uint8_t>(header & 0x00ff));		// lower byte of header

		buff.insert(buff.end(), packet.name.begin(), packet.name.end());
		buff.insert(buff.end(), packet.message.begin(), packet.message.end());

		uint8_t crc = CRC::Calculate(static_cast<void*>(&buff[buff.size() - (HEADER_SIZE + packet.namelen + packet.msglen)]),
			HEADER_SIZE + packet.namelen + packet.msglen,
			CRC::CRC_4_ITU());
			
		buff[buff.size() - (HEADER_SIZE + packet.namelen + packet.msglen) + 1] |= crc;  // set the crc value
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

static inline void clear_crc(std::vector<uint8_t>::iterator buff_header) 
{
	*(buff_header + 1) &= 0xf0; // clear the crc value of the header
}

messenger::msg_t messenger::parse_buff(std::vector<uint8_t>& buff)
{
	uint8_t flag(0);
	uint8_t namelen(0);
	uint8_t textlen(0);
	uint8_t header_crc(0);
	uint8_t calculated_crc(0);
	uint8_t max_packet_len(0);
	std::vector<uint8_t>::iterator buff_iter = buff.begin();
	messenger::msg_t msg("", "");

	unpack_header(buff.begin(), flag, namelen, textlen, header_crc);
	max_packet_len = HEADER_SIZE + namelen + MAX_MSG_LEN;

	// Read first packet and assign NAME to msg.name
	msg.name.assign(buff.begin() + HEADER_SIZE, buff.begin() + HEADER_SIZE + namelen); 

	// Read packets and append messages to msg.text
	for (size_t packet_n = 0; packet_n < buff.size() / max_packet_len + (buff.size() % max_packet_len != 0); packet_n++)
	{
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

		buff_iter += textlen;
	}

	return msg;
}