#include "Windows.h"

#include <cstdlib>
#include <iostream>

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

int main()
{
	AccountSys acc_sys;
	WordList word_list;
	DWORD dwErrCode;
	word_list.AddWord("ans", "admin");
	InitConsole();
	ConsoleIO::set_account_sys_ptr(&acc_sys);
	ConsoleIO::set_wordlist_ptr(&word_list);
	ConsoleIO::IO_Start();

	// std::cin >> std::string();
	Log::CloseLog();
	return 0;
}