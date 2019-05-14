#pragma once

#include <fstream>
#include <string>

#include "user.h"

class Player : public User
{
public:
    Player() = default;
    Player(std::ifstream &ifs);
    Player(const std::string &name, const std::string &pswd, UserType utype)
        : User(name, pswd, utype), exp_(double(0)), level_passed_(int(0)) {}

    void raise_exp_(double increasement) { exp_ += increasement; }
    void inc_level_passed() { level_passed_++; }

    double get_exp() const { return exp_; }
    int get_level_passed() const { return level_passed_; }

    bool Save(std::ofstream &ofs) const override;
    bool Load(std::ifstream &ifs) override;

    ~Player() = default;

private:
    double exp_ = 0.0;
    int level_passed_ = 0;
};