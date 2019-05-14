#include <cstdlib>
#include <iostream>

#include "include/account_sys.h"
#include "include/contributor.h"
#include "include/log.h"
#include "include/player.h"
#include "include/contributor.h"
#include "include/console_io.h"

int main()
{
	std::system("chcp 65001");
	ConsoleIO::set_console_window_size();
	ConsoleIO::clear_screen();
	// ConsoleIO::to_fullscreen();
	system("mode 1000");
	std::cin >> std::string();
	return 0;
}