#pragma once

#include "user.h"

class Player : public User
{
public:
    Player();
    bool Save(std::ofstream &ofs) override;
    bool Load(std::ifstream &ifs) override;

private:
    double exp_;
    int level_passed;
};