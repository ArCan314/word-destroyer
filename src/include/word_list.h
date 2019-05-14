#pragma once

#include <vector>
#include <set>
#include <string>
#include <fstream>

#include "log.h"

struct Word
{
    std::string word;
    std::string contributor;
    unsigned short total;
    unsigned short correct;
    unsigned char difficulty;
};

struct WordSerializationWarp
{
    unsigned char len;
    unsigned char word_len;
    char *word;
    unsigned char name_len;
    char *name;
    unsigned short total;
    unsigned short correct;
    unsigned char diff;
    unsigned char chksum;
};
/*
| 1B  |   1B      | VAR  |   1B     |  VAR  |   2B    |    2B     |   1B    |    1B   |
| LEN |  LEN_WORD | WORD | LEN_NAME |  NAME |  TOTAL  |  CORRECT  |  diff   |  chksum |
*/

class WordList
{
public:
    WordList();

    bool AddWord(const std::string &new_word, const std::string &name);

    const Word &get_word(int difficulty);

    ~WordList();

private:
    bool Save();
    bool Load();
    int get_difficulty(const std::string &word);
    void get_chksum(WordSerializationWarp &warp);
    void Serialize(const Word &warp, char *buf, std::size_t &write_len);
    Word Deserialize(std::ifstream &ifs);

    std::set<std::string> word_set_;
    std::vector<Word> word_vec_;
    unsigned last_origin_ = 0;
    bool is_updated = false;
    static const std::string word_path_;
};
