#pragma once
#include <WS2tcpip.h>
#include <WinSock2.h>

#pragma comment(lib, "Ws2_32.lib")

enum NetPktKind
{
    LOG_IN = 0,
    LOG_IN_RP,
    LOG_IN_RC,
    SIGN_UP,
    SIGN_UP_R,
    MY_INFO,
    MY_INFO_R,
    CHANGE_ROLE,
    CHANGE_ROLE_R,
    USER_LIST,
    USER_LIST_R,
    SORT,
    FILTER_NAME,
    FILTER_OTHER,
    SF_R,
    SF_PAGE,
    GET_WORDS,
    GET_WORDS_R,
    INC_XP,
    WORD_CON,
    LOG_OUT,
};

struct MyPacket
{
    unsigned char kind;
    unsigned len;
    char *data;
    sockaddr_in from;
};