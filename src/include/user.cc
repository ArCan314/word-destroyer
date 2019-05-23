#include <fstream>
#include <string>
#include <utility>

#include "user.h"
#include "log.h"

struct UserSerializationWarp
{
    unsigned char name_len;
    char *name;
    unsigned char pswd_len;
    char *pswd;
    unsigned level;
};

static void EncryptPswd(UserSerializationWarp &warp)
{
    unsigned char cip = (warp.name_len + warp.pswd_len + warp.level) & 0x3;
    cip = cip ? cip : 0x2;
    for (int i = 0; i < 4; i++)
        cip |= cip << 2;
    for (unsigned i = 0; i < static_cast<unsigned>(warp.pswd_len); i++)
        warp.pswd[i] ^= cip;
}

static void DecryptPswd(UserSerializationWarp &warp)
{
    unsigned char cip = (warp.name_len + warp.pswd_len + warp.level) & 0x3;
    cip = cip ? cip : 0x2;
    for (int i = 0; i < 4; i++)
        cip |= cip << 2;
    for (unsigned i = 0; i < static_cast<unsigned>(warp.pswd_len); i++)
        warp.pswd[i] ^= cip;
}

User::User(std::ifstream &ifs)
{
	if (Load(ifs))
		; // Log::WriteLog("");
	else
		; // Log::WriteLog("");
}

bool User::Save(std::ofstream &ofs) const
{
    UserSerializationWarp temp{name_.size(), nullptr, pswd_.size(), nullptr, level_};

    temp.name = new char[temp.name_len];
    temp.pswd = new char[temp.pswd_len];

    for (std::size_t i = 0; i < temp.name_len; i++)
        temp.name[i] = name_[i];

    for (std::size_t i = 0; i < temp.pswd_len; i++)
        temp.pswd[i] = pswd_[i];

    EncryptPswd(temp);
    char utype = static_cast<char>(user_type_);
    ofs.write(&utype, sizeof(utype));
    ofs.write(reinterpret_cast<char *>(&temp), sizeof(temp.name_len));
    ofs.write(temp.name, temp.name_len);
    ofs.write(reinterpret_cast<char *>(&temp.pswd_len), sizeof(temp.pswd_len));
    ofs.write(temp.pswd, temp.pswd_len);
    ofs.write(reinterpret_cast<char *>(&temp.level), sizeof(temp.level));

    delete[] temp.name;
    delete[] temp.pswd;

    if (ofs)
        return true;
    else
        return false;
}

bool User::Load(std::ifstream &ifs)
{
    UserSerializationWarp temp{0, nullptr, 0, nullptr, 0};

    ifs.read(reinterpret_cast<char *>(&temp.name_len), sizeof(temp.name_len));

    temp.name = new char[temp.name_len];
    ifs.read(temp.name, temp.name_len);
    ifs.read(reinterpret_cast<char *>(&temp.pswd_len), sizeof(temp.pswd_len));
    temp.pswd = new char[temp.pswd_len];
    ifs.read(temp.pswd, temp.pswd_len);
    ifs.read(reinterpret_cast<char *>(&temp.level), sizeof(temp.level));

    DecryptPswd(temp);

	name_.clear();
	pswd_.clear();

    for (unsigned i = 0; i < temp.name_len; i++)
        name_.push_back(temp.name[i]);

    for (unsigned i = 0; i < temp.pswd_len; i++)
        pswd_.push_back(temp.pswd[i]);
	level_ = temp.level;

    delete[] temp.name;
    delete[] temp.pswd;

    if (ifs)
        return true;
    else
        return false;
}