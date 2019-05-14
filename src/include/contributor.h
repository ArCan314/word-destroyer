#pragma once

#include <string>
#include <fstream>

#include "user.h"

class Contributor : public User
{
public:
    Contributor() = default;
    Contributor(std::ifstream &ifs);
    Contributor(const std::string name, const std::string pswd, UserType utype)
        : User(name, pswd, utype), word_contributed_(int(0)) {}

    void inc_word_contributed() { word_contributed_++; }
    int get_word_contributed() const { return word_contributed_; }

    bool Save(std::ofstream &ofs) const override;
    bool Load(std::ifstream &ifs) override;

    ~Contributor() = default;

private:
    int word_contributed_ = 0;

};