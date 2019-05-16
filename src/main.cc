#include "Windows.h"

#include <cstdlib>
#include <iostream>

#include <fstream>
#include <sstream>
#include <string>
#include <cctype>

#include "include/account_sys.h"
#include "include/contributor.h"
#include "include/log.h"
#include "include/player.h"
#include "include/contributor.h"
#include "include/console_io.h"
#include "include/word_list.h"

void InitConsole()
{
	std::system("chcp 65001");
	ConsoleIO::InitAttr();
	ConsoleIO::set_console_font(20);
	ConsoleIO::set_console_window_size(110, 30);
	ConsoleIO::set_console_background( BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED);
	ConsoleIO::disable_scrollbar();
	ConsoleIO::set_window_fixed();
	ConsoleIO::clear_screen();
	// ConsoleIO::to_fullscreen();
	// std::cin >> std::string();
	// ConsoleIO::to_window();
}
/*
std::string t(const std::string &s)
{
	std::string temp;
	for (auto c : s)
		if (c >= -1 && c <= 255)
			if (std::isalpha(c))
				temp.push_back(c);
	return temp;
}
*/
int main()
{
	AccountSys acc_sys;
	WordList word_list;
	DWORD dwErrCode;
	/*
	std::ifstream ifs("../aaa.txt");
	std::ofstream ofs("../oops.txt");
	int count = 0;
	std::string temp_s;
	while (ifs)
	{
		std::getline(ifs, temp_s);
		if (ifs)
		{
			std::stringstream ss(temp_s);
			std::string s;
			while (ss >> s)
			{
				s = t(s);
				if (s.size() > 0 && s.size() < 26)
				{
					if (word_list.AddWord(s, "SHENBIREN"))
					{
						count++;
						ofs << s << " ";
						if (!(count % 8))
							ofs << std::endl;
					}
				}
				s.clear();
			}
		}
	}
	*/
	// word_list.AddWord("ans", "admin");
	InitConsole();
	ConsoleIO::set_account_sys_ptr(&acc_sys);
	ConsoleIO::set_wordlist_ptr(&word_list);
	ConsoleIO::IOD_Start();

	// std::cin >> std::string();
	Log::CloseLog();
	return 0;
}