#pragma once

#include <string>
#include <vector>

class Contributor;
class Player;

class AccountSys
{
public:
    AccountSys();

    bool LogIn(const std::string &name, const std::string &password);
    bool SignUp(const std::string &name, const std::string &password, int type);

    Contributor get_contributor(const std::string &name) const;
    Player get_player(const std::string &name) const;

    std::vector<Contributor> get_contributors(int quantity = -1) const;
    std::vector<Player> get_players(int quantity = -1) const;

    bool Save(const Contributor &user) const;
    bool Save(const Player &user) const;

private:
    static const std::string account_path_;
    std::vector<Contributor> online_contributors_;
    std::vector<Player> online_players_;
    int total_user;
    std::vector<int> user_pos;
};