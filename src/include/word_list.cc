#include <string>
#include <vector>
#include <random>
#include <fstream>
#include <memory>
#include <utility>
#include <cctype>

#include "word_list.h"
#include "log.h"
#include "game.h"

const std::string WordList::word_path_ = "../data/words.dat";
static const std::size_t kBufSize = 2048;

WordList::WordList()
{
    if (!Load())
        Log::WriteLog("WordList: Failed to load wordlist");
    else
        Log::WriteLog("WordList: Successfully load wordlist");
}

bool WordList::AddWord(const std::string &new_word, const std::string &name)
{
    if (word_set_.count(new_word))
    {
        Log::WriteLog(std::string("WordList: ") + new_word + " is already in the word list");
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

const Word &WordList::get_word(int level)
{
    static std::random_device r;
	int max_stage = get_max_stage();
	int increasement = word_set_.size() / max_stage;
	std::uniform_int_distribution<int> uni_rd((level - 1) * increasement, increasement * level);
	return word_vec_.at(uni_rd(r));
	/*
	if (level <= 60)
	{
		std::uniform_int_distribution<int> uni_rd((level - 1) * 100, 100 * level);
		return word_vec_.at(uni_rd(r));
	}
	else
	{
		std::uniform_int_distribution<int> uni_rd(3000 + 50 * (level - 1), 3000 + 50 * level);
		return word_vec_.at(uni_rd(r));
	}
	*/
}

WordList::~WordList()
{
    if (Save())
        Log::WriteLog("WordList: Successfully saved wordlist");
    else
        Log::WriteLog("WordList: Failed to save wordlist");
}

bool WordList::Save()
{
    if (is_updated)
    {
        std::ofstream ofs(word_path_, std::ofstream::binary);
        if (ofs)
        {
            for (const auto &word : word_vec_)
            {
                auto temp = Serialize(word);
                ofs.write(reinterpret_cast<char *>(&temp.len), sizeof(temp.len));
                ofs.write(reinterpret_cast<char *>(&temp.word_len), sizeof(temp.word_len));
                ofs.write(temp.word.get(), temp.word_len);
                ofs.write(reinterpret_cast<char *>(&temp.name_len), sizeof(temp.name_len));
                ofs.write(temp.name.get(), temp.name_len);
                ofs.write(reinterpret_cast<char *>(&temp.total), sizeof(temp.total));
                ofs.write(reinterpret_cast<char *>(&temp.correct), sizeof(temp.correct));
                ofs.write(reinterpret_cast<char *>(&temp.diff), sizeof(temp.diff));
                ofs.write(reinterpret_cast<char *>(&temp.chksum), sizeof(temp.chksum));
            }
            return true;
        }
        else
        {
            Log::WriteLog("WordList: Failed to save wordlist: cannot open the file");
            return false;
        }
    }
    else
    {
        Log::WriteLog("WordList: Wordlist is not changed, stop saving");
        return true;
    }
}

WordSerializationWarp WordList::Serialize(const Word &warp)
{
    WordSerializationWarp temp{0, static_cast<unsigned char>(warp.word.size()), nullptr, static_cast<unsigned char>(warp.contributor.size()), nullptr, warp.total, warp.correct, warp.difficulty, 0};
    temp.word = std::make_unique<char[]>(temp.word_len);
    temp.name = std::make_unique<char[]>(temp.name_len);

    for (std::size_t i = 0; i < temp.word_len; i++)
        temp.word[i] = warp.word[i];

    for (std::size_t i = 0; i < temp.name_len; i++)
        temp.name[i] = warp.contributor[i];

    temp.len = sizeof(unsigned char) * 4 + sizeof(unsigned short) * 2 + temp.word_len + temp.name_len;

    get_chksum(temp);
    return temp;
}

void WordList::get_chksum(WordSerializationWarp &warp)
{
    unsigned char chksum = 0;
	chksum = warp.len + warp.word_len + warp.name_len + warp.total + warp.correct + warp.diff;
	// don't release byte
	char *byte = warp.word.get();
	for (int i = 0; i < warp.word_len; i++)
	{
		chksum += *byte;
		byte++;
	}
	byte = warp.name.get();
	for (int i = 0; i < warp.name_len; i++)
	{
		chksum += *byte;
		byte++;
	}
    warp.chksum = chksum;
}

int WordList::get_difficulty(const std::string &word)
{
    bool is_upper = std::isupper(word.front());
	int upper = 0;
	int lower = 0;
    int len = word.length();
	int change_time = 0;
	for (auto c : word)
	{
		if (std::isupper(c))
		{
			if (!is_upper)
			{
				change_time++;
				is_upper = true;
			}
			upper++;
		}
		else
		{
			if (is_upper)
			{
				change_time++;
				is_upper = false;
			}
			lower++;
		}
	}

	double difficulty = len + change_time + (upper - lower) * 0.1;

    return difficulty;
}

bool WordList::Load()
{
    std::ofstream ofs_create_file(word_path_, std::ofstream::binary | std::ofstream::app); // create file if not exist

    std::ifstream ifs(word_path_, std::ifstream::binary);
    if (ifs)
    {
        Word temp;
        while (ifs)
        {
            temp = Deserialize(ifs);
            if (ifs && temp.word.size() && temp.contributor.size())
            {
				// if (!temp.difficulty)
				temp.difficulty = get_difficulty(temp.word);
                word_vec_.push_back(temp);
                word_set_.insert(std::move(temp.word));
            }
        }
        last_origin_ = word_vec_.size();
		std::sort(word_vec_.begin(), word_vec_.end(), [](const Word & a, const Word & b) { return a.difficulty < b.difficulty; });
        return true;
    }
    else
    {
        Log::WriteLog("WordList: Failed to load wordlist, cannot open the file");
        return false;
    }
}

Word WordList::Deserialize(std::ifstream &ifs)
{
    WordSerializationWarp temp{0, 0, nullptr, 0, nullptr, 0, 0, 0, 0};
    ifs.read(reinterpret_cast<char *>(&temp.len), sizeof(temp.len));
    ifs.read(reinterpret_cast<char *>(&temp.word_len), sizeof(temp.word_len));

    temp.word = std::make_unique<char[]>(temp.word_len);
    ifs.read(temp.word.get(), temp.word_len);

    ifs.read(reinterpret_cast<char *>(&temp.name_len), sizeof(temp.name_len));
    temp.name = std::make_unique<char[]>(temp.name_len);
    ifs.read(temp.name.get(), temp.name_len);

    ifs.read(reinterpret_cast<char *>(&temp.total), sizeof(temp.total));
    ifs.read(reinterpret_cast<char *>(&temp.correct), sizeof(temp.correct));
    ifs.read(reinterpret_cast<char *>(&temp.diff), sizeof(temp.diff));
    ifs.read(reinterpret_cast<char *>(&temp.chksum), sizeof(temp.chksum));

    unsigned char expect_chksum = temp.chksum;
    get_chksum(temp);

    if (expect_chksum != temp.chksum)
    {
		Log::WriteLog(std::string("WordList: ") + temp.word.get() + " checksum error, expected :" +
					  std::to_string(static_cast<int>(expect_chksum)) + ", actually : " + std::to_string(static_cast<int>(temp.chksum)));
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
        return res;
    }
}