#include <string>
#include <fstream>

#include "user.h"
#include "contributor.h"
#include "player.h"
#include "log.h"
#include "game.h"

Contributor::Contributor(std::ifstream &ifs) : User(ifs), word_contributed_(int(0))
{
    if (Load(ifs))
        Log::WriteLog("Load contributor successfully");
    else
        Log::WriteLog("Failed to load contributor");
}

void Contributor::from_player(const Player &player)
{
	User::operator=(player);
	user_type_ = UserType::USERTYPE_C;
	word_contributed_ = 0;
}

bool Contributor::Save(std::ofstream &ofs) const
{
    User::Save(ofs);
    ofs.write(reinterpret_cast<const char *>(&word_contributed_), sizeof(word_contributed_));

    return ofs.fail();
}

bool Contributor::Load(std::ifstream &ifs)
{
	User::Load(ifs);
    ifs.read(reinterpret_cast<char *>(&word_contributed_), sizeof(word_contributed_));

    return !ifs.fail();
}

void Contributor::inc_level()
{
	if (word_contributed_ > get_level_up_bound(level_, USERTYPE_C))
		level_++;
}
