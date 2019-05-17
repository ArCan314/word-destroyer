#include <fstream>
#include <string>
#include <utility>

#include "user.h"
#include "player.h"
#include "contributor.h"
#include "log.h"
#include "game.h"

Player::Player(std::ifstream &ifs) : User(ifs), exp_(double(0.0)), level_passed_(int(0))
{
    if (Load(ifs))
        Log::WriteLog("Loading player Succeed");
    else
        Log::WriteLog("Failed to load player");
}

void Player::from_contributor(const Contributor &con)
{
	User::operator=(con);
	user_type_ = UserType::USERTYPE_P;
	exp_ = 0.0;
	level_passed_ = 0;
	level_ = 0;
}

bool Player::Save(std::ofstream &ofs) const
{
    User::Save(ofs);
    ofs.write(reinterpret_cast<const char *>(&this->exp_), sizeof(exp_));
    ofs.write(reinterpret_cast<const char *>(&this->level_passed_), sizeof(level_passed_));

    return ofs.fail();
}

bool Player::Load(std::ifstream &ifs)
{
	User::Load(ifs);
    ifs.read(reinterpret_cast<char *>(&this->exp_), sizeof(exp_));
    ifs.read(reinterpret_cast<char *>(&this->level_passed_), sizeof(level_passed_));

    return !ifs.fail();
}

void Player::inc_level()
{
	for (int i = 0; i < get_max_level(); i++)
		if (exp_ < get_level_up_bound(i, USERTYPE_P))
		{
			level_ = i;
			break;
		}
}
