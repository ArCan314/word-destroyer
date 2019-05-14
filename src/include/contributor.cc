#include <string>
#include <fstream>

#include "user.h"
#include "contributor.h"
#include "log.h"

Contributor::Contributor(std::ifstream &ifs) : User(ifs), word_contributed_(int(0))
{
    if (Load(ifs))
        Log::WriteLog("Load contributor successfully");
    else
        Log::WriteLog("Failed to load contributor");
}

bool Contributor::Save(std::ofstream &ofs) const 
{
    User::Save(ofs);
    ofs.write(reinterpret_cast<const char *>(&word_contributed_), sizeof(word_contributed_));

    return ofs.fail();
}

bool Contributor::Load(std::ifstream &ifs)
{
    ifs.read(reinterpret_cast<char *>(&word_contributed_), sizeof(word_contributed_));

    return ifs.fail();
}
