#include <cstring>
#include <string>
#include <utility>

#include "my_packet.h"
#include "crc32.h"
#include "user.h"
#include "player.h"
#include "contributor.h"
#include "word_list.h"

void Preparer::Clear()
{
    kind = 0;
    uid = 0;
    name_len = 0;
	pswd_len = 0;
    name.clear();
    pswd.clear();
    word.clear();
	client_port = 0;

    is_log_in = 0;
    is_sign_up = 0;
    is_con = 0;

    user_type = 0;
    level = 0;
    exp = 0;
    passed = 0;
    word_con = 0;
    user_per_page = 0;
    user_num = 0;

    page = 0;

    player_vec.clear();
    con_vec.clear();

    sort_type = 0;
    filter_type = 0;
    time = 0;
    round = 0;

    word_vec.clear();
}

// 负责处理 len 及 data, from 由 controller保存
void Preparer::Decode(const MyPacket &my_packet)
{
    Clear();
    int ptr = 0;
    kind = my_packet.data[ptr++];

	client_port = *(unsigned *)(my_packet.data + ptr);
	ptr += sizeof(client_port);

    switch (kind)
    {
    case LOG_IN:
    case SIGN_UP:
        name_len = my_packet.data[ptr++];

        name = ReadStr(my_packet.data, name_len, ptr);

        pswd_len = my_packet.data[ptr++];

        pswd = ReadStr(my_packet.data, pswd_len, ptr);
        break;
    case LOG_IN_RP:
        is_log_in = my_packet.data[ptr++];

        uid = *(unsigned *)(my_packet.data + ptr);
        ptr += sizeof(uid);

        user_type = my_packet.data[ptr++];

        level = *(unsigned *)(my_packet.data + ptr);
        ptr += sizeof(level);

        exp = *(double *)(my_packet.data + ptr);
        ptr += sizeof(exp);

        passed = *(unsigned *)(my_packet.data + ptr);
        ptr += sizeof(passed);
        break;
    case LOG_IN_RC:
        is_log_in = my_packet.data[ptr++];

        uid = *(unsigned *)(my_packet.data + ptr);
        ptr += sizeof(uid);

        user_type = my_packet.data[ptr++];

        level = *(unsigned *)(my_packet.data + ptr);
        ptr += sizeof(level);

        word_con = *(unsigned *)(my_packet.data + ptr);
        ptr += sizeof(word_con);
        break;
    case SIGN_UP_R:
        is_sign_up = my_packet.data[ptr++];

        uid = *(unsigned *)(my_packet.data + ptr);
        ptr += sizeof(uid);
        break;
    case MY_INFO:
        uid = *(unsigned *)(my_packet.data + ptr);
        ptr += sizeof(uid);
        break;
    case MY_INFO_R:
        user_type = my_packet.data[ptr++];

        level = *(unsigned *)(my_packet.data + ptr);
        ptr += sizeof(level);

        if (user_type == USERTYPE_C)
        {
            word_con = *(unsigned *)(my_packet.data + ptr);
            ptr += sizeof(word_con);
        }
        else
        {
            exp = *(double *)(my_packet.data + ptr);
            ptr += sizeof(exp);

            passed = *(unsigned *)(my_packet.data + ptr);
            ptr += sizeof(passed);
        }
        break;
    case CHANGE_ROLE:
        uid = *(unsigned *)(my_packet.data + ptr);
        ptr += sizeof(uid);
        break;
    case CHANGE_ROLE_ACK:
        break;
    case USER_LIST:
        user_type = my_packet.data[ptr++];

        user_per_page = *(unsigned short *)(my_packet.data + ptr);
        ptr += sizeof(user_per_page);

        page = *(unsigned *)(my_packet.data + ptr);
        ptr += sizeof(page);
        break;
    case USER_LIST_R:
        user_type = my_packet.data[ptr++];

        user_num = *(unsigned short *)(my_packet.data + ptr);
        ptr += sizeof(user_num);

        if (user_type == USERTYPE_C)
            for (int i = 0; i < user_num; i++)
                con_vec.push_back(ReadContributorPacket(my_packet.data, ptr));
        else
            for (int i = 0; i < user_num; i++)
                player_vec.push_back(ReadPlayerPacket(my_packet.data, ptr));
        break;
    case SORT:
        uid = *(unsigned *)(my_packet.data + ptr);
        ptr += sizeof(uid);

        user_type = my_packet.data[ptr++];

        user_per_page = *(unsigned short *)(my_packet.data + ptr);
        ptr += sizeof(user_per_page);

        sort_type = my_packet.data[ptr++];
        break;
    case FILTER_NAME:
        uid = *(unsigned *)(my_packet.data + ptr);
        ptr += sizeof(uid);

        filter_type = FT_NAME;

        user_type = my_packet.data[ptr++];

        user_per_page = *(unsigned short *)(my_packet.data + ptr);
        ptr += sizeof(user_per_page);

        name_len = my_packet.data[ptr++];

        name = ReadStr(my_packet.data, name_len, ptr);
        break;
    case FILTER_OTHER:
        uid = *(unsigned *)(my_packet.data + ptr);
        ptr += sizeof(uid);

        user_type = my_packet.data[ptr++];

        user_per_page = *(unsigned short *)(my_packet.data + ptr);
        ptr += sizeof(user_per_page);

        filter_type = my_packet.data[ptr++];
        switch (filter_type)
        {
        case FT_LV:
            level = *(unsigned *)(my_packet.data + ptr);
            ptr += sizeof(level);
            break;
        case FT_PASSED:
            passed = *(unsigned *)(my_packet.data + ptr);
            ptr += sizeof(passed);
            break;
        case FT_WORD:
            word_con = *(unsigned *)(my_packet.data + ptr);
            ptr += sizeof(word_con);
            break;
        case FT_XP:
            exp = *(double *)(my_packet.data + ptr);
            ptr += sizeof(exp);
            break;
        }

        break;
    case SF_R:
        user_type = my_packet.data[ptr++];

        user_num = *(unsigned short *)(my_packet.data + ptr);
        ptr += sizeof(user_num);

        if (user_type == USERTYPE_C)
            for (int i = 0; i < user_num; i++)
                con_vec.push_back(ReadContributorPacket(my_packet.data, ptr));
        else
            for (int i = 0; i < user_num; i++)
                player_vec.push_back(ReadPlayerPacket(my_packet.data, ptr));

        break;
    case SF_PAGE:
        uid = *(unsigned *)(my_packet.data + ptr);
        ptr += sizeof(uid);

        user_type = my_packet.data[ptr++];

        user_per_page = *(unsigned short *)(my_packet.data + ptr);
        ptr += sizeof(user_per_page);

        page = *(unsigned *)(my_packet.data + ptr);
        ptr += sizeof(page);
        break;
    case GET_WORDS:
        passed = *(unsigned *)(my_packet.data + ptr);
        ptr += sizeof(passed);
        break;
    case GET_WORDS_R:
        time = *(unsigned short *)(my_packet.data + ptr);
        ptr += sizeof(time);

        round = my_packet.data[ptr++];

        for (int i = 0; i < round; i++)
            word_vec.push_back(ReadWordPacket(my_packet.data, ptr));
        break;
    case INC_XP:
        uid = *(unsigned *)(my_packet.data + ptr);
        ptr += sizeof(uid);

        exp = *(double *)(my_packet.data + ptr);
        ptr += sizeof(exp);
        break;
    case WORD_CON:
        uid = *(unsigned *)(my_packet.data + ptr);
        ptr += sizeof(uid);

        word_len = my_packet.data[ptr++];

        word = ReadStr(my_packet.data, word_len, ptr);
        break;
    case LOG_OUT:
        uid = *(unsigned *)(my_packet.data + ptr);
        ptr += sizeof(uid);
        break;
    case INC_XP_ACK:
        break;
    case WORD_CON_ACK:
        is_con = my_packet.data[ptr++];
        break;
    case LOG_OUT_ACK:
        break;
    case LOG_IN_FAIL:
		is_log_in = 0;
        break;
    case SIGN_UP_FAIL:
		is_sign_up = 0;
        break;
    default:
        break;
    }

    // crc_check
}

// 负责处理 len 及 data, from 由 controller添加
MyPacket Preparer::Encode()
{
    int len = 0;
    char buffer[2048];
    MyPacket res;

    CopyToCSTR(kind, buffer, len);
	CopyToCSTR(client_port, buffer, len);

    switch (kind)
    {
    case LOG_IN:
        CopyToCSTR(name_len, buffer, len);

        CopyToCSTR(name, buffer, len);

        CopyToCSTR(pswd_len, buffer, len);

        CopyToCSTR(pswd, buffer, len);
        break;
    case LOG_IN_RP:
        CopyToCSTR(is_log_in, buffer, len);

        CopyToCSTR(uid, buffer, len);

        CopyToCSTR(user_type, buffer, len);

        CopyToCSTR(level, buffer, len);

        CopyToCSTR(exp, buffer, len);

        CopyToCSTR(passed, buffer, len);
        break;
    case LOG_IN_RC:
        CopyToCSTR(is_log_in, buffer, len);

        CopyToCSTR(uid, buffer, len);

        CopyToCSTR(user_type, buffer, len);

        CopyToCSTR(level, buffer, len);

        CopyToCSTR(word_con, buffer, len);
        break;
    case SIGN_UP:
        CopyToCSTR(name_len, buffer, len);

        CopyToCSTR(name, buffer, len);

        CopyToCSTR(pswd_len, buffer, len);

        CopyToCSTR(pswd, buffer, len);
        break;
    case SIGN_UP_R:
        CopyToCSTR(is_sign_up, buffer, len);

        CopyToCSTR(uid, buffer, len);
        break;
    case MY_INFO:
        CopyToCSTR(uid, buffer, len);
        break;
    case MY_INFO_R:
        CopyToCSTR(user_type, buffer, len);

        CopyToCSTR(level, buffer, len);

        if (user_type == USERTYPE_C)
        {
            CopyToCSTR(word_con, buffer, len);
        }
        else
        {
            CopyToCSTR(exp, buffer, len);

            CopyToCSTR(passed, buffer, len);
        }
        break;
    case CHANGE_ROLE:
        CopyToCSTR(uid, buffer, len);
        break;
    case CHANGE_ROLE_ACK:
        break;
    case USER_LIST:
        CopyToCSTR(user_type, buffer, len);

        CopyToCSTR(user_per_page, buffer, len);

        CopyToCSTR(page, buffer, len);
        break;
    case USER_LIST_R:
        CopyToCSTR(user_type, buffer, len);

        CopyToCSTR(user_num, buffer, len);

        if (user_type == USERTYPE_C)
            for (int i = 0; i < user_num; i++)
                CopyToCSTR(con_vec.at(i), buffer, len);
        else
            for (int i = 0; i < user_num; i++)
                CopyToCSTR(player_vec.at(i), buffer, len);
        break;
    case SORT:
        CopyToCSTR(uid, buffer, len);

        CopyToCSTR(user_type, buffer, len);

        CopyToCSTR(user_per_page, buffer, len);

        CopyToCSTR(sort_type, buffer, len);
        break;
    case FILTER_NAME:
        CopyToCSTR(uid, buffer, len);

        CopyToCSTR(user_type, buffer, len);

        CopyToCSTR(user_per_page, buffer, len);

        CopyToCSTR(name_len, buffer, len);

        CopyToCSTR(name, buffer, len);
        break;
    case FILTER_OTHER:
        CopyToCSTR(uid, buffer, len);

        CopyToCSTR(user_type, buffer, len);

        CopyToCSTR(user_per_page, buffer, len);

        CopyToCSTR(filter_type, buffer, len);

        switch (filter_type)
        {
        case FT_LV:
            CopyToCSTR(level, buffer, len);
            break;
        case FT_PASSED:
            CopyToCSTR(passed, buffer, len);
            break;
        case FT_WORD:
            CopyToCSTR(word_con, buffer, len);
            break;
        case FT_XP:
            CopyToCSTR(exp, buffer, len);
            break;
        }
        break;
    case SF_R:
        CopyToCSTR(user_type, buffer, len);

        CopyToCSTR(user_num, buffer, len);

        if (user_type == USERTYPE_C)
            for (int i = 0; i < user_num; i++)
                CopyToCSTR(con_vec.at(i), buffer, len);
        else
            for (int i = 0; i < user_num; i++)
                CopyToCSTR(player_vec.at(i), buffer, len);

        break;
    case SF_PAGE:
        CopyToCSTR(uid, buffer, len);

        CopyToCSTR(user_type, buffer, len);

        CopyToCSTR(user_per_page, buffer, len);

        CopyToCSTR(page, buffer, len);
        break;
    case GET_WORDS:
        CopyToCSTR(passed, buffer, len);
        break;
    case GET_WORDS_R:
        CopyToCSTR(time, buffer, len);

        CopyToCSTR(round, buffer, len);

        for (int i = 0; i < round; i++)
            CopyToCSTR(word_vec.at(i), buffer, len);
        break;
    case INC_XP:
        CopyToCSTR(uid, buffer, len);

        CopyToCSTR(exp, buffer, len);
        break;
    case WORD_CON:
        CopyToCSTR(uid, buffer, len);

        CopyToCSTR(word_len, buffer, len);

        CopyToCSTR(word, buffer, len);
        break;
    case LOG_OUT:
        CopyToCSTR(uid, buffer, len);
        break;
    case INC_XP_ACK:
        break;
    case WORD_CON_ACK:
        CopyToCSTR(is_con, buffer, len);
        break;
    case LOG_OUT_ACK:
        break;
    case LOG_IN_FAIL:
        break;
    case SIGN_UP_FAIL:
        break;
    default:
        break;
    }
    // crc checksum
    res.len = len;
    res.data = new char[len];
    std::memcpy(res.data, buffer, len);
    return res;
}

void Preparer::CopyToCSTR(const unsigned char c, char *buffer, int &ptr)
{
    buffer[ptr++] = c;
}

void Preparer::CopyToCSTR(const unsigned short s, char *buffer, int &ptr)
{
    unsigned short *ps = reinterpret_cast<unsigned short *>(buffer + ptr);
    *ps = s;
    ptr += sizeof(s);
}
void Preparer::CopyToCSTR(const unsigned u, char *buffer, int &ptr)
{
    unsigned *pu = reinterpret_cast<unsigned *>(buffer + ptr);
    *pu = u;
    ptr += sizeof(u);
}
void Preparer::CopyToCSTR(const double d, char *buffer, int &ptr)
{
    double *pd = reinterpret_cast<double *>(buffer + ptr);
    *pd = d;
    ptr += sizeof(d);
}

void Preparer::CopyToCSTR(const PlayerPacket &pp, char *buffer, int &ptr)
{
    CopyToCSTR(pp.user.name_len, buffer, ptr);

    CopyToCSTR(pp.user.name, buffer, ptr);

    CopyToCSTR(pp.user.level, buffer, ptr);

    CopyToCSTR(pp.exp, buffer, ptr);

    CopyToCSTR(pp.passed, buffer, ptr);
}
void Preparer::CopyToCSTR(const ContributorPacket &cp, char *buffer, int &ptr)
{
    CopyToCSTR(cp.user.name_len, buffer, ptr);

    CopyToCSTR(cp.user.name, buffer, ptr);

    CopyToCSTR(cp.user.level, buffer, ptr);

    CopyToCSTR(cp.word_con, buffer, ptr);
}
void Preparer::CopyToCSTR(const WordPacket &wp, char *buffer, int &ptr)
{
    CopyToCSTR(wp.word_len, buffer, ptr);

    CopyToCSTR(wp.word, buffer, ptr);

    CopyToCSTR(wp.difficulty, buffer, ptr);
}
void Preparer::CopyToCSTR(const std::string &str, char *buffer, int &ptr)
{
    for (const auto &c : str)
        CopyToCSTR(static_cast<unsigned char>(c), buffer, ptr);
}

UserPacket Preparer::GenUserPacket(const User &user)
{
    static UserPacket temp;
    temp.name = user.get_user_name();
    temp.level = user.get_level();
    temp.name_len = temp.name.size();
    return temp;
}

PlayerPacket Preparer::GenPlayerPacket(const Player &player)
{
    static PlayerPacket temp;
    temp.user = GenUserPacket(player);
    temp.exp = player.get_exp();
    temp.passed = player.get_level_passed();
    return temp;
}

ContributorPacket Preparer::GenContributorPacket(const Contributor &contributor)
{
    static ContributorPacket temp;
    temp.user = GenUserPacket(contributor);
    temp.word_con = contributor.get_word_contributed();
    return temp;
}

WordPacket Preparer::GenWordPacket(const Word &word)
{
    static WordPacket temp;
	temp.word_len = word.word.size();
    temp.word = word.word;
    temp.difficulty = word.difficulty;
    return temp;
}

std::string Preparer::ReadStr(const char *buffer, int len, int &ptr)
{
    std::string temp;
    for (int i = 0; i < len; i++)
        temp.push_back(buffer[ptr++]);
    return std::move(temp);
}

UserPacket Preparer::ReadUserPacket(const char *buffer, int &ptr)
{
    UserPacket temp;
    temp.name_len = buffer[ptr++];

    temp.name = ReadStr(buffer, temp.name_len, ptr);

    temp.level = *(unsigned *)(buffer + ptr);
    ptr += sizeof(temp.level);
    return temp;
}

PlayerPacket Preparer::ReadPlayerPacket(const char *buffer, int &ptr)
{
    PlayerPacket temp;

    temp.user = ReadUserPacket(buffer, ptr);

    temp.exp = *(double *)(buffer + ptr);
    ptr += sizeof(temp.exp);

    temp.passed = *(unsigned *)(buffer + ptr);
    ptr += sizeof(temp.passed);

    return temp;
}

ContributorPacket Preparer::ReadContributorPacket(const char *buffer, int &ptr)
{
    ContributorPacket temp;

    temp.user = ReadUserPacket(buffer, ptr);

    temp.word_con = *(unsigned *)(buffer + ptr);
    ptr += sizeof(temp.word_con);

    return temp;
}

WordPacket Preparer::ReadWordPacket(const char *buffer, int &ptr)
{
    WordPacket temp;

    temp.word_len = buffer[ptr++];

    temp.word = ReadStr(buffer, temp.word_len, ptr);

    temp.difficulty = buffer[ptr++];
    return temp;
}