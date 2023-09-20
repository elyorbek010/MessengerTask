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

static std::vector<Text> text_splitter(std::string::const_iterator text_begin, std::string::const_iterator text_end, uint8_t split_length) {
	std::vector<Text> texts;

	std::string::const_iterator start = text_begin;
	std::string::const_iterator end = text_begin;

	while (end != text_end) {
		if (distance(start, end) == split_length) {
			texts.push_back({ start, end });
			start = end;
		}

		end++;
	}

	texts.push_back({ start, end });

	return texts;
}

class Header {
private:
	uint8_t flag;
	uint8_t namelen;
	uint8_t msglen;
	uint8_t crc4;

public:
	Header(uint8_t namelen, uint8_t msglen)
		: flag(FLAG_VAL)
		, namelen(namelen)
		, msglen(msglen)
		, crc4(0)
	{ }

	uint8_t size() {
		return HEADER_SIZE;
	}

	void set_crc(uint8_t crc4) {
		this->crc4 = crc4 & N_BIT_MASK(CRC_LEN);
	}
	
	void clear_crc(uint8_t crc4) {
		this->crc4 = 0;
	}

	uint16_t get_header() {
		uint16_t header(0);
		header <<= FLAG_LEN;
		header |= flag;

		header <<= NAMELEN_LEN;
		header |= namelen;

		header <<= TEXTLEN_LEN;
		header |= msglen;

		header <<= CRC_LEN;
		header |= crc4;

		return header;
	}

	uint8_t get_header_h() {
		uint16_t header = this->get_header();

		return static_cast<uint8_t>(header >> __CHAR_BIT__);
	}

	uint8_t get_header_l() {
		uint16_t header = this->get_header();

		return static_cast<uint8_t>(header & N_BIT_MASK(__CHAR_BIT__));
	}


};

class Payload {
private:
	std::string name;
	std::string message;

public:
	Payload(){}

	uint8_t size() {
		return name.size() + message.size();
	}

	// what's better, setting name, msg in constructor or through setters ? anyway they're set only once
	void set_name(std::string name) {
		if (name.empty()) throw std::length_error("error: name cannot be empty");
		if (name.size() > MAX_NAME_LEN) throw std::length_error("error: name is too long");

		this->name = name;
	}

	void set_message(std::string::const_iterator msg_begin, std::string::const_iterator msg_end) {
		if (distance(msg_begin, msg_end) > MAX_MSG_LEN) throw std::length_error("error: message is too long");
		if (msg_end == msg_begin) throw std::length_error("error: message cannot be empty");

		this->message.assign(msg_begin, msg_end);
	}

	std::string::iterator name_begin() {
		return name.begin();
	}

	std::string::iterator name_end() {
		return name.end();
	}

	std::string::iterator msg_begin() {
		return message.begin();
	}

	std::string::iterator msg_end() {
		return message.end();
	}

};

class Packet {
private:
	Header header;
	Payload payload;

public:
	Packet(std::string name, std::string::const_iterator msg_begin, std::string::const_iterator msg_end)
		: header(name.size(), distance(msg_begin, msg_end))
	{
		payload.set_name(name);
		payload.set_message(msg_begin, msg_end);
	}

	uint8_t size() {
		return header.size() + payload.size();
	}

	std::vector<uint8_t>get_packet() {
		std::vector<uint8_t> packet;

		packet.push_back(header.get_header_h());	
		packet.push_back(header.get_header_l());	

		packet.insert(packet.end(), payload.name_begin(), payload.name_end());
		packet.insert(packet.end(), payload.msg_begin(), payload.msg_end());

		uint8_t crc = CRC::Calculate(static_cast<void*>(&packet[0]),
			this->size(),
			CRC::CRC_4_ITU());

		header.set_crc(
			CRC::Calculate(
				static_cast<void*>(&packet[0]), // pointer to data
				this->size(),					// size of data
				CRC::CRC_4_ITU()				// crc formula
			)
		);

		packet[0] = header.get_header_h();
		packet[1] = header.get_header_l();

		return packet;
	}
};

std::vector<uint8_t> messenger::make_buff(const messenger::msg_t& msg)
{
	auto msgs_list = text_splitter(msg.text.cbegin(), msg.text.cend(), MAX_MSG_LEN); // split texts into chunks of length <= MAX_MSG_LEN

	std::vector<Packet> packets_list;
	std::vector<uint8_t> res_buff;
	std::vector<uint8_t> single_packet_buff;
	
	for (auto text : msgs_list) 
	{
		packets_list.push_back({msg.name, text.cbegin(), text.cend()}); // create single packets
	}

	for (auto single_packet : packets_list) {
		single_packet_buff = single_packet.get_packet();
		res_buff.insert(res_buff.end(), single_packet_buff.begin(), single_packet_buff.end());
	}

	return res_buff;
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