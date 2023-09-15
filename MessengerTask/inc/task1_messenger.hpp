/**
 * @file   messenger.hpp
 * @author
 * @brief  Çàäàíèå1 - ðåàëèçàöèÿ ïðîòîêîëà îáìåíà ñîîáùåíèÿìè ìåæäó ïîëüçîâàòåëÿìè.
 *
 * @detail Äëÿ îáìåíà ñîîáùåíèÿìè ìåæäó ïîëüçîâàòåëÿìè èñïîëüçóþòñÿ ïàêåòû ñëåäóþùåãî âèäà:
 *
 *	+-MSG packet------------------------------------------------------------------------------------------>
 *	0        2 3          6 7           11 12     15 16
 *	+---------+------------+--------------+---------+----------------------+------------------------------+
 *	|  FLAG   | NAME_LEN   |    MSG_LEN   |  CRC4   |        NAME          |            MSG               |
 *	+---------+------------+--------------+---------+----------------------+------------------------------+
 *
 *	FLAG		- [3 bits]			- ôëàã ñîäåðæàùèé äâîè÷íîå çíà÷åíèå 101;
 *	NAME_LEN	- [4 bits]			- ñîäåðæèò êîëè÷åñòâî ñèìâîëîâ â ïîëå NAME (áåç ñèìâîëà îêîí÷àíèÿ ñòðîêè), âîçìîæíûå çíà÷åíèÿ: [1:15];
 *  MSG_LEN		- [5 bits]			- ñîäåðæèò êîëè÷åñòâî ñèìâîëîâ â ïîëå MSG (áåç ñèìâîëà îêîí÷àíèÿ ñòðîêè), âîçìîæíûå çíà÷åíèÿ: [1:31];
 *  CRC4		- [4 bits]			- ñîäåðæèò çíà÷åíèå CRC4 äëÿ ïîëåé: FLAG, NAME_LEN, MSG_LEN, NAME, MSG;
 *	NAME		- [NAME_LEN bytes]	- ñîäåðæèò èìÿ îòïðàâèòåëÿ (áåç ñèìâîëà îêîí÷àíèÿ ñòðîêè);
 *	MSG  		- [MSG_LEN bytes]	- ñîäåðæèò òåêñò ñîîáùåíèÿ (áåç ñèìâîëà îêîí÷àíèÿ ñòðîêè).
 *
 * Ïðè ýòîì:
 *		1) â ñëó÷àå åñëè òåêñò îòïðàâëÿåìîãî ñîîáùåíèÿ íå ìîæåò ïîìåñòèòüñÿ â 1 ïàêåò, ñîîáùåíèå íåîáõîäèìî óïàêîâàòü íåñêîëüêèìè ïàêåòàìè;
 *		2) â ñëó÷àå åñëè èìÿ îòïðàâèòåëÿ ñîîáùåíèÿ ïðåâûøàåò ìàêñèìàëüíûé äîïóñòèìûé ðàçìåð áðîñèòü èñêëþ÷åíèå std::length_error;
 *		3) èìÿ îòïðàâèòåëÿ è òåêñò ñîîáùåíèÿ íå ìîãóò áûòü ïóñòûìè - â ñëó÷àå íàðóøåíèÿ óñëîâèÿ áðîñèòü èñêëþ÷åíèå std::length_error;
 *		4) çíà÷åíèå CRC4 ðàññ÷èòûâàåòñÿ èñïîëüçóÿ êîä http://read.pudn.com/downloads169/sourcecode/math/779571/CRC4.C__.htm
 */
#ifndef TASK1_MESSENGER_HPP
#define TASK1_MESSENGER_HPP

#include <stdint.h>
#include <stdexcept>
#include <iterator>		// std::advance, std::distance
#include <cassert>
#include <vector>
#include <string>

namespace messenger
{

	/**
	 * Helper type to represent message: sender name, message text
	 */
	struct msg_t
	{
		msg_t(const std::string& nm, const std::string& txt)
			: name(nm)
			, text(txt)
		{}

		std::string name;	/**< message sender's name */
		std::string text;	/**< message text */
	};


	/**
	 * Prepare raw message buffer from specified message
	 *
	 * @note raw message buffer may consist from several (at least one) message packets
	 *
	 * @param msg message sender's name & message text
	 * @return buffer with prepared message packets
	 *
	 * @sample
	 *
	 * // if make_buff succeeded: buff contains required number of packets (for this case 1) to encode specified message
	 * std::vector<uint8_t> buff = messenger::make_buff( messenger::msg_t("Timur", "Hi") );
	 *
	 * // if parse_buff succeeded:
	 * //	msg.name should be "Timur";
	 * //	msg.text should be "Hi".
	 * messenger::msg_t msg = messenger::parse_buff(buff);
	*/
	std::vector<uint8_t> make_buff(const msg_t& msg);


	/**
	* Parse specified raw message buffer to get original message
	*
	* @param buff raw message buffer
	* @return parsed message
	*
	* @note In the process of decoding the buffer, it is necessary to verify the value of the fields:
	*	- FLAG;
	*	- CRC4.
	* If their value will be incorrect throw std::runtime_error
	*/
	msg_t parse_buff(std::vector<uint8_t>& buff);

}	// namespace messenger

#endif // !TASK1_MESSENGER_HPP
