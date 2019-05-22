// #define CLIENT
#define SERVER
#ifdef CLIENT
#include <cstdlib>
#include <iostream>

#include <fstream>
#include <sstream>
#include <string>
#include <cctype>

#include "include/account_sys.h"
#include "include/log.h"
#include "include/player.h"
#include "include/contributor.h"
#include "include/word_list.h"
#include "include/controller.h"


#include "include/console_io.h"
#include "Windows.h"


void InitConsole()
{
	std::system("chcp 65001");
	ConsoleIO::InitAttr();
	ConsoleIO::set_console_font(20);
	ConsoleIO::set_console_window_size(130, 35);
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
	ClientAccountSys acc_sys;
	ClientWordList word_list;
	ClientController controller(&word_list, &acc_sys);
	// DWORD dwErrCode;
	/*
	auto t = [](const std::string &s) {
	std::string temp;
	for (auto c : s)
		if (c >= -1 && c <= 255)
			if (std::isalpha(c))
				temp.push_back(c);
	return temp;
	};

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
	ConsoleIO::set_controller_ptr(&controller);
	ConsoleIO::IO_Start();

	// std::cin >> std::string();
	Log::CloseLog();
	return 0;
}
/*
int main()
{
	std::thread	c(Client);
	std::thread s(Server);
	c.join(), s.join();
	return 0;
}
*/
#endif

#ifdef SERVER

#include <thread>
#include <vector>
#include <utility>
#include <chrono>

#include "include/job_queue.h"
#include "include/receiver.h"
#include "include/resolver.h"
#include "include/controller.h"
#include "include/word_list.h"
#include "include/account_sys.h"

void Recvr(JobQueue *queue)
{
	Receiver recvr(queue);
	recvr.Start();
}

void Resolvr(JobQueue *queue, ServerController *controller)
{
	Resolver resolvr(queue, controller);
	resolvr.Start();
}


int main()
{
	int thread_num = 4;
	AccountSys acc_sys;
	WordList word_list;
	ServerController controller(&word_list, &acc_sys);
	JobQueue job_queue;

	std::vector<std::thread> Resolvrs;
	for (int i = 0; i < thread_num; i++)
		Resolvrs.push_back(std::thread(Resolvr, &job_queue, &controller));
	std::this_thread::sleep_for(std::chrono::milliseconds(200));
	std::thread Rcver(Recvr, &job_queue);
	for (int i = 0; i < thread_num; i++)
		Resolvrs.at(i).join();
	Rcver.join();
	return 0;
}

#endif