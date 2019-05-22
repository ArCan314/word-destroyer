#include <string>
#include <utility>
#include <algorithm>

#include "controller.h"
#include "word_list.h"
#include "account_sys.h"
#include "my_packet.h"
#include "player.h"
#include "contributor.h"
#include "game.h"

static std::mutex mutex_;

MyPacket ServerController::get_responce(const MyPacket &quest)
{
	std::lock_guard<std::mutex> lok(mutex_);
    MyPacket send_packet;

    preparer_.Decode(quest);
    kind_ = static_cast<NetPktKind>(preparer_.kind);
    switch (preparer_.kind)
    {
    case LOG_IN:
        LogIn();
        break;
    case SIGN_UP:
        SignUp();
        break;
    case MY_INFO:
        MyInfo();
        break;
    case CHANGE_ROLE:
        ChangeRole();
        break;
    case USER_LIST:
        UserList();
        break;
    case SORT:
        Sort();
        break;
    case FILTER_NAME:
    case FILTER_OTHER:
        Filter();
        break;
    case SF_PAGE:
        SFTurnPage();
        break;
    case GET_WORDS:
        GetWords();
        break;
    case INC_XP:
        IncXP();
        break;
    case WORD_CON:
        WordCon();
        break;
    case LOG_OUT:
        LogOut();
        break;
    }

    send_packet = preparer_.Encode();
    preparer_.Clear();
    send_packet.from = quest.from;
	send_packet.from.sin_port = client_port_.sin_port;
    return send_packet;
}

bool ServerController::LogIn()
{
    if (acc_sys_->LogIn(preparer_.name, preparer_.pswd))
    {
        if (acc_sys_->get_usertype(preparer_.name) == USERTYPE_C)
        {
            auto temp = acc_sys_->get_contributor(preparer_.name);
            preparer_.kind = LOG_IN_RC;
            preparer_.user_type = USERTYPE_C;
            preparer_.level = temp.get_level();
            preparer_.word_con = temp.get_word_contributed();
        }
        else
        {
            auto temp = acc_sys_->get_player(preparer_.name);
            preparer_.kind = LOG_IN_RP;
            preparer_.user_type = USERTYPE_P;
            preparer_.level = temp.get_level();
            preparer_.exp = temp.get_exp();
            preparer_.passed = temp.get_level_passed();
        }
        preparer_.is_log_in = true;
        preparer_.uid = acc_sys_->get_uid(preparer_.name);

        return true;
    }
    else
    {
        preparer_.kind = LOG_IN_FAIL;
        return false;
    }
}

bool ServerController::SignUp()
{
    if (acc_sys_->SignUp(preparer_.name, preparer_.pswd, USERTYPE_P))
    {
        preparer_.kind = SIGN_UP_R;
		preparer_.is_sign_up = 1;
        preparer_.uid = acc_sys_->get_uid(preparer_.name);
        return true;
    }
    else
    {
        preparer_.kind = SIGN_UP_FAIL;
        return false;
    }
}

bool ServerController::MyInfo()
{
    preparer_.kind = MY_INFO_R;
    std::string name = acc_sys_->get_name(preparer_.uid);
    preparer_.user_type = acc_sys_->get_usertype(name);
    if (preparer_.user_type == USERTYPE_C)
    {
        auto temp = acc_sys_->get_ref_contributor(name);
        preparer_.level = temp.get_level();
        preparer_.word_con = temp.get_word_contributed();
    }
    else
    {
        auto temp = acc_sys_->get_ref_player(name);
        preparer_.level = temp.get_level();
        preparer_.exp = temp.get_exp();
        preparer_.passed = temp.get_level_passed();
    }
    return true;
}

bool ServerController::ChangeRole()
{
    preparer_.kind = CHANGE_ROLE_ACK;
    std::string name = acc_sys_->get_name(preparer_.uid);
    acc_sys_->ChangeRole(name);
    return true;
}

bool ServerController::UserList()
{
    preparer_.kind = USER_LIST_R;
    if (preparer_.user_type == USERTYPE_C)
    {
        auto temp = acc_sys_->get_contributor_page(preparer_.page, preparer_.user_per_page);
        preparer_.user_num = std::distance(temp.first, temp.second);
        for (auto i = temp.first; i != temp.second; ++i)
            preparer_.con_vec.push_back(preparer_.GenContributorPacket(*i));
    }
    else
    {
        auto temp = acc_sys_->get_player_page(preparer_.page, preparer_.user_per_page);
        preparer_.user_num = std::distance(temp.first, temp.second);
        for (auto i = temp.first; i != temp.second; ++i)
            preparer_.player_vec.push_back(preparer_.GenPlayerPacket(*i));
    }
    return true;
}

bool ServerController::Sort()
{
    preparer_.kind = SF_R;
	preparer_.page = 1;
    if (preparer_.user_type == USERTYPE_C)
    {
        auto temp = acc_sys_->get_sort_con_page(preparer_.uid, preparer_.page, preparer_.user_per_page, preparer_.sort_type);
        preparer_.user_num = std::distance(temp.first, temp.second);

        for (auto i = temp.first; i != temp.second; ++i)
            preparer_.con_vec.push_back(preparer_.GenContributorPacket(*i));
    }
    else
    {
        auto temp = acc_sys_->get_sort_player_page(preparer_.uid, preparer_.page, preparer_.user_per_page, preparer_.sort_type);
        preparer_.user_num = std::distance(temp.first, temp.second);

        for (auto i = temp.first; i != temp.second; ++i)
            preparer_.player_vec.push_back(preparer_.GenPlayerPacket(*i));
    }
    return true;
}

bool ServerController::Filter()
{
    preparer_.kind = SF_R;
	preparer_.page = 1;
    FilterPacket filter_packet;

    filter_packet.filter_type = preparer_.filter_type;
    switch (preparer_.filter_type)
    {
    case FPT_NAME:
        filter_packet.val_str = preparer_.name;
        break;
    case FPT_LV:
        filter_packet.val_32 = preparer_.level;
        break;
    case FPT_CON:
        filter_packet.val_32 = preparer_.word_con;
        break;
    case FPT_EXP:
        filter_packet.val_double = preparer_.exp;
        break;
    case FPT_PASS:
        filter_packet.val_32 = preparer_.passed;
        break;
    default:
        break;
    }

    if (preparer_.user_type == USERTYPE_C)
    {

        auto temp = acc_sys_->get_filter_con_page(preparer_.uid, preparer_.page, preparer_.user_per_page, filter_packet);
        preparer_.user_num = std::distance(temp.first, temp.second);

        for (auto i = temp.first; i != temp.second; ++i)
            preparer_.con_vec.push_back(preparer_.GenContributorPacket(*i));
    }
    else
    {
        auto temp = acc_sys_->get_filter_player_page(preparer_.uid, preparer_.page, preparer_.user_per_page, filter_packet);
        preparer_.user_num = std::distance(temp.first, temp.second);

        for (auto i = temp.first; i != temp.second; ++i)
            preparer_.player_vec.push_back(preparer_.GenPlayerPacket(*i));
    }
    return true;
}

bool ServerController::SFTurnPage()
{
    preparer_.kind = SF_R;
    if (preparer_.user_type == USERTYPE_C)
    {
        auto temp = acc_sys_->SortFilterTurnPageC(preparer_.uid, preparer_.page, preparer_.user_per_page);
        preparer_.user_num = std::distance(temp.first, temp.second);

        for (auto i = temp.first; i != temp.second; ++i)
            preparer_.con_vec.push_back(preparer_.GenContributorPacket(*i));
    }
    else
    {
        auto temp = acc_sys_->SortFilterTurnPageP(preparer_.uid, preparer_.page, preparer_.user_per_page);
        preparer_.user_num = std::distance(temp.first, temp.second);

        for (auto i = temp.first; i != temp.second; ++i)
            preparer_.player_vec.push_back(preparer_.GenPlayerPacket(*i));
    }
    return true;
}

bool ServerController::GetWords()
{
    preparer_.kind = GET_WORDS_R;
    preparer_.round = get_round(preparer_.passed);
    preparer_.time = get_round_time(preparer_.passed);
    for (int i = 0; i < preparer_.round; i++)
    {
        auto word = word_list_->get_word(preparer_.passed);
        preparer_.word_vec.push_back(preparer_.GenWordPacket(word));
    }
    return true;
}

bool ServerController::IncXP()
{
    preparer_.kind = INC_XP_ACK;
    auto name = acc_sys_->get_name(preparer_.uid);
    auto &p = acc_sys_->get_ref_player(name);
    p.raise_exp(preparer_.exp);
    p.inc_level();
    p.inc_level_passed();
    return true;
}

bool ServerController::WordCon()
{
    preparer_.kind = WORD_CON_ACK;
    auto name = acc_sys_->get_name(preparer_.uid);
    if (word_list_->AddWord(preparer_.word, name))
    {
		auto &temp = acc_sys_->get_ref_contributor(name);
		temp.inc_word_contributed();
		temp.inc_level();
        preparer_.is_con = 1;
        return true;
    }
    else
    {
        preparer_.is_con = 0;
        return false;
    }
}

bool ServerController::LogOut()
{
    preparer_.kind = LOG_OUT_ACK;
    acc_sys_->LogOut(preparer_.uid);
    return true;
}

bool ClientController::LogIn(const std::string &name, const std::string &pswd)
{
    preparer_.Clear();

    preparer_.kind = LOG_IN;
    preparer_.name_len = name.size();
    preparer_.name = name;
    preparer_.pswd_len = pswd.size();
    preparer_.pswd = pswd;

    auto temp = preparer_.Encode();
    temp.from = server_addr_;

    send_sock_.SendTo(temp);
    delete[] temp.data;
    temp = recv_sock_.RecvFrom();

    preparer_.Decode(temp);
    delete[] temp.data;

    if (preparer_.is_log_in == true)
    {
        if (preparer_.user_type == USERTYPE_C)
        {
            auto con = std::move(Contributor(name, pswd, USERTYPE_C));
			con.set_level(preparer_.level);
            con.set_word_contributed(preparer_.word_con);
            acc_sys_->set_user(preparer_.uid, con);
        }
        else
        {
            auto player = std::move(Player(name, pswd, USERTYPE_P));
			player.set_level(preparer_.level);
            player.set_level_passed(preparer_.passed);
            player.set_xp(preparer_.exp);
            acc_sys_->set_user(preparer_.uid, player);
        }
        return true;
    }
    else
    {
        return false;
    }
}

bool ClientController::SignUp(const std::string &name, const std::string &pswd)
{
    preparer_.Clear();

    preparer_.kind = SIGN_UP;
    preparer_.name_len = name.size();
    preparer_.name = name;
    preparer_.pswd_len = pswd.size();
    preparer_.pswd = pswd;

    auto temp = preparer_.Encode();
    temp.from = server_addr_;

    send_sock_.SendTo(temp);

    delete[] temp.data;
    temp = recv_sock_.RecvFrom();

    preparer_.Decode(temp);
    delete[] temp.data;

    if (preparer_.is_sign_up == true)
    {
        auto player = std::move(Player(name, pswd, USERTYPE_P));
        acc_sys_->set_user(preparer_.uid, player);

        return true;
    }
    else
    {
        return false;
    }
}

bool ClientController::MyInfo(const unsigned uid)
{
    // already exists in the device
	return true;
}
bool ClientController::ChangeRole(const unsigned uid)
{
    preparer_.Clear();

    preparer_.kind = CHANGE_ROLE;
    preparer_.uid = uid;

    auto temp = preparer_.Encode();
    temp.from = server_addr_;

    send_sock_.SendTo(temp);

    delete[] temp.data;
    temp = recv_sock_.RecvFrom();

    preparer_.Decode(temp);
    delete[] temp.data;

    if (preparer_.kind == CHANGE_ROLE_ACK)
    {
		acc_sys_->change_user_type();
        return true;
    }
    else
    {
        return false;
    }
}

bool ClientController::UserList(const unsigned char utype, unsigned short user_per_page, unsigned page)
{
    preparer_.Clear();

    preparer_.kind = USER_LIST;
    preparer_.user_type = utype;
    preparer_.user_per_page = user_per_page;
    preparer_.page = page;

    auto temp = preparer_.Encode();
    temp.from = server_addr_;

    send_sock_.SendTo(temp);

    delete[] temp.data;
    temp = recv_sock_.RecvFrom();

    preparer_.Decode(temp);
    delete[] temp.data;

    if (preparer_.user_type == USERTYPE_C)
    {
        std::vector<Contributor> temp;

        for (int i = 0; i < preparer_.con_vec.size(); i++)
        {
            auto con_packet = preparer_.con_vec.at(i);
            auto con = Contributor(con_packet.user.name, "", USERTYPE_C);
            con.set_word_contributed(con_packet.word_con);
            con.set_level(con_packet.user.level);
            temp.push_back(con);
        }
        acc_sys_->set_contributor_vec(temp.begin(), temp.end());
    }
    else
    {
        std::vector<Player> temp;

        for (int i = 0; i < preparer_.player_vec.size(); i++)
        {
            auto player_packet = preparer_.player_vec.at(i);
            auto player = Player(player_packet.user.name, "", USERTYPE_P);
            player.set_xp(player_packet.exp);
            player.set_level_passed(player_packet.passed);
            player.set_level(player_packet.user.level);
            temp.push_back(player);
        }
        acc_sys_->set_player_vec(temp.begin(), temp.end());
    }
    return true;
}

bool ClientController::Sort(unsigned uid, const unsigned char utype, unsigned short user_per_page, unsigned char sort_type)
{
    preparer_.Clear();

    preparer_.kind = SORT;
    preparer_.uid = uid;
    preparer_.user_type = utype;
    preparer_.user_per_page = user_per_page;
    preparer_.sort_type = sort_type;

    auto temp = preparer_.Encode();
    temp.from = server_addr_;

    send_sock_.SendTo(temp);

    delete[] temp.data;
    temp = recv_sock_.RecvFrom();

    preparer_.Decode(temp);
    delete[] temp.data;

    if (preparer_.user_type == USERTYPE_C)
    {
        std::vector<Contributor> temp;

        for (int i = 0; i < preparer_.con_vec.size(); i++)
        {
            auto con_packet = preparer_.con_vec.at(i);
            auto con = Contributor(con_packet.user.name, "", USERTYPE_C);
            con.set_word_contributed(con_packet.word_con);
            con.set_level(con_packet.user.level);
            temp.push_back(con);
        }
        acc_sys_->set_contributor_vec(temp.begin(), temp.end());
    }
    else
    {
        std::vector<Player> temp;

        for (int i = 0; i < preparer_.player_vec.size(); i++)
        {
            auto player_packet = preparer_.player_vec.at(i);
            auto player = Player(player_packet.user.name, "", USERTYPE_P);
            player.set_xp(player_packet.exp);
            player.set_level_passed(player_packet.passed);
            player.set_level(player_packet.user.level);
            temp.push_back(player);
        }
        acc_sys_->set_player_vec(temp.begin(), temp.end());
    }
    return true;
}

bool ClientController::Filter(unsigned uid, const unsigned char utype, unsigned short user_per_page, const FilterPacket &filter_packet)
{
    preparer_.Clear();

    preparer_.kind = (filter_packet.filter_type == FPT_NAME) ? FILTER_NAME : FILTER_OTHER;
    preparer_.uid = uid;
    preparer_.user_type = utype;
    preparer_.user_per_page = user_per_page;
    preparer_.filter_type = filter_packet.filter_type;

    switch (filter_packet.filter_type)
    {
    case FPT_NAME:
        preparer_.name_len = filter_packet.val_str.size();
        preparer_.name = filter_packet.val_str;
        break;
    case FPT_CON:
        preparer_.word_con = filter_packet.val_32;
        break;
    case FPT_EXP:
        preparer_.exp = filter_packet.val_double;
        break;
    case FPT_LV:
        preparer_.level = filter_packet.val_32;
        break;
    case FPT_PASS:
        preparer_.passed = filter_packet.val_32;
        break;
    default:
        break;
    }

    MyPacket temp = preparer_.Encode();
    temp.from = server_addr_;

    send_sock_.SendTo(temp);

    delete[] temp.data;
    temp = recv_sock_.RecvFrom();

    preparer_.Decode(temp);
    delete[] temp.data;

    if (preparer_.user_type == USERTYPE_C)
    {
        std::vector<Contributor> temp;

        for (int i = 0; i < preparer_.con_vec.size(); i++)
        {
            ContributorPacket con_packet = preparer_.con_vec.at(i);
            Contributor con = Contributor(con_packet.user.name, "", USERTYPE_C);
            con.set_word_contributed(con_packet.word_con);
            con.set_level(con_packet.user.level);
            temp.push_back(con);
        }
        acc_sys_->set_contributor_vec(temp.begin(), temp.end());
    }
    else
    {
        std::vector<Player> temp;

        for (int i = 0; i < preparer_.player_vec.size(); i++)
        {
            PlayerPacket player_packet = preparer_.player_vec.at(i);
            Player player = Player(player_packet.user.name, "", USERTYPE_P);
            player.set_xp(player_packet.exp);
            player.set_level_passed(player_packet.passed);
            player.set_level(player_packet.user.level);
            temp.push_back(player);
        }
        acc_sys_->set_player_vec(temp.begin(), temp.end());
    }
    return true;
}

bool ClientController::SFTurnPage(unsigned uid, const unsigned char utype, unsigned short user_per_page, unsigned page)
{
    preparer_.Clear();

    preparer_.kind = SF_PAGE;
    preparer_.uid = uid;
    preparer_.user_type = utype;
    preparer_.user_per_page = user_per_page;
    preparer_.page = page;

    auto temp = preparer_.Encode();
    temp.from = server_addr_;

    send_sock_.SendTo(temp);

    delete[] temp.data;
    temp = recv_sock_.RecvFrom();

    preparer_.Decode(temp);
    delete[] temp.data;

    if (preparer_.user_type == USERTYPE_C)
    {
        std::vector<Contributor> temp;

        for (int i = 0; i < preparer_.con_vec.size(); i++)
        {
            auto con_packet = preparer_.con_vec.at(i);
            auto con = Contributor(con_packet.user.name, "", USERTYPE_C);
            con.set_word_contributed(con_packet.word_con);
            con.set_level(con_packet.user.level);
            temp.push_back(con);
        }
        acc_sys_->set_contributor_vec(temp.begin(), temp.end());
    }
    else
    {
        std::vector<Player> temp;

        for (int i = 0; i < preparer_.player_vec.size(); i++)
        {
            auto player_packet = preparer_.player_vec.at(i);
            auto player = Player(player_packet.user.name, "", USERTYPE_P);
            player.set_xp(player_packet.exp);
            player.set_level_passed(player_packet.passed);
            player.set_level(player_packet.user.level);
            temp.push_back(player);
        }
        acc_sys_->set_player_vec(temp.begin(), temp.end());
    }
    return true;
}

bool ClientController::GetWord(unsigned passed)
{
    preparer_.Clear();

    preparer_.kind = GET_WORDS;
    preparer_.passed = passed;

    auto temp = preparer_.Encode();
    temp.from = server_addr_;

    send_sock_.SendTo(temp);

    delete[] temp.data;
    temp = recv_sock_.RecvFrom();

    preparer_.Decode(temp);
    delete[] temp.data;

    word_list_->round = preparer_.round;
    word_list_->time = preparer_.time;

    for (int i = 0; i < word_list_->round; i++)
        word_list_->AddWord({preparer_.word_vec.at(i).word, preparer_.word_vec.at(i).difficulty});

    return true;
}

bool ClientController::IncXp(unsigned uid, double exp)
{
    preparer_.Clear();

    preparer_.kind = INC_XP;
    preparer_.uid = uid;
    preparer_.exp = exp;

    auto temp = preparer_.Encode();
    temp.from = server_addr_;

    send_sock_.SendTo(temp);

    delete[] temp.data;
    temp = recv_sock_.RecvFrom();

    preparer_.Decode(temp);
    delete[] temp.data;

    acc_sys_->get_player().raise_exp(exp);
    acc_sys_->get_player().inc_level_passed();
    acc_sys_->get_player().inc_level();

    return true;
}

bool ClientController::WordCon(unsigned uid, const std::string &word)
{
    preparer_.Clear();

    preparer_.kind = WORD_CON;
    preparer_.uid = uid;
    preparer_.word = word;
    preparer_.word_len = word.size();

    auto temp = preparer_.Encode();
    temp.from = server_addr_;

    send_sock_.SendTo(temp);

    delete[] temp.data;
    temp = recv_sock_.RecvFrom();

    preparer_.Decode(temp);
    delete[] temp.data;

    if (preparer_.is_con)
    {
        acc_sys_->get_con().inc_word_contributed();
        acc_sys_->get_con().inc_level();
        return true;
    }
    else
    {
        return false;
    }
}

bool ClientController::LogOut(unsigned uid)
{
    preparer_.Clear();

    preparer_.kind = LOG_OUT;
    preparer_.uid = uid;

    auto temp = preparer_.Encode();
    temp.from = server_addr_;

    send_sock_.SendTo(temp);

    delete[] temp.data;
    temp = recv_sock_.RecvFrom();

    preparer_.Decode(temp);
    delete[] temp.data;
    
    return true;
}