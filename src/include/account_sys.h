#pragma once

#include <string>
#include <vector>
#include <set>
#include <map>

#include "player.h"
#include "contributor.h"
enum UserType;

class AccountSys
{
public:
	AccountSys();
	~AccountSys() { Save(); }

	bool LogIn(const std::string &name, const std::string &password);
	bool SignUp(const std::string &name, const std::string &password, UserType utype);
	void LogOut(const std::string &name);

	constexpr std::size_t get_min_acc_len() { return 4; }
	constexpr std::size_t get_min_pswd_len() { return 9; }

	Contributor get_contributor(const std::string &name) const;
	Contributor &get_ref_contributor(const std::string &name);

	Player get_player(const std::string &name) const;
	Player &get_ref_player(const std::string &name);

	bool is_repeat(const std::string &name) const
	{
		return con_map_.count(name) || player_map_.count(name);
	}

	std::vector<Contributor>::const_iterator get_contributors_cbegin() const { return contributors_.cbegin(); }
	std::vector<Contributor>::const_iterator get_contributors_cend() const { return contributors_.cend(); }
	std::vector<Player>::const_iterator get_players_cbegin() const { return players_.cbegin(); }
	std::vector<Player>::const_iterator get_players_cend() const { return players_.cend(); }

	bool Save() const;

	bool is_online(const std::string &name) const;

	std::size_t get_online_con_number() const { return online_contributors_.size(); }
	std::size_t get_online_player_number() const { return online_players_.size(); }
	std::size_t get_online_number() const { return get_online_con_number() + get_online_player_number(); }

	std::size_t get_user_con_number() const { return contributors_.size(); }
	std::size_t get_user_player_number() const { return players_.size(); }
	std::size_t get_user_number() const { return get_user_con_number() + get_user_player_number(); }

	const std::string &get_current_user_str() const { return current_user_str_; }
	UserType get_current_usertype() const { return get_usertype(current_user_str_); }

	void DelAccount(const std::string &name);

	std::string  get_utype_str(UserType utype) const;
	void ChangeRole(const std::string &name);

	UserType get_usertype(const std::string &name) const
	{
		if (con_map_.count(current_user_str_))
			return UserType::USERTYPE_C;
		else if (player_map_.count(current_user_str_))
			return UserType::USERTYPE_P;
		else
			return UserType::USERTYPE_U;
	}

	// DEBUG MEMBERS
	void set_current_user() 
	{
		if (players_.size())
			current_user_str_ = players_.front().get_user_name();
		else if (contributors_.size())
			current_user_str_ = contributors_.front().get_user_name();
	}

private:
	static const std::string account_path_;
	std::vector<Contributor> contributors_;
	std::vector<Player> players_;
	std::map<std::string, size_t> con_map_;
	std::map<std::string, size_t> player_map_;
	std::set<std::size_t> online_players_;
	std::set<std::size_t> online_contributors_;
	std::string current_user_str_;
	int total_user_ = 0;
};