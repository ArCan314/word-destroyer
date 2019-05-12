#pragma once

#include <string>
#include <fstream>

class User
{
public:
    User() = default;
    User(std::ifstream &ifs);

    virtual bool Save(std::ofstream &ofs);
    virtual bool Load(std::ifstream &ifs);

    virtual ~User();
protected:
    std::string name_;
    std::string pswd_;
    unsigned level_ = 0;
private:

};