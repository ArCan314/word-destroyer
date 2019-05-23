#pragma once
#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

#include <mutex>

#include "my_packet.h"
#include "my_socket.h"
#include "log.h"





/*
 NAME					FLAG(1B)									DATA(VAR)																	CHECKSUM(4B)
@LOG_IN:				  0			LEN_NAME(1)			NAME(V)					LEN_PSWD(1)			PSWD(XOR)(V)						
LOG_IN_RP:				  1			IS_LOG_IN(1)		UID(4)					USER_TYPE(1)		LEVEL(4)			XP(8)				PASSED(4)
LOG_IN_RC:				  2			IS_LOG_IN(1)		UID(4)					USER_TYPE(1)		LEVEL(4)			WORD_CON(4)			
@SIGN_UP:				  3			LEN_NAME(1)			NAME(V)					LEN_PSWD(1)			PSWD(V)
SIGN_UP_R:				  4			IS_SIGN_UP(1)		UID(4)
@MY_INFO:				  5			UID(4)
MY_INFO_R:				  6			USER_TYPE(1)		LEVEL(4)				XP(8)					PASSED(4)
																				WORD_CON(4)
@CHANGE_ROLE:			  7			UID(4)
CHANGE_ROLE_ACK:		  8			
@USER_LIST:				  9			USER_TYPE(1)		USER_PER_PAGE(2)		PAGE(4)
USER_LIST_R:			  10		USER_TYPE(1)		USER_NUM(2)			[PLAYER_PACKETS/CON_PACKETS]
[PLAYER_PACKET]				  [		LEN_NAME(1)			NAME(V)					LEVEL(4)			XP(8)				PASSED(4)	]
[CON_PACKET]				  [		LEN_NAME(1)			NAME(V)					LEVEL(4)			WORD_CON(4)	   ]
@SORT:					  11		UID(4)				USER_TYPE(1)			USER_PER_PAGE(2)	SORT_TYPE(1)(NAME / LV / WORD / XP / PASSED)
@FILTER_NAME:			  12		UID(4)				USER_TYPE(1)			USER_PER_PAGE(2)	LEN_NAME(1)				NAME_VALUE(V)	
@FILTER_OTHER:			  13		UID(4)				USER_TYPE(1)			USER_PER_PAGE(2)	FILTER_TYPE(LV / WORD / XP / PASSED)					FILTER_VALUE(4 / 8)
SF_R:					  14		USER_TYPE(1)		USER_NUM(2)			[PLAYER_PACKETS/CON_PACKETS]
@SF_PAGE:				  15		UID(4)				USER_TYPE(1)			USER_PER_PAGE(2)		PAGE(4)
@GET_WORDS:				  16		PASSED(4)
GET_WORDS_R:			  17		TIME(2)				ROUND(1)				[LEN_WORD			WORD		DIFFICULTY]
@INC_XP:				  18		UID(4)				XP(8)
@WORD_CON:				  19		UID(4)				LEN_WORD(1)				WORD(V)	
@LOG_OUT:				  20		UID(4)
INC_XP_ACK				  21
WORD_CON_ACK			  22		IS_CON(1)
LOG_OUT_ACK				  23
LOG_IN_FAIL				  24
SIGN_UP_FAIL			  25
*/

class WordList;
class AccountSys;
class MySocket;

// server
class ServerController
{
public:
	ServerController() = delete;
	ServerController(WordList *word_list, AccountSys *acc_sys) : word_list_(word_list), acc_sys_(acc_sys), preparer_() {}

	ServerController(const ServerController &other) = delete;
	ServerController &operator=(const ServerController &rhs) = delete;

	MyPacket get_responce(const MyPacket &quest);

private:
	bool LogIn();
	bool SignUp();
	bool MyInfo();
	bool ChangeRole();
	bool UserList();
	bool Sort();
	bool Filter();
	bool SFTurnPage();
	bool GetWords();
	bool IncXP();
	bool WordCon();
	bool LogOut();

	WordList *word_list_;
	AccountSys *acc_sys_;

	NetPktKind kind_ = LOG_IN;
	sockaddr_in client_port_;
	Preparer preparer_;
};

// client
class ClientAccountSys;
class ClientWordList;
struct FilterPacket;
class ClientController
{
public:
	ClientController() = delete;
	ClientController(ClientWordList *word_list, ClientAccountSys *acc_sys) 
		: word_list_(word_list), acc_sys_(acc_sys), send_sock_(SEND_SOCKET), preparer_(), recv_sock_(RECV_SOCKET)
	{
		WSADATA wsaData;
		Log::WriteLog(std::string("ClientController") + " : init : get server addr");
		if (WSAStartup(MAKEWORD(2, 2), &wsaData))
		{
			Log::WriteLog(std::string("ClientController") + " : init : WSAStartup fail, WSAErrorCode: " + std::to_string(WSAGetLastError()));
			return;
		}
		addrinfo hint, *res = 0;

		ZeroMemory(&hint, sizeof(hint));
		hint.ai_family = AF_INET;
		hint.ai_protocol = IPPROTO_UDP;
		hint.ai_flags = AI_PASSIVE;
		hint.ai_socktype = SOCK_DGRAM;
		if (getaddrinfo("localhost", "9961", &hint, &res))
		{
			Log::WriteLog(std::string("ClientController") + " : init : getaddrinfo fail, WSAErrorCode: " + std::to_string(WSAGetLastError()));
			return;
		}

		sockaddr_in *temp = reinterpret_cast<sockaddr_in *>(res->ai_addr);
		server_addr_ = *temp;
		freeaddrinfo(res);
	}
	ClientController(const ClientController &other) = delete;
	ClientController &operator=(const ClientController &rhs) = delete;

	bool LogIn(const std::string &name, const std::string &pswd);
	bool SignUp(const std::string &name, const std::string &pswd);
	bool MyInfo(const unsigned uid);
	bool ChangeRole(const unsigned uid);
	bool UserList(const unsigned char utype, unsigned short user_per_page, unsigned page);
	bool Sort(unsigned uid, const unsigned char utype, unsigned short user_per_page, unsigned char sort_type);
	bool Filter(unsigned uid, const unsigned char utype, unsigned short user_per_page, const FilterPacket &filter_packet);
	bool SFTurnPage(unsigned uid, const unsigned char utype, unsigned short user_per_page, unsigned page);
	bool GetWord(unsigned passed);
	bool IncXp(unsigned uid, double exp);
	bool WordCon(unsigned uid, const std::string &word);
	bool LogOut(unsigned uid);

private:
	ClientWordList *word_list_;
	ClientAccountSys *acc_sys_;

	NetPktKind kind_ = LOG_IN;
	Preparer preparer_;
	sockaddr_in server_addr_ = sockaddr_in();

	MySocket send_sock_;
	MySocket recv_sock_;
};