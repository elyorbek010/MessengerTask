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

class Header {
private:
	uint16_t header;
	uint8_t flag;
	uint8_t namelen;
	uint8_t msglen;
	uint8_t crc4;

	void update_header() {
		uint16_t header(0);

		header <<= FLAG_LEN;
		header |= flag;

		header <<= NAMELEN_LEN;
		header |= namelen;

		header <<= TEXTLEN_LEN;
		header |= msglen;

		header <<= CRC_LEN;
		header |= crc4;

		this->header = header;
	}

public:
	Header(uint8_t namelen, uint8_t msglen)
		: flag(FLAG_VAL)
		, namelen(namelen)
		, msglen(msglen)
		, crc4(0)
	{
		update_header();
	}

	Header(std::vector<uint8_t>::const_iterator header_iter)
	{
		uint16_t header = (static_cast<unsigned short>(*header_iter) << __CHAR_BIT__) + (static_cast<unsigned short>(*(header_iter + 1)));

		crc4 = header & N_BIT_MASK(CRC_LEN);
		header >>= CRC_LEN;

		msglen = header & N_BIT_MASK(TEXTLEN_LEN);
		header >>= TEXTLEN_LEN;

		namelen = header & N_BIT_MASK(NAMELEN_LEN);
		header >>= NAMELEN_LEN;

		flag = header & N_BIT_MASK(FLAG_LEN);
		header >>= FLAG_LEN;

		update_header();

		if (flag != FLAG_VAL) throw std::runtime_error("error: invalid flag");
	}

	uint8_t size() {
		return HEADER_SIZE;
	}

	uint8_t get_header_h() {
		return static_cast<uint8_t>(header >> __CHAR_BIT__);
	}

	uint8_t get_header_l() {
		return static_cast<uint8_t>(header & N_BIT_MASK(__CHAR_BIT__));
	}

	uint8_t get_namelen() {
		return namelen;
	}

	uint8_t get_msglen() {
		return msglen;
	}

	uint8_t get_crc4() {
		return crc4;
	}

	void set_crc(uint8_t crc4) {
		this->crc4 = crc4 & N_BIT_MASK(CRC_LEN);
		update_header();
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

	void set_name(std::string name) {
		if (name.empty()) throw std::length_error("error: name cannot be empty");
		if (name.size() > MAX_NAME_LEN) throw std::length_error("error: name is too long");

		this->name = name;
	}

	void set_name(std::vector<uint8_t>::const_iterator name_begin, std::vector<uint8_t>::const_iterator name_end) {
		name.assign(name_begin, name_end);
	}

	void set_message(std::string::const_iterator msg_begin, std::string::const_iterator msg_end) {
		if (distance(msg_begin, msg_end) > MAX_MSG_LEN) throw std::length_error("error: message is too long");
		if (msg_end == msg_begin) throw std::length_error("error: message cannot be empty");

		this->message.assign(msg_begin, msg_end);
	}

	void set_message(std::vector<uint8_t>::const_iterator msg_begin, std::vector<uint8_t>::const_iterator msg_end) {
		message.assign(msg_begin, msg_end);
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

	void set_buff_crc(std::vector<uint8_t>& buff) {
		buff[1] |= header.get_crc4();
	}

	void clear_buff_crc(std::vector<uint8_t>::iterator header_iter) {
		*(header_iter + 1) &= ~N_BIT_MASK(CRC_LEN);
	}

public:
	Packet(std::string name, std::string::const_iterator msg_begin, std::string::const_iterator msg_end)
		: header(name.size(), distance(msg_begin, msg_end))
	{

		payload.set_name(name);
		payload.set_message(msg_begin, msg_end);
	}

	Packet(std::vector<uint8_t>::iterator cbegin)
		: header(cbegin)
	{
		auto header_start = cbegin;
		auto name_start = header_start + header.size();
		auto msg_start = name_start + header.get_namelen();

		clear_buff_crc(header_start);

		payload.set_name(name_start, name_start + header.get_namelen());
		payload.set_message(msg_start, msg_start + header.get_msglen());

		uint8_t calculated_crc4 = CRC::Calculate(
			static_cast<void*>(&*header_start), // pointer to data
			this->size(),						// size of the data
			CRC::CRC_4_ITU()					// crc formula
		);

		if (calculated_crc4 != header.get_crc4()) throw std::runtime_error("error: invalid crc");
	}

	uint8_t size()
	{
		return header.size() + payload.size();
	}

	std::string get_name() {
		return std::string(payload.name_begin(), payload.name_end());
	}

	std::string get_message() {
		return std::string(payload.msg_begin(), payload.msg_end());
	}

	std::vector<uint8_t>bufferize() 
	{
		std::vector<uint8_t> packet;

		packet.push_back(header.get_header_h());	
		packet.push_back(header.get_header_l());	

		packet.insert(packet.end(), payload.name_begin(), payload.name_end());
		packet.insert(packet.end(), payload.msg_begin(), payload.msg_end());

		// update the crc value in the header(previously there was a CRC_PLACEHOLDER)
		header.set_crc(
			CRC::Calculate(
				static_cast<void*>(&packet[0]), // pointer to data
				this->size(),					// size of data
				CRC::CRC_4_ITU()				// crc formula
			)
		);

		// update the crc field of the heaader in the current packet
		set_buff_crc(packet);

		return packet;
	}
};

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

		end += std::min(split_length, static_cast<uint8_t>(text_end - start));
	}

	texts.push_back({ start, end }); // insert the last packet when end == text_end

	return texts;
}

std::vector<uint8_t> messenger::make_buff(const messenger::msg_t& msg)
{
	auto msgs_list = text_splitter(msg.text.cbegin(), msg.text.cend(), MAX_MSG_LEN);

	std::vector<uint8_t> res_buff;
	std::vector<uint8_t> single_packet_buff;

	for (auto text : msgs_list) 
	{
		Packet single_packet(msg.name, text.cbegin(), text.cend());								
		single_packet_buff = single_packet.bufferize();											
		res_buff.insert(res_buff.end(), single_packet_buff.cbegin(), single_packet_buff.cend());
	}

	return res_buff;
}

static std::vector<Packet> packet_splitter(std::vector<uint8_t>::iterator buff_begin,
	std::vector<uint8_t>::iterator buff_end)
{
	std::vector<Packet> packets;

	while (buff_begin != buff_end) {
		Packet packet(buff_begin);
		packets.push_back(packet);
		buff_begin += packet.size();
	}

	return packets;
}

messenger::msg_t messenger::parse_buff(std::vector<uint8_t>& buff)
{
	messenger::msg_t msg("", "");

	std::vector<Packet> packets_list = packet_splitter(buff.begin(), buff.end());

	// set name
	msg.name = packets_list[0].get_name();

	// set message
	for (auto packet : packets_list) {
		msg.text += packet.get_message();
	}

	return msg;
}