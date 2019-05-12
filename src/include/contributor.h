#pragma once

#include "user.h"

class Contributor : public User
{
public:
    Contributor() ;
    bool Save(std::ofstream &ofs) override;
    bool Load(std::ifstream &ifs) override;

    

private:
    int word_contributed_;

};