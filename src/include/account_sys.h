#pragma once

#include <cstring>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <utility>

#include "player.h"
#include "contributor.h"
enum UserType;

enum FilterPackType
{
	FPT_NAME = 0,
	FPT_LV,
	FPT_EXP,
	FPT_PASS,
	FPT_CON
};

struct FilterPacket
{
	FilterPacket() : filter_type(0), val_str(), val_32(0), val_double(0) {}
	int filter_type;
	std::string val_str;
	unsigned val_32;
	double val_double;

	FilterPacket &operator=(const FilterPacket &rhs)
	{
		filter_type = rhs.filter_type;
		switch (filter_type)
		{
		case FPT_NAME:
			val_str = rhs.val_str;
			break;
		case FPT_LV:
		case FPT_CON:
		case FPT_PASS:
			val_32 = rhs.val_32;
			break;
		case FPT_EXP:
			val_double = rhs.val_double;
			break;
		default:
			break;
		}
		return *this;
	}

	bool operator==(const FilterPacket &rhs)
	{
		bool res = false;
		if (filter_type == rhs.filter_type)
		{
			switch (filter_type)
			{
			case FPT_NAME:
				res = (val_str == rhs.val_str);
				break;
			case FPT_LV:
			case FPT_CON:
			case FPT_PASS:
				res = (val_32 == rhs.val_32);
				break;
			case FPT_EXP:
				res = (val_double == rhs.val_double);
				break;
			default:
				break;
			}
		}
		return res;
	}

	bool operator!=(const FilterPacket &rhs)
	{
		return !operator==(rhs);
	}
};

class AccountSys
{
public:
	AccountSys();
	~AccountSys() { Save(); }

	bool LogIn(const std::string &name, const std::string &password);
	bool SignUp(const std::string &name, const std::string &password, UserType utype);
	void LogOut(const unsigned uid);

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

	std::pair<std::vector<Contributor>::const_iterator, std::vector<Contributor>::const_iterator>
	get_contributor_page(int page, int user_per_page) const;

	std::pair<std::vector<Player>::const_iterator, std::vector<Player>::const_iterator>
	get_player_page(int page, int user_per_page) const;

	std::pair<std::vector<Contributor>::const_iterator, std::vector<Contributor>::const_iterator>
	get_sort_con_page(int uid, int page, int user_per_page, int sort_type);

	std::pair<std::vector<Player>::const_iterator, std::vector<Player>::const_iterator>
	get_sort_player_page(int uid, int page, int user_per_page, int sort_type);

	std::pair<std::vector<Contributor>::const_iterator, std::vector<Contributor>::const_iterator>
	get_filter_con_page(int uid, int page, int user_per_page, FilterPacket filter_packet);

	std::pair<std::vector<Player>::const_iterator, std::vector<Player>::const_iterator>
	get_filter_player_page(int uid, int page, int user_per_page, FilterPacket filter_packet);

	std::pair<std::vector<Player>::const_iterator, std::vector<Player>::const_iterator>
	SortFilterTurnPageP(int uid, int page, int user_per_page);

	std::pair<std::vector<Contributor>::const_iterator, std::vector<Contributor>::const_iterator>
	SortFilterTurnPageC(int uid, int page, int user_per_page);

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

	// const std::string &get_current_user_str() const { return current_user_str_; }
	// UserType get_current_usertype() const { return get_usertype(current_user_str_); }

	void DelAccount(const std::string &name);

	std::string get_utype_str(UserType utype) const;
	void ChangeRole(const std::string &name);

	UserType get_usertype(const std::string &name) const
	{
		if (con_map_.count(name))
			return UserType::USERTYPE_C;
		else if (player_map_.count(name))
			return UserType::USERTYPE_P;
		else
			return UserType::USERTYPE_U;
	}

	unsigned get_uid(const std::string &name) const
	{
		if (rev_uid_map_.count(name))
			return rev_uid_map_.at(name);
		else
			return -1;
	}

	std::string get_name(const unsigned uid) const
	{
		if (uid_map_.count(uid))
			return uid_map_.at(uid);
		else
			return std::string();
	}

	// DEBUG MEMBERS
	// void set_current_user()
	// {
	// 	if (players_.size())
	// 		current_user_str_ = players_.front().get_user_name();
	// 	else if (contributors_.size())
	// 		current_user_str_ = contributors_.front().get_user_name();
	// }

private:
	static const std::string account_path_;
	static unsigned uid_;

	std::vector<Contributor> contributors_;
	std::vector<Player> players_;
	std::map<std::string, size_t> con_map_;
	std::map<std::string, size_t> player_map_;
	std::map<unsigned, std::string> uid_map_;
	std::map<std::string, unsigned> rev_uid_map_;
	std::set<std::size_t> online_players_;
	std::set<std::size_t> online_contributors_;

	std::map<int, FilterPacket> previous_filter_type_;
	std::map<int, int> previous_sort_type_;
	std::map<int, std::vector<Player>> uid_player_map_;
	std::map<int, std::vector<Contributor>> uid_contributor_map_;
	int total_user_ = 0;
};

class ClientAccountSys
{
public:
	ClientAccountSys() = default;
	~ClientAccountSys() = default;

	void set_user(unsigned uid, const Player &player)
	{
		uid_ = uid;
		utype_ = USERTYPE_P;
		name_ = player.get_user_name();
		player_ = player;
	}
	void set_user(unsigned uid, const Contributor &contributor)
	{
		uid_ = uid;
		utype_ = USERTYPE_C;
		name_ = contributor.get_user_name();
		con_ = contributor;
	}
	constexpr std::size_t get_min_acc_len() { return 4; }
	constexpr std::size_t get_min_pswd_len() { return 8; }
	void set_uid(unsigned uid) { uid_ = uid; }
	void set_utype(const UserType utype) { utype_ = utype; }
	UserType get_utype() const { return utype_; }

	Player &get_player() { return player_; }

	Contributor &get_con() { return con_; }

	unsigned get_uid() const { return uid_; }

	void set_player_vec(std::vector<Player>::const_iterator b, std::vector<Player>::const_iterator e);
	void set_contributor_vec(std::vector<Contributor>::const_iterator b, std::vector<Contributor>::const_iterator e);

	std::vector<Player> &get_player_vec() { return player_vec_; }
	std::vector<Contributor> &get_con_vec() { return con_vec_; }
	std::string get_utype_str(UserType utype) const
	{
		return (utype == USERTYPE_P) ? "Player" : "Contributor";
	}

	void change_user_type() 
	{
		if (utype_ == USERTYPE_P)
		{
			con_.from_player(player_);
		}
		else
		{
			player_.from_contributor(con_);
		}
		utype_ = (utype_ == USERTYPE_P) ? USERTYPE_C : USERTYPE_P;
	}

private:
	static unsigned uid_;
	UserType utype_;
	std::string name_;

	Contributor con_;
	Player player_;

	std::vector<Player> player_vec_;
	std::vector<Contributor> con_vec_;
};