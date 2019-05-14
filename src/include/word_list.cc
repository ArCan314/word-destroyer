#include <string>
#include <vector>
#include <random>
#include <fstream>
#include <memory>
#include <utility>
#include <cctype>

#include "word_list.h"
#include "log.h"

const std::string WordList::word_path_ = "../../data/words.dat";
static const std::size_t kBufSize = 2048;

WordList::WordList()
{
    if (!Load())
        Log::WriteLog("Failed to load wordlist");
    else
        Log::WriteLog("Successfully load wordlist");
}

bool WordList::AddWord(const std::string &new_word, const std::string &name)
{
    if (word_set_.count(new_word))
    {
        Log::WriteLog(new_word + " is already in the word list");
        return false;
    }
    else
    {
        is_updated = true;
        word_set_.insert(new_word);
        word_vec_.push_back({new_word, name, 0, 0, static_cast<unsigned char>(get_difficulty(new_word))});
        return true;
    }
}

const Word &WordList::get_word(int difficulty)
{
    std::vector<int> temp;
    static std::random_device r;

    for (std::size_t i = 0; i < word_vec_.size(); i++)
    {
        if (word_vec_[i].difficulty == difficulty)
            temp.push_back(i);
    }

    std::uniform_int_distribution<int> uni_rd(0, temp.size());
    return word_vec_.at(temp[uni_rd(r)]);
}

WordList::~WordList()
{
    if (Save())
        Log::WriteLog("Successfully saved wordlist");
    else
        Log::WriteLog("Failed to save wordlist");
}

bool WordList::Save()
{
    if (is_updated)
    {
        std::ofstream ofs;
        ofs.open(word_path_, std::ofstream::binary);
        if (ofs)
        {
            static char buffer[kBufSize];
            std::size_t buf_cur = 0;
            for (const auto &word : word_vec_)
            {
                if (kBufSize - buf_cur < 256)
                {
                    ofs.write(buffer, buf_cur);
                    buf_cur = 0;
                }
                Serialize(word, buffer, buf_cur);
            }
            if (buf_cur)
                ofs.write(buffer, buf_cur);
            return true;
        }
        else
        {
            Log::WriteLog("Failed to save wordlist: cannot open the file");
            return false;
        }
    }
    else
    {
        Log::WriteLog("Wordlist is not changed, stop saving");
        return true;
    }
}

void WordList::Serialize(const Word &warp, char *buf, std::size_t &write_len)
{
    WordSerializationWarp temp{0, static_cast<unsigned char>(warp.word.size()), nullptr, static_cast<unsigned char>(warp.contributor.size()), nullptr, warp.total, warp.correct, warp.difficulty, 0};
    temp.word = new char[temp.word_len];
    temp.name = new char[temp.name_len];

    for (std::size_t i = 0; i < temp.word_len; i++)
        temp.word[i] = warp.word[i];

    for (std::size_t i = 0; i < temp.name_len; i++)
        temp.name[i] = warp.contributor[i];

    temp.len = sizeof(unsigned char) * 4 + sizeof(unsigned short) * 2 + temp.word_len + temp.name_len;

    get_chksum(temp);

    std::memcpy(buf + write_len , &temp , sizeof(temp.len) + sizeof(temp.word_len) );
    write_len += sizeof(temp.len) + sizeof(temp.word_len);

    std::memcpy(buf + write_len , temp.word , temp.word_len );
    write_len += temp.word_len;

    std::memcpy(buf + write_len, &temp.name_len, sizeof(temp.name_len));
    write_len += sizeof(temp.name_len);

    std::memcpy(buf + write_len, temp.name, temp.name_len);
    write_len += temp.name_len;

    std::memcpy(buf + write_len, &temp.total, sizeof(unsigned short) * 2 + sizeof(unsigned char) * 2);
    write_len += sizeof(unsigned short) * 2 + sizeof(unsigned char) * 2;

    delete[] temp.word;
    delete[] temp.name;
}

void WordList::get_chksum(WordSerializationWarp &warp)
{
    unsigned char chksum = 0;
    unsigned char *byte_ptr = reinterpret_cast<unsigned char *>(&warp);
    for (std::size_t i = 0; i < static_cast<std::size_t>(warp.len); i++)
    {
        chksum += *byte_ptr;
        byte_ptr++;
    }
    warp.chksum = chksum;
}

int WordList::get_difficulty(const std::string &word)
{
    bool is_upper = std::isupper(word.front());
    int len = word.length();
    //
    return 0;
}

bool WordList::Load()
{
	std::ofstream ofs_create_file(word_path_, std::ofstream::binary | std::ofstream::app);	// create file if not exist

    std::ifstream ifs(word_path_, std::ifstream::binary);
    if (ifs)
    {
        Word temp;
        while (ifs)
        {
            temp = Deserialize(ifs);
            word_vec_.push_back(temp);
            word_set_.insert(std::move(temp.word));
        }
        last_origin_ = word_vec_.size();

		return true;
    }
    else
    {
        Log::WriteLog("Failed to load wordlist : cannot open the file");
        return false;
    }
}

Word WordList::Deserialize(std::ifstream &ifs)
{
    WordSerializationWarp temp{0, 0, nullptr, 0, nullptr, 0, 0, 0, 0};
    ifs.read(reinterpret_cast<char *>(&temp.len), sizeof(temp.len) + sizeof(temp.word_len));

    temp.word = new char[temp.word_len];
    ifs.read(temp.word, temp.word_len + sizeof(temp.name_len));

    temp.name = new char[temp.name_len];
    ifs.read(temp.name, temp.name_len);

    ifs.read(reinterpret_cast<char *>(&temp.total), sizeof(unsigned char) * 2 + sizeof(unsigned short) * 2);

    unsigned char now_chksum = temp.chksum;
    get_chksum(temp);

    if (now_chksum != temp.chksum)
    {
        Log::WriteLog("Loading wordlist : checksum error");
        delete[] temp.word;
        delete[] temp.name;
        temp.len = 0;
        return {"", "", 0, 0, 0};
    }
    else
    {
        Word res{"", "", temp.total, temp.correct, temp.diff};
        for (std::size_t i = 0; i < temp.word_len; i++)
            res.word.push_back(temp.word[i]);

        for (std::size_t i = 0; i < temp.name_len; i++)
            res.contributor.push_back(temp.name[i]);

        delete[] temp.word;
        delete[] temp.name;
        return res;
    }
}