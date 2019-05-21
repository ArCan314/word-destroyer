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

MyPacket ServerController::get_responce(const MyPacket &quest)
{
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
        preparer_.user_num = std::distance(temp.second, temp.first);
        for (auto i = temp.first; i != temp.second; ++i)
            preparer_.con_vec.push_back(preparer_.GenContributorPacket(*i));
    }
    else
    {
        auto temp = acc_sys_->get_player_page(preparer_.page, preparer_.user_per_page);
        preparer_.user_num = std::distance(temp.second, temp.first);
        for (auto i = temp.first; i != temp.second; ++i)
            preparer_.player_vec.push_back(preparer_.GenPlayerPacket(*i));
    }
    return true;
}

bool ServerController::Sort()
{
    preparer_.kind = SF_R;
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
    FilterPacket filter_packet;

    filter_packet.filter_type = preparer_.filter_type;
    switch (preparer_.filter_type)
    {
    case FPT_NAME:
        filter_packet.value.val_str = preparer_.name;
        break;
    case FPT_LV:
        filter_packet.value.val_32 = preparer_.level;
        break;
    case FPT_CON:
        filter_packet.value.val_32 = preparer_.word_con;
        break;
    case FPT_EXP:
        filter_packet.value.val_double = preparer_.exp;
        break;
    case FPT_PASS:
        filter_packet.value.val_32 = preparer_.passed;
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
    auto name = acc_sys_->get_name(preparer_.uid);
    acc_sys_->LogOut(name);
    return true;
}