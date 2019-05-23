#include <string>
#include <vector>
#include <fstream>
#include <set>
#include <algorithm>
#include <iterator>
#include <utility>

#include "account_sys.h"
#include "player.h"
#include "contributor.h"
#include "log.h"

const std::string AccountSys::account_path_ = "../data/acnt.dat";

unsigned AccountSys::uid_ = 0;
unsigned ClientAccountSys::uid_ = 0;

AccountSys::AccountSys()
{
	std::ofstream ofs_create_file(account_path_, std::fstream::app | std::fstream::binary); // create account file if not exist

	std::ifstream ifs(account_path_, std::fstream::binary);
	if (ifs)
	{
		char utype;
		Contributor temp_c;
		Player temp_p;

		while (ifs)
		{

			ifs.read(&utype, sizeof(utype));
			if (ifs)
			{
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
		}
		ifs.clear();
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
			uid_map_[uid_] = name;
			rev_uid_map_[name] = uid_;
			uid_++;
			// current_user_str_ = name;
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
		// current_user_str_ = name;
		uid_map_[uid_] = name;
		rev_uid_map_[name] = uid_;
		uid_++;
		total_user_++;
		Log::WriteLog(std::string("AccSys: ") + name + " sign up.");
		return true;
	}
}

void AccountSys::LogOut(const unsigned uid)
{
	std::string name = get_name(uid);
	Log::WriteLog(std::string("AccSys: ") + name + " log out.");
	if (uid_map_.count(uid))
		uid_map_.erase(uid);
	if (rev_uid_map_.count(name))
		rev_uid_map_.erase(name);
	if (previous_filter_type_.count(uid))
		previous_filter_type_.erase(uid);
	if (previous_sort_type_.count(uid))
		previous_sort_type_.erase(uid);

	if (get_usertype(name) == USERTYPE_P)
	{
		online_players_.erase(player_map_.at(name));
		if (uid_player_map_.count(uid))
			uid_player_map_.erase(uid);

	}
	else
	{
		online_contributors_.erase(con_map_.at(name));
		if (uid_contributor_map_.count(uid))
			uid_contributor_map_.erase(uid);
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
	Log::WriteLog(std::string("AccSys: ") + "Saving");
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

std::string AccountSys::get_utype_str(UserType utype) const
{
	if (utype == UserType::USERTYPE_C)
		return std::string("Contributor");
	else if (utype == UserType::USERTYPE_P)
		return std::string("Player");
	else
		return std::string("User");
}

void AccountSys::ChangeRole(const std::string &name)
{
	Log::WriteLog(std::string("AccSys: ") + name + " change role.");
	UserType utype = get_usertype(name);

	static const char *role_str[] = {"Contributor", "Player", ""};
	Log::WriteLog(std::string("AccSys: ") + name + " changes role from " + role_str[utype] + " to " + role_str[(utype == USERTYPE_C) ? USERTYPE_P : USERTYPE_C]);

	std::size_t pos;
	Player temp_p;
	Contributor temp_c;
	switch (utype)
	{
	case UserType::USERTYPE_C:
	{
		pos = con_map_.at(name);
		temp_c = contributors_.at(pos);
		temp_p.from_contributor(temp_c);
		contributors_.erase(contributors_.begin() + pos);
		con_map_.erase(name);
		players_.push_back(temp_p);
		player_map_[temp_p.get_user_name()] = players_.size() - 1;
	}
	break;
	case UserType::USERTYPE_P:
	{
		pos = player_map_.at(name);
		temp_p = players_.at(pos);
		temp_c.from_player(temp_p);
		players_.erase(players_.begin() + pos);
		con_map_.erase(name);
		contributors_.push_back(temp_c);
		con_map_[temp_c.get_user_name()] = contributors_.size() - 1;
	}
	break;
	default:
		break;
	}
}

std::pair<std::vector<Contributor>::const_iterator, std::vector<Contributor>::const_iterator>
AccountSys::get_contributor_page(int page, int user_per_page) const
{
	std::vector<Contributor>::const_iterator b, e;

	if (contributors_.size() <= (static_cast<std::size_t>(page))*user_per_page)
	{
		if (contributors_.size() > (static_cast<std::size_t>(page) - 1) * user_per_page)
		{
			b = contributors_.cbegin() + (static_cast<std::size_t>(page) - 1) * user_per_page;
			e = contributors_.cend();
		}
		else
		{
			b = e = contributors_.cbegin();
		}
	}
	else
	{
		b = contributors_.cbegin() + (static_cast<std::size_t>(page) - 1) * user_per_page;
		e = contributors_.cbegin() + (static_cast<std::size_t>(page))*user_per_page;
	}
	return {b, e};
}

std::pair<std::vector<Player>::const_iterator, std::vector<Player>::const_iterator>
AccountSys::get_player_page(int page, int user_per_page) const
{
	std::vector<Player>::const_iterator b, e;

	if (players_.size() <= (static_cast<std::size_t>(page))*user_per_page)
	{
		if (players_.size() > (static_cast<std::size_t>(page) - 1) * user_per_page)
		{
			b = players_.cbegin() + (static_cast<std::size_t>(page) - 1) * user_per_page;
			e = players_.cend();
		}
		else
		{
			b = e = players_.cbegin();
		}
	}
	else
	{
		b = players_.cbegin() + (static_cast<std::size_t>(page) - 1) * user_per_page;
		e = players_.cbegin() + (static_cast<std::size_t>(page))*user_per_page;
	}
	return {b, e};
}

enum SortSelection
{
	SORTS_NAME = 0,
	SORTS_LEVEL,
	SORTS_EXP,
	SORTS_PASS,
	SORTS_CON
};

std::pair<std::vector<Contributor>::const_iterator, std::vector<Contributor>::const_iterator>
AccountSys::get_sort_con_page(int uid, int page, int user_per_page, int sort_type)
{
	previous_filter_type_[uid] = FilterPacket();
	previous_filter_type_[uid].filter_type = -1;
	if (!previous_sort_type_.count(uid))
	{
		previous_sort_type_[uid] = -1;
	}

	if (previous_sort_type_[uid] != sort_type)
	{
		previous_sort_type_[uid] = sort_type;
		std::vector<Contributor> con_vec;
		std::vector<Contributor> *con_vec_p;

		std::copy(contributors_.begin(), contributors_.end(), std::back_insert_iterator<std::vector<Contributor>>(con_vec));
		uid_contributor_map_[uid] = std::move(con_vec);
		con_vec_p = &uid_contributor_map_.at(uid);

		switch (sort_type)
		{
		case SORTS_NAME:
			std::sort(con_vec_p->begin(), con_vec_p->end(), [](const Contributor &a, const Contributor &b) { return a.get_user_name() < b.get_user_name(); });
			break;
		case SORTS_LEVEL:
			std::sort(con_vec_p->rbegin(), con_vec_p->rend(), [](const Contributor &a, const Contributor &b) { return a.get_level() < b.get_level(); });
			break;
		case SORTS_CON:
			std::sort(con_vec_p->rbegin(), con_vec_p->rend(), [](const Contributor &a, const Contributor &b) { return a.get_word_contributed() < b.get_word_contributed(); });
			break;
		default:
			break;
		}
	}

	std::vector<Contributor>::const_iterator b, e;
	std::vector<Contributor> &vec = uid_contributor_map_.at(uid);
	if (vec.size() <= (static_cast<std::size_t>(page))*user_per_page)
	{
		if (vec.size() > (static_cast<std::size_t>(page) - 1) * user_per_page)
		{
			b = vec.cbegin() + (static_cast<std::size_t>(page) - 1) * user_per_page;
			e = vec.cend();
		}
		else
		{
			b = e = vec.cbegin();
		}
	}
	else
	{
		b = vec.cbegin() + (static_cast<std::size_t>(page) - 1) * user_per_page;
		e = vec.cbegin() + (static_cast<std::size_t>(page))*user_per_page;
	}
	return {b, e};
}

std::pair<std::vector<Player>::const_iterator, std::vector<Player>::const_iterator>
AccountSys::get_sort_player_page(int uid, int page, int user_per_page, int sort_type)
{
	previous_filter_type_[uid] = FilterPacket();
	previous_filter_type_[uid].filter_type = -1;
	if (!previous_sort_type_.count(uid))
	{
		previous_sort_type_[uid] = -1;
	}

	if (previous_sort_type_[uid] != sort_type)
	{
		previous_sort_type_[uid] = sort_type;
		std::vector<Player> player_vec;
		std::vector<Player> *player_map_p;

		std::copy(players_.begin(), players_.end(), std::back_insert_iterator<std::vector<Player>>(player_vec));
		uid_player_map_[uid] = std::move(player_vec);
		player_map_p = &uid_player_map_.at(uid);

		switch (sort_type)
		{
		case SORTS_NAME:
			std::sort(player_map_p->begin(), player_map_p->end(), [](const Player &a, const Player &b) { return a.get_user_name() < b.get_user_name(); });
			break;
		case SORTS_LEVEL:
			std::sort(player_map_p->rbegin(), player_map_p->rend(), [](const Player &a, const Player &b) { return a.get_level() < b.get_level(); });
			break;
		case SORTS_EXP:
			std::sort(player_map_p->rbegin(), player_map_p->rend(), [](const Player &a, const Player &b) { return a.get_exp() < b.get_exp(); });
			break;
		case SORTS_PASS:
			std::sort(player_map_p->rbegin(), player_map_p->rend(), [](const Player &a, const Player &b) { return a.get_level_passed() < b.get_level_passed(); });
			break;
		default:
			break;
		}
	}

	std::vector<Player>::const_iterator b, e;
	std::vector<Player> &vec = uid_player_map_.at(uid);
	if (vec.size() <= (static_cast<std::size_t>(page))*user_per_page)
	{
		if (vec.size() > (static_cast<std::size_t>(page) - 1) * user_per_page)
		{
			b = vec.cbegin() + (static_cast<std::size_t>(page) - 1) * user_per_page;
			e = vec.cend();
		}
		else
		{
			b = e = vec.cbegin();
		}
	}
	else
	{
		b = vec.cbegin() + (static_cast<std::size_t>(page) - 1) * user_per_page;
		e = vec.cbegin() + (static_cast<std::size_t>(page))*user_per_page;
	}
	return {b, e};
}

std::pair<std::vector<Contributor>::const_iterator, std::vector<Contributor>::const_iterator>
AccountSys::get_filter_con_page(int uid, int page, int user_per_page, FilterPacket filter_packet)
{
	if (!previous_filter_type_.count(uid) || filter_packet != previous_filter_type_.at(uid))
	{
		previous_sort_type_[uid] = -1;
		previous_filter_type_[uid] = filter_packet;
		uid_contributor_map_[uid] = std::vector<Contributor>();
		std::vector<Contributor> &temp = uid_contributor_map_.at(uid);
		switch (filter_packet.filter_type)
		{
		case FPT_NAME:
			for (auto i = contributors_.cbegin(); i != contributors_.cend(); ++i)
				if ((*i).get_user_name() == filter_packet.val_str)
					temp.push_back(*i);
			break;
		case FPT_LV:
			for (auto i = contributors_.cbegin(); i != contributors_.cend(); ++i)
				if ((*i).get_level() == filter_packet.val_32)
					temp.push_back(*i);
			break;
		case FPT_CON:
			for (auto i = contributors_.cbegin(); i != contributors_.cend(); ++i)
				if ((*i).get_word_contributed() == filter_packet.val_32)
					temp.push_back(*i);
			break;
		default:
			break;
		}
	}
	std::vector<Contributor> &con_vec = uid_contributor_map_.at(uid);
	std::vector<Contributor>::const_iterator b, e;
	if (con_vec.size() <= (static_cast<std::size_t>(page))*user_per_page)
	{
		if (con_vec.size() > (static_cast<std::size_t>(page) - 1) * user_per_page)
		{
			b = con_vec.cbegin() + (static_cast<std::size_t>(page) - 1) * user_per_page;
			e = con_vec.cend();
		}
		else
		{
			b = e = con_vec.cbegin();
		}
	}
	else
	{
		b = con_vec.cbegin() + (static_cast<std::size_t>(page) - 1) * user_per_page;
		e = con_vec.cbegin() + (static_cast<std::size_t>(page))*user_per_page;
	}
	return {b, e};
}

std::pair<std::vector<Player>::const_iterator, std::vector<Player>::const_iterator>
AccountSys::get_filter_player_page(int uid, int page, int user_per_page, FilterPacket filter_packet)
{
	if (!previous_filter_type_.count(uid) || filter_packet != previous_filter_type_.at(uid))
	{
		previous_sort_type_[uid] = -1;
		previous_filter_type_[uid] = filter_packet;
		uid_player_map_[uid] = std::vector<Player>();
		std::vector<Player> &temp = uid_player_map_.at(uid);
		switch (filter_packet.filter_type)
		{
		case FPT_NAME:
			for (auto i = players_.cbegin(); i != players_.cend(); ++i)
				if ((*i).get_user_name() == filter_packet.val_str)
					temp.push_back(*i);
			break;
		case FPT_LV:
			for (auto i = players_.cbegin(); i != players_.cend(); ++i)
				if ((*i).get_level() == filter_packet.val_32)
					temp.push_back(*i);
			break;
		case FPT_EXP:
			for (auto i = players_.cbegin(); i != players_.cend(); ++i)
				if ((*i).get_exp() == filter_packet.val_double)
					temp.push_back(*i);
			break;
		case FPT_PASS:
			for (auto i = players_.cbegin(); i != players_.cend(); ++i)
				if ((*i).get_level_passed() == filter_packet.val_32)
					temp.push_back(*i);
			break;
		default:
			break;
		}
	}

	std::vector<Player> &player_vec = uid_player_map_.at(uid);
	std::vector<Player>::const_iterator b, e;
	if (player_vec.size() <= (static_cast<std::size_t>(page))*user_per_page)
	{
		if (player_vec.size() > (static_cast<std::size_t>(page) - 1) * user_per_page)
		{
			b = player_vec.cbegin() + (static_cast<std::size_t>(page) - 1) * user_per_page;
			e = player_vec.cend();
		}
		else
		{
			b = e = player_vec.cbegin();
		}
	}
	else
	{
		b = player_vec.cbegin() + (static_cast<std::size_t>(page) - 1) * user_per_page;
		e = player_vec.cbegin() + (static_cast<std::size_t>(page))*user_per_page;
	}
	return {b, e};
}

std::pair<std::vector<Player>::const_iterator, std::vector<Player>::const_iterator>
AccountSys::SortFilterTurnPageP(int uid, int page, int user_per_page)
{
	std::vector<Player>::const_iterator b, e;

	if (uid_player_map_.at(uid).size() <= (static_cast<std::size_t>(page))*user_per_page)
	{
		if (uid_player_map_.at(uid).size() > (static_cast<std::size_t>(page) - 1) * user_per_page)
		{
			b = uid_player_map_.at(uid).cbegin() + (static_cast<std::size_t>(page) - 1) * user_per_page;
			e = uid_player_map_.at(uid).cend();
		}
		else
		{
			b = e = uid_player_map_.at(uid).cbegin();
		}
	}
	else
	{
		b = uid_player_map_.at(uid).cbegin() + (static_cast<std::size_t>(page) - 1) * user_per_page;
		e = uid_player_map_.at(uid).cbegin() + (static_cast<std::size_t>(page))*user_per_page;
	}

	return {b, e};
}

std::pair<std::vector<Contributor>::const_iterator, std::vector<Contributor>::const_iterator>
AccountSys::SortFilterTurnPageC(int uid, int page, int user_per_page)
{
	std::vector<Contributor>::const_iterator b, e;

	if (uid_contributor_map_.at(uid).size() <= (static_cast<std::size_t>(page))*user_per_page)
	{
		if (uid_contributor_map_.at(uid).size() > (static_cast<std::size_t>(page) - 1) * user_per_page)
		{
			b = uid_contributor_map_.at(uid).cbegin() + (static_cast<std::size_t>(page) - 1) * user_per_page;
			e = uid_contributor_map_.at(uid).cend();
		}
		else
		{
			b = e = uid_contributor_map_.at(uid).cbegin();
		}
	}
	else
	{
		b = uid_contributor_map_.at(uid).cbegin() + (static_cast<std::size_t>(page) - 1) * user_per_page;
		e = uid_contributor_map_.at(uid).cbegin() + (static_cast<std::size_t>(page))*user_per_page;
	}
	return {b, e};
}

void ClientAccountSys::set_player_vec(std::vector<Player>::const_iterator b, std::vector<Player>::const_iterator e)
{
	player_vec_.clear();
	std::copy(b, e, std::back_insert_iterator<std::vector<Player>>(player_vec_));
}

void ClientAccountSys::set_contributor_vec(std::vector<Contributor>::const_iterator b, std::vector<Contributor>::const_iterator e)
{
	con_vec_.clear();
	std::copy(b, e, std::back_insert_iterator<std::vector<Contributor>>(con_vec_));
}