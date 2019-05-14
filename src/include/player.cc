#include <fstream>
#include <string>

#include "user.h"
#include "player.h"
#include "log.h"

Player::Player(std::ifstream &ifs) : User(ifs), exp_(double(0.0)), level_passed_(int(0))
{
    if (Load(ifs))
        Log::WriteLog("Loading player Succeed");
    else
        Log::WriteLog("Failed to load player");
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
    ifs.read(reinterpret_cast<char *>(&this->exp_), sizeof(exp_));
    ifs.read(reinterpret_cast<char *>(&this->level_passed_), sizeof(level_passed_));

    return ifs.fail();
}