#pragma once
#include <string>
#include <vector>

#include <WS2tcpip.h>
#include <WinSock2.h>

#pragma comment(lib, "Ws2_32.lib")

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
    FT_WORD,
    FT_XP,
    FT_PASSED,
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
    Preparer() = delete;
    Preparer(WordList *word_list, AccountSys *acc_sys) : word_list_(word_list), acc_sys_(acc_sys) {}

    void Decode(const MyPacket &my_packet);
    MyPacket Encode();

    void Clear();

    MyPacket raw_packet;

    unsigned char kind;
    unsigned uid;

    unsigned char name_len;
    unsigned char pswd_len;
    unsigned char word_len;
    std::string name;
    std::string pswd;
    std::string word;

    unsigned char is_log_in;
    unsigned char is_sign_up;
    unsigned char is_con;

    unsigned char user_type;
    unsigned level;

    double exp;
    unsigned passed;

    unsigned word_con;

    unsigned short user_per_page;
    unsigned short user_num;
    unsigned page;

    std::vector<PlayerPacket> player_vec;
    std::vector<ContributorPacket> con_vec;

    unsigned char sort_type;
    unsigned char filter_type;

    unsigned short time;
    unsigned char round;

    std::vector<WordPacket> word_vec;

    UserPacket GenUserPacket(const User &user);
    PlayerPacket GenPlayerPacket(const Player &player);
    ContributorPacket GenContributorPacket(const Contributor &contributor);
    WordPacket GenWordPacket(const Word &word);

private:
    WordList *word_list_;
    AccountSys *acc_sys_;

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