#pragma once 

// WARNING : OS(WIN32) SPECIFIC CODE
class AccountSys;
class WordList;
enum NextPage;

namespace ConsoleIO
{
#include "Windows.h"

	void InitAttr();

	void set_account_sys_ptr(AccountSys *);
	void set_wordlist_ptr(WordList *);
	DWORD IO_Start(); 
	DWORD IOD_Start();
	// return ErrorCode
	DWORD set_console_window_size();
	DWORD set_console_window_size(SHORT dwWidth, SHORT dwHeight);

	DWORD set_console_background(WORD attribute);

	// return ErrorCode, will change maximum window size
	DWORD set_console_font(SHORT dwFontSize);

	DWORD write_console_in_colour(HANDLE hOut, COORD pos, const char *str, WORD colour);
	DWORD write_console_in_colour(HANDLE hOut, COORD pos, const wchar_t *wstr, WORD colour);

	DWORD clear_screen();

	DWORD set_window_fixed();

	DWORD to_fullscreen();
	DWORD to_window();

	DWORD disable_scrollbar();
	DWORD enable_scrollbar();

	DWORD to_welcome_page();
	DWORD to_menu_page(NextPage &next_page);
	DWORD to_my_info_page(NextPage &next_page);
	DWORD to_user_list_page(NextPage &next_page);

};