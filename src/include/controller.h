#pragma once

#include <WS2tcpip.h>
#include <WinSock2.h>

#pragma comment(lib, "Ws2_32.lib")

#include "my_packet.h"

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

// server
class ServerController
{
public:
	ServerController() = delete;
	ServerController(WordList *word_list, AccountSys *acc_sys) : word_list_(word_list), acc_sys_(acc_sys), preparer_(word_list, acc_sys) {}
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

	bool LogInR();
	bool SignUpR();
	bool MyInfoR();
	bool ChangeRoleR();
	bool UserListR();
	bool SortR();
	bool FilterR();
	bool SFTurnPageR();
	bool GetWordsR();
	bool GetACK();

	WordList *word_list_;
	AccountSys *acc_sys_;

	NetPktKind kind_;
	Preparer preparer_;
	sockaddr_in from_;
};