#include <string>
#include <vector>
#include <fstream>
#include <set>
#include <algorithm>

#include "account_sys.h"
#include "player.h"
#include "contributor.h"
#include "log.h"

const std::string AccountSys::account_path_ = "../data/acnt.txt";

AccountSys::AccountSys()
{
    std::ofstream ofs_create_file(account_path_, std::fstream::app | std::fstream::binary); // create account file if not exist

    std::ifstream ifs(account_path_, std::fstream::binary);
    if (ifs)
    {
        Contributor temp_c;
        Player temp_p;
        char utype;
        
        while (ifs)
        {
            ifs.read(&utype, sizeof(utype));
            switch (utype)
            {
            case UserType::USERTYPE_C:
                temp_c.Load(ifs);
                temp_c.set_user_type(static_cast<UserType>(utype));
                contributors_.push_back(temp_c);
                con_map_[temp_c.get_user_name()] = contributors_.size() - 1;
                break;
            case UserType::USERTYPE_P:
                temp_p.Load(ifs);
                temp_p.set_user_type(static_cast<UserType>(utype));
                players_.push_back(temp_p);
                player_map_[temp_p.get_user_name()] = players_.size() - 1;
                break;
            default:
                break;
            }
        }
        total_user_ = contributors_.size() + players_.size();
		Log::WriteLog(std::string("AccSys: successfully initialized, load ") + std::to_string(total_user_) + "users");
        // Error Handling
    }
    else
    {
        Log::WriteLog(std::string("AccSys: Cannot open the account file, path: ") + account_path_);
    }
}

bool AccountSys::LogIn(const std::string &name, const std::string &password)
{
    if (is_online(name))
    {
        Log::WriteLog(std::string("AccSys: ") + name + "is already online");
        return false;
    }
    else
    {
        std::string pswd;
        UserType utype = UserType::USERTYPE_U;
        size_t pos;
        if (con_map_.count(name))
        {
            pos = con_map_.at(name);
            pswd = contributors_[pos].get_password();
            utype = contributors_[pos].get_user_type();
        }
        else if (player_map_.count(name))
        {
            pos = player_map_.at(name);
            pswd = players_[pos].get_password();
            utype = players_[pos].get_user_type();
        }
		else
		{
			Log::WriteLog(std::string("AccSys: Log in failed : Has no user named ") + name);
			return false;
		}

        if (password == pswd)
        {
            switch (utype)
            {
            case UserType::USERTYPE_C:
                online_contributors_.insert(pos);
                break;
            case UserType::USERTYPE_P:
                online_players_.insert(pos);
                break;
            default:
                break;
            }
            return true;
        }
        else
        {
            Log::WriteLog(std::string("AccSys: ") + name + " failed to log in: error password");
            return false;
        }
    }
}

// check the parametres outside
bool AccountSys::SignUp(const std::string &name, const std::string &password, UserType utype)
{
    if (is_repeat(name))
    {
        Log::WriteLog(std::string("AccSys: ") + "Sign up failed : name repeated, name:" + name);
        return false;
    }
    else
    {
        switch (utype)
        {
        case UserType::USERTYPE_C:
            contributors_.push_back(Contributor(name, password, utype));
            con_map_[name] = contributors_.size() - 1;
            break;
        case UserType::USERTYPE_P:
            players_.push_back(Player(name, password, utype));
            player_map_[name] = players_.size() - 1;
            break;
        default:
            break;
        }
        total_user_++;
        return true;
    }
}

Contributor AccountSys::get_contributor(const std::string &name) const
{
    if (con_map_.count(name))
        return contributors_[con_map_.at(name)];
    else
        return Contributor();
}

Contributor &AccountSys::get_ref_contributor(const std::string &name)
{
	if (con_map_.count(name))
		return contributors_[con_map_.at(name)];
	else
		throw std::logic_error(std::string("AccSys: ") + std::string("no contributor named ") + name);
}

Player AccountSys::get_player(const std::string &name) const
{
    if (player_map_.count(name))
        return players_[player_map_.at(name)];
    else
        return Player();
}


Player &AccountSys::get_ref_player(const std::string &name)
{
    if (player_map_.count(name))
        return players_[player_map_.at(name)];
	else
		throw std::logic_error(std::string("AccSys: ") + std::string("no player names ") + name);
}

bool AccountSys::Save() const
{
    std::ofstream ofs(account_path_, std::ofstream::binary);

    for (const auto &c : contributors_)
        c.Save(ofs);

    for (const auto &p : players_)
        p.Save(ofs);

    return true;
    // error handling
}

bool AccountSys::is_online(const std::string &name) const
{
    if (con_map_.count(name))
        return online_contributors_.count(con_map_.at(name));
    else if (player_map_.count(name))
        return online_players_.count(player_map_.at(name));
    else
        return false;
}