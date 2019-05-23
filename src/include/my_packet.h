#pragma once
#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

#include <string>
#include <vector>

#include "user.h"
#include "player.h"
#include "contributor.h"

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
    CHANGE_ROLE_ACK,
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
    INC_XP_ACK,
    WORD_CON_ACK,
    LOG_OUT_ACK,
    LOG_IN_FAIL,
    SIGN_UP_FAIL
};

enum SortType
{
    ST_NAME = 0,
    ST_LV,
    ST_WORD,
    ST_XP,
    ST_PASSED,
};

enum FilterType
{
    FT_NAME = 0,
    FT_LV,
    FT_XP,
    FT_PASSED,
    FT_WORD,
};

struct MyPacket
{
    unsigned len;
    char *data;
    sockaddr_in from;
};

struct UserPacket
{
    unsigned char name_len;
    std::string name;
    unsigned level;
};

struct PlayerPacket
{
    UserPacket user;
    double exp;
    unsigned passed;
};

struct ContributorPacket
{
    UserPacket user;
    unsigned word_con;
};

struct WordPacket
{
    unsigned char word_len;
    std::string word;
    unsigned char difficulty;
};

class WordList;
class AccountSys;
struct Word;
class Player;
class User;
class Contributor;
class Preparer
{
public:
    Preparer() = default;

    void Decode(const MyPacket &my_packet);
    MyPacket Encode();

    void Clear();

    MyPacket raw_packet = MyPacket();

    unsigned char kind = 0;
    unsigned uid = 0;
	unsigned short client_port = 0;

    unsigned char name_len = 0;
    unsigned char pswd_len = 0;
    unsigned char word_len = 0;
    std::string name = "";
    std::string pswd = "";
    std::string word = "";

    unsigned char is_log_in = 0;
    unsigned char is_sign_up = 0;
    unsigned char is_con = 0;

    unsigned char user_type = 0;
    unsigned level = 0;

    double exp = 0;
    unsigned passed = 0;

    unsigned word_con = 0;

    unsigned short user_per_page = 0;
    unsigned short user_num = 0;
    unsigned page = 0;

    std::vector<PlayerPacket> player_vec;
    std::vector<ContributorPacket> con_vec;

    unsigned char sort_type = 0;
    unsigned char filter_type = 0;

    unsigned short time = 0;
    unsigned char round = 0;

    std::vector<WordPacket> word_vec;

    UserPacket GenUserPacket(const User &user);
    PlayerPacket GenPlayerPacket(const Player &player);
    ContributorPacket GenContributorPacket(const Contributor &contributor);
    WordPacket GenWordPacket(const Word &word);

private:
    std::string ReadStr(const char *buffer, int len, int &ptr);
    UserPacket ReadUserPacket(const char *buffer, int &ptr);
    PlayerPacket ReadPlayerPacket(const char *buffer, int &ptr);
    ContributorPacket ReadContributorPacket(const char *buffer, int &ptr);
    WordPacket ReadWordPacket(const char *buffer, int &ptr);

    void CopyToCSTR(const unsigned char s, char *buffer, int &ptr);
    void CopyToCSTR(const unsigned short s, char *buffer, int &ptr);
    void CopyToCSTR(const unsigned u, char *buffer, int &ptr);
    void CopyToCSTR(const double d, char *buffer, int &ptr);
    void CopyToCSTR(const UserPacket &d, char *buffer, int &ptr);
    void CopyToCSTR(const PlayerPacket &d, char *buffer, int &ptr);
    void CopyToCSTR(const ContributorPacket &d, char *buffer, int &ptr);
    void CopyToCSTR(const WordPacket &d, char *buffer, int &ptr);
    void CopyToCSTR(const std::string &str, char *buffer, int &ptr);
};