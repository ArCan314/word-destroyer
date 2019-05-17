#undef UNICODE
#define _WIN32_WINNT 0x0502
#include "Windows.h"

#include <cwchar>
#include <cstring>
#include <cctype>
#include <cassert>
#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include <random>
#include <memory>
#include <iterator>
#include <new>
#include <chrono>

#include "console_io.h"
#include "log.h"
#include "account_sys.h"
#include "word_list.h"
#include "game.h"

enum NextPage;
enum SortSelection;
struct FilterPack;

// 设置字体和背景的颜色
static const int FOREGROUND_WHITE = (FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN);
static const int BACKGROUND_WHITE = (BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED);

// 提供账号管理及单词查询
static AccountSys *acc_sys;
static WordList *word_list;

static COORD coordOrigincsbiSize = {151, 9000};
static bool is_fullscreen = false; // unused
static bool has_scrollbar = true;  // 记录是否有滚动栏

static const int kAttrBufSize = 120;
static bool attr_inited = false;
static WORD attribute_fwhite[kAttrBufSize];
static WORD attribute_bwhite[kAttrBufSize];
static WORD attribute_fred[kAttrBufSize];
static char space_array[kAttrBufSize];

// 对attribution数组的初始化
void ConsoleIO::InitAttr()
{
	if (!attr_inited)
	{
		attr_inited = true;
		for (int i = 0; i < kAttrBufSize; i++)
		{
			attribute_fwhite[i] = FOREGROUND_WHITE;
			attribute_bwhite[i] = BACKGROUND_WHITE;
			attribute_fred[i] = FOREGROUND_INTENSITY | FOREGROUND_RED | BACKGROUND_WHITE;
			space_array[i] = ' ';
		}
	}
}

static void Shine(HANDLE h, SHORT len, COORD pos);
static DWORD set_cursor_visible(HANDLE h, BOOL visible);
static void ErrorMsg(const std::string &msg, DWORD last_error);
static DWORD press_alt_enter(); // not used
static void DrawUserList(HANDLE h, const std::vector<Player> &p_vec, const char *OptionStr[9 + 1], std::pair<SHORT, SHORT> OptionPos[9 + 1], const std::pair<SHORT, SHORT> &sidex, const SMALL_RECT &srW, int page);
static void DrawUserList(HANDLE h, const std::vector<Contributor> &c_vec, const char *OptionStr[9 + 1], std::pair<SHORT, SHORT> OptionPos[9 + 1], const std::pair<SHORT, SHORT> &sidex, const SMALL_RECT &srW, int page);
static DWORD ShowSortMsgBox(HANDLE hOut, HANDLE hIn, const std::pair<SHORT, SHORT> &sidex, SMALL_RECT srW, UserType utype, SortSelection &s_sel);
static DWORD ShowFilterMsgBox(HANDLE hOut, HANDLE hIn, const std::pair<SHORT, SHORT> &sidex, SMALL_RECT srW, UserType utype, FilterPack &f_pack);
static DWORD ShowGameHelpMsgBox(HANDLE hOut, HANDLE hIn, SHORT x, SHORT y, SHORT dx, SHORT dy);
static DWORD ShowLevelPassMsgBox(HANDLE hOut, HANDLE hIn, SHORT x, SHORT y, SHORT dx, SHORT dy, bool &is_back, bool lv_up, int gain_exp);
static DWORD ShowLevelFailMsgBox(HANDLE hOut, HANDLE hIn, SHORT x, SHORT y, SHORT dx, SHORT dy, bool &is_back);
static DWORD DrawGameFrame(HANDLE hOut, SMALL_RECT srW, SHORT x, SHORT y, SHORT dx, SHORT dy);
static DWORD CleanGameBoard(HANDLE hOut, SHORT x, SHORT y, SHORT dx, SHORT dy);
static void ShowGameWord(HANDLE hOut, SHORT x, SHORT y, SHORT dx, SHORT dy, const std::string &word);
static void RidKeyInput(HANDLE hIn);

// 闪烁效果
static void Shine(HANDLE h, SHORT len, COORD pos)
{
	static DWORD written;
	static constexpr int kShineTime = 7; // 为奇数以保证最后一次闪烁返回原来的状态一定是白底
	static const WORD *attr_bf[2] = {attribute_fwhite, attribute_bwhite};
	for (int i = 0; i < kShineTime; i++)
	{
		Sleep(50); // 足够大以保证刷新不引起闪屏
		WriteConsoleOutputAttribute(h, attr_bf[i % 2], len, pos, &written);
	}
}

// 设置光标的可见性
static DWORD set_cursor_visible(HANDLE h, BOOL visible)
{
	CONSOLE_CURSOR_INFO cci;
	if (GetConsoleCursorInfo(h, &cci) == FALSE)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("ConIO: set cursor visibility failed", last_error);
		Log::WriteLog(std::string("ConIO: set cursor visibility failed : cannot get console cursor info, errorcode: ") + std::to_string(last_error));
		return last_error;
	}
	cci.bVisible = visible;
	if (SetConsoleCursorInfo(h, &cci) == FALSE)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("ConIO: set cursor visibility failed", last_error);
		Log::WriteLog(std::string("ConIO: set cursor visibility failed : cannot set console cursor info, errorcode: ") + std::to_string(last_error));
		return last_error;
	}
	return ERROR_SUCCESS;
}

// 提供错误信息打印
static void ErrorMsg(const std::string &msg, DWORD last_error)
{
	std::cerr << msg << " ErrorCode: " << last_error << std::endl;
}

void ConsoleIO::set_account_sys_ptr(AccountSys *as)
{
	acc_sys = as;
}

void ConsoleIO::set_wordlist_ptr(WordList *wl)
{
	word_list = wl;
}

enum NextPage
{
	P_WELCOME = 0,
	P_MENU,
	P_SP,
	P_MP,
	P_INFO,
	P_ULIST,
	P_EXIT
};

enum SortSelection
{
	SORTS_NAME = 0,
	SORTS_LEVEL,
	SORTS_EXP,
	SORTS_PASS,
	SORTS_CON
};

struct FilterPack
{
	enum FilterPackType
	{
		FPT_NAME = 0,
		FPT_LV,
		FPT_EXP,
		FPT_PASS,
		FPT_CON
	} type;
	std::string name;
	int integer;
	double exp;
};

// 提供页面跳转
DWORD ConsoleIO::IO_Start()
{
	DWORD dwErrCode;
	NextPage next_page;
	bool is_exit = false;
	Log::WriteLog(std::string("ConIO: CLI START"));
	dwErrCode = to_welcome_page();
	next_page = P_MENU;
	if (dwErrCode != ERROR_SUCCESS)
		return dwErrCode;

	clear_screen();

	while (!is_exit)
	{
		switch (next_page)
		{
		case P_WELCOME:
			dwErrCode = to_welcome_page();
			if (dwErrCode)
				return dwErrCode;
			else
				next_page = P_MENU;
			clear_screen();
			break;
		case P_SP:
			if (acc_sys->get_current_usertype() == USERTYPE_C) // 根据用户类型选择进入的页面
				dwErrCode = to_contributor_play_page(next_page);
			else
				dwErrCode = to_player_play_page(next_page);
			clear_screen();
			break;
		case P_MENU:
			dwErrCode = to_menu_page(next_page);
			if (dwErrCode)
				return dwErrCode;
			clear_screen();
			break;
		case P_INFO:
			dwErrCode = to_my_info_page(next_page);
			if (dwErrCode)
				return dwErrCode;
			clear_screen();
			break;
		case P_ULIST:
			dwErrCode = to_user_list_page(next_page);
			if (dwErrCode)
				return dwErrCode;
			clear_screen();
			break;
		case P_EXIT:
			clear_screen();
			is_exit = true;
			break;
		default:
			next_page = P_MENU;
			break;
		}
	}
	Log::WriteLog("Console IO: Exiting");
	return 0;
}

// 为debug提供单页面服务
DWORD ConsoleIO::IOD_Start()
{
	NextPage next_page;
	to_player_play_page(next_page);
	return ERROR_SUCCESS;
}

// 设置初始窗口大小
DWORD ConsoleIO::set_console_window_size()
{
	return set_console_window_size(150, 40);
}

// 设置窗口大小
DWORD ConsoleIO::set_console_window_size(SHORT dwWidth, SHORT dwHeight)
{
	Log::WriteLog(std::string("ConIO: setting console window size, (width, height): (") + std::to_string(dwWidth) + ", " + std::to_string(dwHeight) + ")");

	if (is_fullscreen)
		return ERROR_SUCCESS;

	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

	if (hStdOut == INVALID_HANDLE_VALUE)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Set console window size failed", last_error);
		Log::WriteLog(std::string("ConIO: Set console window failed: cannot get handle of stdout, errorcode: ") + std::to_string(last_error));
		return last_error;
	}

	CONSOLE_SCREEN_BUFFER_INFO csbiStdOut;

	if (GetConsoleScreenBufferInfo(hStdOut, &csbiStdOut) == FALSE)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Set console window size failed", last_error);
		Log::WriteLog(std::string("ConIO: Set console window failed: cannot get console screen buffer info, errorcode: ") + std::to_string(last_error));
		return last_error;
	}

	SMALL_RECT srWnd = csbiStdOut.srWindow;

	if (dwHeight < csbiStdOut.dwMaximumWindowSize.Y - 1) // 检测大小是否超过缓冲区大小
	{
		srWnd.Bottom = (dwHeight > csbiStdOut.dwMaximumWindowSize.Y - 1) ? csbiStdOut.dwMaximumWindowSize.Y - 1 : dwHeight;
		srWnd.Right = (dwWidth > csbiStdOut.dwMaximumWindowSize.X - 1) ? csbiStdOut.dwMaximumWindowSize.X - 1 : dwWidth;

		// change console window size
		if (SetConsoleWindowInfo(hStdOut, TRUE, &srWnd) == FALSE)
		{
			DWORD last_error = GetLastError();
			ErrorMsg("Set console window size failed", last_error);
			Log::WriteLog(std::string("ConIO: Set console window failed: cannot set console window info, errorcode: ") + std::to_string(last_error));
			return last_error;
		}

		if (SetConsoleScreenBufferSize(hStdOut, {dwWidth + 1, (csbiStdOut.dwSize.Y > dwHeight) ? csbiStdOut.dwSize.Y : (dwHeight + 1)}) == FALSE)
		{
			DWORD last_error = GetLastError();
			ErrorMsg("Set console window size failed", last_error);
			Log::WriteLog(std::string("ConIO: Set console window failed: cannot set console screen buffer size, errorcode: ") + std::to_string(last_error));
			return last_error;
		}
	}
	else
	{
		// set console buffer size to a larger one
		if (SetConsoleScreenBufferSize(hStdOut, {dwWidth + 1, (csbiStdOut.dwSize.Y > dwHeight) ? csbiStdOut.dwSize.Y : (dwHeight + 1)}) == FALSE)
		{
			DWORD last_error = GetLastError();
			ErrorMsg("Set console window size failed", last_error);
			Log::WriteLog(std::string("ConIO: Set console window failed: cannot set console screen buffer size, errorcode: ") + std::to_string(last_error));
			return last_error;
		}

		srWnd.Bottom = (csbiStdOut.dwSize.Y > dwHeight) ? csbiStdOut.dwSize.Y - 1 : (dwHeight);
		srWnd.Right = (dwWidth > csbiStdOut.dwMaximumWindowSize.X - 1) ? csbiStdOut.dwMaximumWindowSize.X - 1 : dwWidth;

		// change console window size
		if (SetConsoleWindowInfo(hStdOut, TRUE, &srWnd) == FALSE)
		{
			DWORD last_error = GetLastError();
			ErrorMsg("Set console window size failed", last_error);
			Log::WriteLog(std::string("ConIO: Set console window failed: cannot set console window info, errorcode: ") + std::to_string(last_error));
			return last_error;
		}
	}
	if (GetConsoleScreenBufferInfo(hStdOut, &csbiStdOut) == FALSE)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Set console window size failed", last_error);
		Log::WriteLog(std::string("ConIO: Set console window failed: cannot get console screen buffer info, errorcode: ") + std::to_string(last_error));
		return last_error;
	}

	return ERROR_SUCCESS;
}

// 设置背景
DWORD ConsoleIO::set_console_background(WORD attribute)
{
	Log::WriteLog(std::string("ConIO: setting console background, attr: ") + std::to_string(attribute));
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hStdOut == INVALID_HANDLE_VALUE)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Set console background failed", last_error);
		Log::WriteLog(std::string("ConIO: Set console background failed: cannot get standard output handle, errorcode: ") + std::to_string(last_error));
		return last_error;
	}

	CONSOLE_SCREEN_BUFFER_INFOEX csbiInfo;
	csbiInfo.cbSize = sizeof(csbiInfo);

	if (GetConsoleScreenBufferInfoEx(hStdOut, &csbiInfo) == FALSE)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Set console background failed", last_error);
		Log::WriteLog(std::string("ConIO: Set console background failed: cannot get console screen buffer info, errorcode: ") + std::to_string(last_error));
		return last_error;
	}
	csbiInfo.wAttributes = attribute;
	SetConsoleScreenBufferInfoEx(hStdOut, &csbiInfo);
	return 0;
}

DWORD ConsoleIO::clear_screen()
{
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	COORD coordLeftTop{0, 0};

	if (hStdOut == INVALID_HANDLE_VALUE)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Clear screen failed", last_error);
		Log::WriteLog(std::string("ConIO: Clear screen failed: cannot get standard output handle, errorcode: ") + std::to_string(last_error));
		return last_error;
	}

	CONSOLE_SCREEN_BUFFER_INFO csbiInfo;

	// get console buffer size and attributes
	if (GetConsoleScreenBufferInfo(hStdOut, &csbiInfo) == FALSE)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Clear screen failed", last_error);
		Log::WriteLog(std::string("ConIO: Clear screen failed: cannot get console screen buffer info, errorcode: ") + std::to_string(last_error));
		return last_error;
	}

	DWORD dwCharsWritten;
	DWORD dwBufLen = csbiInfo.dwSize.X * csbiInfo.dwSize.Y;
	std::cout.flush(); // clear the buffer of std::cout

	// fill console buffer with spaces
	if (FillConsoleOutputCharacter(hStdOut, TEXT(' '), dwBufLen, coordLeftTop, &dwCharsWritten) == FALSE)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Clear screen failed", last_error);
		Log::WriteLog(std::string("ConIO: Clear screen failed: cannot fill console with spaces, errorcode: ") + std::to_string(last_error));
		return last_error;
	}

	if (FillConsoleOutputAttribute(hStdOut, csbiInfo.wAttributes, dwBufLen, coordLeftTop, &dwCharsWritten) == FALSE)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Clear screen failed", last_error);
		Log::WriteLog(std::string("ConIO: Clear screen failed: cannot fill console attributes, errorcode: ") + std::to_string(last_error));
		return last_error;
	}

	if (SetConsoleCursorPosition(hStdOut, coordLeftTop) == FALSE)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Clear screen failed", last_error);
		Log::WriteLog(std::string("ConIO: Clear screen failed: cannot set cursor position, errorcode: ") + std::to_string(last_error));
		return last_error;
	}

	return ERROR_SUCCESS;
}

// 模拟按键以实现全屏，未使用
static DWORD press_alt_enter()
{
	INPUT inputTemp[2];
	Log::WriteLog(std::string("ConIO: simulating keyboard press"));
	KEYBDINPUT kbinputAlt = {0, MapVirtualKey(VK_MENU, MAPVK_VK_TO_VSC), KEYEVENTF_SCANCODE, 0, 0},
			   kbinputEnter = {0, MapVirtualKey(VK_RETURN, MAPVK_VK_TO_VSC), KEYEVENTF_SCANCODE, 0, 0};
	int cbSize = sizeof(INPUT);

	inputTemp[0].type = inputTemp[1].type = INPUT_KEYBOARD;
	inputTemp[0].ki = kbinputAlt, inputTemp[1].ki = kbinputEnter;

	if (SendInput(2, inputTemp, cbSize) == FALSE) // 按下按键
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Change display mode failed", last_error);
		Log::WriteLog(std::string("ConIO: Change display mode failed: cannot send key event, errorcode: ") + std::to_string(last_error));
		return last_error;
	}

	inputTemp[0].ki.dwFlags = inputTemp[1].ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE;
	if (SendInput(2, inputTemp, cbSize) == FALSE) // 抬起按键
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Change display mode failed", last_error);
		Log::WriteLog(std::string("ConIO: Change display mode failed: cannot send key event, errorcode: ") + std::to_string(last_error));
		return last_error;
	}

	return ERROR_SUCCESS;
}

DWORD ConsoleIO::to_fullscreen()
{
	if (is_fullscreen)
		return ERROR_SUCCESS;
	Log::WriteLog(std::string("ConIO: set console to fullscreen mode"));

	DWORD last_error = press_alt_enter();
	if (last_error != ERROR_SUCCESS)
	{
		ErrorMsg("Change display mode failed", last_error);
		Log::WriteLog(std::string("ConIO: Change display mode failed: cannot send key event, errorcode: ") + std::to_string(last_error));
		return last_error;
	}

	is_fullscreen = true;
	Sleep(50); // Sleep一段时间以确保全屏按键处理完毕
	if ((last_error = disable_scrollbar()) != ERROR_SUCCESS)
	{
		ErrorMsg("Change display mode failed", last_error);
		Log::WriteLog(std::string("ConIO: Change display mode failed: cannot disable scrollbar, errorcode: ") + std::to_string(last_error));
		return last_error;
	}

	return ERROR_SUCCESS;
}

DWORD ConsoleIO::disable_scrollbar()
{
	if (!has_scrollbar)
		return ERROR_SUCCESS;
	Log::WriteLog(std::string("ConIO: disabling scrollbar"));

	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hStdOut == INVALID_HANDLE_VALUE)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Remove scrollbar failed", last_error);
		Log::WriteLog(std::string("ConIO: Remove scrollbar failed: cannot get standard output handle, errorcode: ") + std::to_string(last_error));
		return last_error;
	}

	CONSOLE_SCREEN_BUFFER_INFO csbiInfo;

	if (GetConsoleScreenBufferInfo(hStdOut, &csbiInfo) == FALSE)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Remove scrollbar failed", last_error);
		Log::WriteLog(std::string("ConIO: Remove scrollbar failed: cannot get console screen buffer info, errorcode: ") + std::to_string(last_error));
		return last_error;
	}

	coordOrigincsbiSize = csbiInfo.dwSize;
	csbiInfo.dwSize = csbiInfo.dwMaximumWindowSize;
	csbiInfo.dwSize.Y = csbiInfo.srWindow.Bottom - csbiInfo.srWindow.Top + 1;

	if (SetConsoleScreenBufferSize(hStdOut, csbiInfo.dwSize) == FALSE)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Remove scrollbar failed", last_error);
		Log::WriteLog(std::string("ConIO: Remove scrollbar failed: cannot set console screen buffer info, errorcode: ") + std::to_string(last_error));
		return last_error;
	}

	has_scrollbar = false;
	return ERROR_SUCCESS;
}

// 从全屏返回窗口化
DWORD ConsoleIO::to_window()
{
	if (!is_fullscreen)
		return ERROR_SUCCESS;
	Log::WriteLog(std::string("ConIO: set console to window mode"));
	DWORD last_error = press_alt_enter();
	if (last_error != ERROR_SUCCESS)
	{
		ErrorMsg("Change display mode failed", last_error);
		Log::WriteLog(std::string("ConIO: Change display mode failed: cannot send key event, errorcode: ") + std::to_string(last_error));
		return last_error;
	}

	Sleep(50);
	is_fullscreen = false;

	if ((last_error == enable_scrollbar()) != ERROR_SUCCESS)
	{
		ErrorMsg("Change display mode failed", last_error);
		Log::WriteLog(std::string("ConIO: Change display mode failed: cannot enable scroll bar, errorcode: ") + std::to_string(last_error));
		return last_error;
	}

	if ((last_error == set_console_window_size()) != ERROR_SUCCESS)
	{
		ErrorMsg("Change display mode failed", last_error);
		Log::WriteLog(std::string("ConIO: Change display mode failed: cannot set console window size, errorcode: ") + std::to_string(last_error));
		return last_error;
	}

	return ERROR_SUCCESS;
}

DWORD ConsoleIO::enable_scrollbar()
{

	if (has_scrollbar)
		return ERROR_SUCCESS;
	Log::WriteLog(std::string("ConIO: enable scrollbar"));

	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hStdOut == INVALID_HANDLE_VALUE)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Enable scrollbar failed", last_error);
		Log::WriteLog(std::string("ConIO: Enable scrollbar failed: cannot get standard output handle, errorcode: ") + std::to_string(last_error));
		return last_error;
	}

	CONSOLE_SCREEN_BUFFER_INFO csbiInfo;

	if (GetConsoleScreenBufferInfo(hStdOut, &csbiInfo) == FALSE)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Enable scrollbar failed", last_error);
		Log::WriteLog(std::string("ConIO: Enable scrollbar failed: cannot get console screen buffer info, errorcode: ") + std::to_string(last_error));
		return last_error;
	}

	csbiInfo.dwSize = coordOrigincsbiSize;

	if (SetConsoleScreenBufferSize(hStdOut, csbiInfo.dwSize) == FALSE) // 通过改变缓冲区大小来控制滚动栏的显示
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Enable scrollbar failed", last_error);
		Log::WriteLog(std::string("ConIO: Enable scrollbar failed: cannot set console screen buffer info, errorcode: ") + std::to_string(last_error));
		return last_error;
	}

	has_scrollbar = true;
	return ERROR_SUCCESS;
}

DWORD ConsoleIO::set_console_font(SHORT dwFontSize)
{
	Log::WriteLog(std::string("ConIO: setting console font, font size: ") + std::to_string(dwFontSize));
	CONSOLE_FONT_INFOEX cfiInfo;
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

	if (hStdOut == INVALID_HANDLE_VALUE)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Change console font size failed", last_error);
		Log::WriteLog(std::string("ConIO: Change console font size failed: cannot get standard output handle, errorcode: ") + std::to_string(last_error));
		return last_error;
	}

	cfiInfo.cbSize = sizeof(cfiInfo);
	if (GetCurrentConsoleFontEx(hStdOut, FALSE, &cfiInfo) == FALSE)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Change console font size failed", last_error);
		Log::WriteLog(std::string("ConIO: Change console font size failed: cannot get console font info, errorcode: ") + std::to_string(last_error));
		return last_error;
	}

	cfiInfo.dwFontSize.X = 0;
	cfiInfo.dwFontSize.Y = dwFontSize;
	cfiInfo.FontWeight = FW_NORMAL;
	std::wcscpy(cfiInfo.FaceName, L"Consolas");

	if (SetCurrentConsoleFontEx(hStdOut, FALSE, &cfiInfo) == FALSE)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Change console font size failed", last_error);
		Log::WriteLog(std::string("ConIO: Change console font size failed: cannot set console font info, errorcode: ") + std::to_string(last_error));
		return last_error;
	}

	return ERROR_SUCCESS;
}

DWORD ConsoleIO::write_console_in_colour(HANDLE hOut, COORD pos, const char *str, WORD colour)
{
	DWORD written;
	WriteConsoleOutputAttribute(hOut, &colour, std::strlen(str), pos, &written);
	WriteConsoleOutputCharacterA(hOut, str, std::strlen(str), pos, &written);
	return ERROR_SUCCESS;
}

DWORD ConsoleIO::set_window_fixed()
{
	HWND consoleWindow = GetConsoleWindow();
	if (consoleWindow == INVALID_HANDLE_VALUE)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Set console window fixed failed", last_error);
		Log::WriteLog(std::string("ConIO: Set console window fixed failed: cannot get console window handle, errorcode: ") + std::to_string(last_error));
		return last_error;
	}

	if (SetWindowLong(consoleWindow, GWL_STYLE, GetWindowLong(consoleWindow, GWL_STYLE) & ~WS_MAXIMIZEBOX & ~WS_SIZEBOX) == FALSE)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Set console window fixed failed", last_error);
		Log::WriteLog(std::string("ConIO: Set console window fixed failed: cannot set window attr, errorcode: ") + std::to_string(last_error));
		return last_error;
	}
	return ERROR_SUCCESS;
}

DWORD ConsoleIO::to_welcome_page()
{
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	enum WelPos
	{
		ACC = 0,
		PAS,
		LOG,
		SIGN,
		INFO
	};
	auto inc = [](WelPos &a) { a = (a == SIGN) ? ACC : static_cast<WelPos>(a + 1); };
	auto dec = [](WelPos &a) { a = (a == ACC) ? SIGN : static_cast<WelPos>(a - 1); };
	if (hStdOut == INVALID_HANDLE_VALUE || hStdIn == INVALID_HANDLE_VALUE)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Show welcome page failed", last_error);
		Log::WriteLog(std::string("ConIO: Show welcome page failed: cannot get standard output handle, errorcode: ") + std::to_string(last_error));
		return last_error;
	}

	if (GetConsoleScreenBufferInfo(hStdOut, &csbi) == FALSE)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Show welcome page failed", last_error);
		Log::WriteLog(std::string("ConIO: Show welcome page failed: cannot get console screen buffer info, errorcode: ") + std::to_string(last_error));
		return last_error;
	}

	DWORD dwOldConsoleMode;
	GetConsoleMode(hStdOut, &dwOldConsoleMode);
	SetConsoleMode(hStdOut, ENABLE_PROCESSED_INPUT);

	DWORD written;
	SMALL_RECT srW = csbi.srWindow;
	const char *OptionStr[INFO + 1] =
		{
			"Account",
			"Password",
			"Log in",
			"Sign up",
			"",
		};

	std::pair<SHORT /*X*/, SHORT /*Y*/> OptionPos[INFO + 1] =
		{
			{srW.Right / 3, 12},						 // ACC
			{srW.Right / 3, 15},						 // PAS
			{srW.Right / 3 + std::strlen("Log in"), 19}, // LOG
			{srW.Right * 4 / 7, 19},					 // SIGN
			{srW.Right / 2, 23},						 // INFO
		};

	for (SHORT row = 0; row < csbi.srWindow.Bottom; row++)
		for (SHORT col = 0; col < csbi.srWindow.Right; col++)
		{
			if (row == 0 || row == 6 || col == 0 || col == csbi.srWindow.Right - 1 || row == csbi.srWindow.Bottom - 1)
				WriteConsoleOutputCharacter(hStdOut, "%", 1, {col, row}, &written);

			if (row > 0 && row < 6 && col < srW.Right - 1 && col > 0)
			{
				char charFill[] = " ";
				WriteConsoleOutputCharacter(hStdOut, charFill + std::rand() % (sizeof(charFill) - 1), 1, {col, row}, &written);
			}

			if (col == OptionPos[ACC].first - std::strlen(OptionStr[ACC]) && row == OptionPos[ACC].second)
				WriteConsoleOutputCharacter(hStdOut, OptionStr[ACC], std::strlen(OptionStr[ACC]), {col, row}, &written);

			if (col == OptionPos[PAS].first - std::strlen(OptionStr[PAS]) && row == OptionPos[PAS].second)
				WriteConsoleOutputCharacter(hStdOut, OptionStr[PAS], std::strlen(OptionStr[PAS]), {col, row}, &written);

			if (col == OptionPos[LOG].first && row == OptionPos[LOG].second)
				WriteConsoleOutputCharacter(hStdOut, OptionStr[LOG], std::strlen(OptionStr[LOG]), {col, row}, &written);

			if (col == OptionPos[SIGN].first && row == OptionPos[SIGN].second)
				WriteConsoleOutputCharacter(hStdOut, OptionStr[SIGN], std::strlen(OptionStr[SIGN]), {col, row}, &written);
		}

	WelPos mpos = ACC;
	std::string account_name, password;

	INPUT_RECORD irKb[12];
	DWORD wNumber;
	std::string msg;
	bool is_break = false;

	SetConsoleCursorPosition(hStdOut, {OptionPos[ACC].first + 1, OptionPos[ACC].second});
	set_cursor_visible(hStdOut, TRUE);
	while (!is_break)
	{
		if (ReadConsoleInput(hStdIn, irKb, 12, &wNumber) == FALSE)
		{
			DWORD last_error = GetLastError();
			ErrorMsg("Show welcome page failed", last_error);
			Log::WriteLog(std::string("ConIO: Show welcome page failed: cannot read console input, errorcode: ") + std::to_string(last_error));
			return last_error;
		}

		for (DWORD i = 0; i < wNumber; i++)
		{
			if (irKb[i].EventType == KEY_EVENT && irKb[i].Event.KeyEvent.bKeyDown == TRUE)
			{
				KEY_EVENT_RECORD ker = irKb[i].Event.KeyEvent;

				if (std::isalnum(ker.uChar.AsciiChar))
				{
					char input_char = ker.uChar.AsciiChar;
					switch (mpos)
					{
					case ACC:
						if (account_name.size() < 16)
						{
							WriteConsoleOutputCharacter(hStdOut, &input_char, 1, {OptionPos[ACC].first + 1 + (SHORT)account_name.length(), OptionPos[ACC].second}, &written);
							SetConsoleCursorPosition(hStdOut, {OptionPos[ACC].first + 2 + (SHORT)account_name.length(), OptionPos[ACC].second});
							account_name.push_back(ker.uChar.AsciiChar);
						}
						break;
					case PAS:
						if (password.size() < 24)
						{
							WriteConsoleOutputCharacter(hStdOut, "*", 1, {OptionPos[PAS].first + 1 + (SHORT)password.length(), OptionPos[PAS].second}, &written);
							SetConsoleCursorPosition(hStdOut, {OptionPos[PAS].first + 2 + (SHORT)password.length(), OptionPos[PAS].second});
							password.push_back(ker.uChar.AsciiChar);
						}
						break;
					default:
						break;
					}
				}
				else
				{
					switch (ker.wVirtualKeyCode)
					{
					case VK_UP:
						if (mpos == LOG || mpos == SIGN)
						{
							set_cursor_visible(hStdOut, TRUE);
							WriteConsoleOutputAttribute(hStdOut, attribute_bwhite, std::strlen(OptionStr[mpos]), {OptionPos[mpos].first, OptionPos[mpos].second}, &written);

							SetConsoleCursorPosition(hStdOut, {OptionPos[PAS].first + 1 + (SHORT)password.length(), OptionPos[PAS].second});
							mpos = PAS;
						}
						else if (mpos == ACC)
						{
							set_cursor_visible(hStdOut, FALSE);

							WriteConsoleOutputAttribute(hStdOut, attribute_fwhite, std::strlen(OptionStr[LOG]), {OptionPos[LOG].first, OptionPos[LOG].second}, &written);
							mpos = LOG;
						}
						else if (mpos == PAS)
						{
							SetConsoleCursorPosition(hStdOut, {OptionPos[ACC].first + 1 + (SHORT)account_name.length(), OptionPos[ACC].second});
							mpos = ACC;
						}
						break;
					case VK_DOWN:
						if (mpos == LOG || mpos == SIGN)
						{
							set_cursor_visible(hStdOut, TRUE);
							WriteConsoleOutputAttribute(hStdOut, attribute_bwhite, std::strlen(OptionStr[mpos]), {OptionPos[mpos].first, OptionPos[mpos].second}, &written);

							SetConsoleCursorPosition(hStdOut, {OptionPos[ACC].first + 1 + (SHORT)account_name.length(), OptionPos[ACC].second});
							mpos = ACC;
						}
						else if (mpos == ACC)
						{
							SetConsoleCursorPosition(hStdOut, {OptionPos[PAS].first + 1 + (SHORT)password.length(), OptionPos[PAS].second});
							mpos = PAS;
						}
						else if (mpos == PAS)
						{
							set_cursor_visible(hStdOut, FALSE);
							WriteConsoleOutputAttribute(hStdOut, attribute_fwhite, std::strlen(OptionStr[LOG]), {OptionPos[LOG].first, OptionPos[LOG].second}, &written);
							mpos = LOG;
						}
						break;
					case VK_LEFT:
					case VK_RIGHT:
						if (mpos == LOG || mpos == SIGN)
						{
							WriteConsoleOutputAttribute(hStdOut, attribute_bwhite, std::strlen(OptionStr[mpos]), {OptionPos[mpos].first, OptionPos[mpos].second}, &written);
							mpos = (mpos == LOG) ? SIGN : LOG;
							WriteConsoleOutputAttribute(hStdOut, attribute_fwhite, std::strlen(OptionStr[mpos]), {OptionPos[mpos].first, OptionPos[mpos].second}, &written);
						}
						break;
					case VK_RETURN:
						switch (mpos)
						{
						case LOG:
							Shine(hStdOut, std::strlen(OptionStr[LOG]), {OptionPos[LOG].first, OptionPos[LOG].second});
							if (account_name.length() < acc_sys->get_min_acc_len())
								msg = "Account must have 4 characters at least.";
							else if (password.length() < acc_sys->get_min_pswd_len())
								msg = "Password must have 9 characters at least.";
							else if (!acc_sys->LogIn(account_name, password))
								msg = "Failed to log in, check your account name and password.";
							else
								is_break = true;

							if (!is_break)
							{
								WriteConsoleOutputCharacter(hStdOut, space_array, srW.Right - srW.Left - 2, {srW.Left + 1, OptionPos[INFO].second}, &written);
								WriteConsoleOutputCharacter(hStdOut, msg.c_str(), std::strlen(msg.c_str()), {OptionPos[INFO].first - static_cast<SHORT>(std::strlen(msg.c_str())) / 2, OptionPos[INFO].second}, &written);
							}
							break;
						case SIGN:
							Shine(hStdOut, std::strlen(OptionStr[SIGN]), {OptionPos[SIGN].first, OptionPos[SIGN].second});
							if (account_name.length() < acc_sys->get_min_acc_len())
								msg = "Account must have 4 characters at least.";
							else if (password.length() < acc_sys->get_min_pswd_len())
								msg = "Password must have 9 characters at least.";
							else if (!acc_sys->SignUp(account_name, password, UserType::USERTYPE_C))
								msg = "Sign up failed, account name has been used.";
							else
								is_break = true;

							if (!is_break)
							{
								WriteConsoleOutputCharacter(hStdOut, space_array, srW.Right - srW.Left - 2, {srW.Left + 1, OptionPos[INFO].second}, &written);
								WriteConsoleOutputCharacter(hStdOut, msg.c_str(), std::strlen(msg.c_str()), {OptionPos[INFO].first - static_cast<SHORT>(std::strlen(msg.c_str())) / 2, OptionPos[INFO].second}, &written);
							}

							break;
						default:
							break;
						}
						break;
					case VK_BACK:
						switch (mpos)
						{
						case ACC:
							if (account_name.size())
							{
								account_name.pop_back();
								WriteConsoleOutputCharacter(hStdOut, " ", 1, {OptionPos[ACC].first + 1 + (SHORT)account_name.length(), OptionPos[ACC].second}, &written);
								SetConsoleCursorPosition(hStdOut, {OptionPos[ACC].first + 1 + (SHORT)account_name.length(), OptionPos[ACC].second});
							}
							break;
						case PAS:
							if (password.size())
							{
								password.pop_back();
								WriteConsoleOutputCharacter(hStdOut, " ", 1, {OptionPos[PAS].first + 1 + (SHORT)password.length(), OptionPos[PAS].second}, &written);
								SetConsoleCursorPosition(hStdOut, {OptionPos[PAS].first + 1 + (SHORT)password.length(), OptionPos[PAS].second});
							}
							break;
						default:
							break;
						}
						break;
					default:
						break;
					}
				}
			}
		}
	}

	SetConsoleMode(hStdOut, dwOldConsoleMode);

	return ERROR_SUCCESS;
}

DWORD ConsoleIO::to_menu_page(NextPage &next_page)
{
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	DWORD dwOldConsoleMode;

	enum MenuPos
	{
		START_SINGLE = 0,
		START_MULT,
		MY_INFO,
		USER_LIST,
		EXIT
	};
	auto inc = [](MenuPos &a) { a = (a == EXIT) ? START_SINGLE : static_cast<MenuPos>(a + 1); };
	auto dec = [](MenuPos &a) { a = (a == START_SINGLE) ? EXIT : static_cast<MenuPos>(a - 1); };
	if (hStdOut == INVALID_HANDLE_VALUE || hStdIn == INVALID_HANDLE_VALUE)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Show menu failed", last_error);
		Log::WriteLog(std::string("ConIO: Show menu failed: cannot get standard output handle, errorcode: ") + std::to_string(last_error));
		return last_error;
	}

	if (GetConsoleScreenBufferInfo(hStdOut, &csbi) == FALSE)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Show menu failed", last_error);
		Log::WriteLog(std::string("ConIO: Show menu failed: cannot get console screen buffer info, errorcode: ") + std::to_string(last_error));
		return last_error;
	}

	set_cursor_visible(hStdOut, FALSE);
	GetConsoleMode(hStdOut, &dwOldConsoleMode);
	SetConsoleMode(hStdOut, ENABLE_PROCESSED_INPUT);

	DWORD written;
	SMALL_RECT srW = csbi.srWindow;

	SHORT x = srW.Right / 3;
	SHORT y = srW.Top;
	SHORT dx = srW.Right / 3;
	SHORT dy = (srW.Bottom - 1) - y;

	const char *OptionStr[EXIT + 1] =
		{
			"Play Offline",
			"Play Online",
			"My Infomation",
			"Other Players",
			"Exit"};

	std::pair<SHORT /*X*/, SHORT /*Y*/> OptionPos[] =
		{
			{x + dx * 1 / 2, y + dy * 1 / 6}, // START_SINGLE
			{x + dx * 1 / 2, y + dy * 2 / 6}, // START_MULTIPLE
			{x + dx * 1 / 2, y + dy * 3 / 6}, // MY_INFO
			{x + dx * 1 / 2, y + dy * 4 / 6}, // USER_LIST
			{x + dx * 1 / 2, y + dy * 5 / 6}, // EXIT
		};

	for (SHORT row = 0; row < srW.Bottom; row++)
		for (SHORT col = 0; col < srW.Right; col++)
		{
			if (col < x || col > x + dx)
				WriteConsoleOutputCharacter(hStdOut, "%", 1, {col, row}, &written);
			else if (col == x || col == x + dx)
				WriteConsoleOutputCharacter(hStdOut, "#", 1, {col, row}, &written);
			else if (row == y || row == y + dy)
				WriteConsoleOutputCharacter(hStdOut, "$", 1, {col, row}, &written);
			else if (row == OptionPos[START_SINGLE].second ||
					 row == OptionPos[START_MULT].second ||
					 row == OptionPos[MY_INFO].second ||
					 row == OptionPos[USER_LIST].second ||
					 row == OptionPos[EXIT].second)
			{
				WriteConsoleOutputCharacter(hStdOut, "#", 1, {col, row - 1}, &written);
				WriteConsoleOutputCharacter(hStdOut, "#", 1, {col, row + 1}, &written);
			}
		}

	for (int i = START_SINGLE; i <= EXIT; i++)
		WriteConsoleOutputCharacter(hStdOut, OptionStr[i], std::strlen(OptionStr[i]), {OptionPos[i].first - static_cast<SHORT>(std::strlen(OptionStr[i])) / 2, OptionPos[i].second}, &written);

	bool is_break = false;
	MenuPos mpos = START_SINGLE;

	std::pair<SHORT /*L-Side*/, SHORT /*R-Side*/> side = {x + 1, x + dx - 1};
	WriteConsoleOutputAttribute(hStdOut, attribute_fwhite, side.second - side.first, {side.first, OptionPos[START_SINGLE].second}, &written);

	INPUT_RECORD irKb[12];
	DWORD wNumber;
	while (!is_break)
	{
		if (ReadConsoleInput(hStdIn, irKb, 12, &wNumber) == FALSE)
		{
			DWORD last_error = GetLastError();
			ErrorMsg("Show welcome page failed", last_error);
			Log::WriteLog(std::string("ConIO: Show welcome page failed: cannot read console input, errorcode: ") + std::to_string(last_error));
			return last_error;
		}

		for (DWORD i = 0; i < wNumber; i++)
		{
			if (irKb[i].EventType == KEY_EVENT && irKb[i].Event.KeyEvent.bKeyDown)
			{
				switch (irKb[i].Event.KeyEvent.wVirtualKeyCode)
				{
				case VK_UP:
					WriteConsoleOutputAttribute(hStdOut, attribute_bwhite, side.second - side.first, {side.first, OptionPos[mpos].second}, &written);
					dec(mpos);
					WriteConsoleOutputAttribute(hStdOut, attribute_fwhite, side.second - side.first, {side.first, OptionPos[mpos].second}, &written);
					break;
				case VK_DOWN:
					WriteConsoleOutputAttribute(hStdOut, attribute_bwhite, side.second - side.first, {side.first, OptionPos[mpos].second}, &written);
					inc(mpos);
					WriteConsoleOutputAttribute(hStdOut, attribute_fwhite, side.second - side.first, {side.first, OptionPos[mpos].second}, &written);
					break;
				case VK_RETURN:
					switch (mpos)
					{
					case START_SINGLE:
						next_page = P_SP;
						break;
					case START_MULT:
						next_page = P_MP;
						break;
					case MY_INFO:
						next_page = P_INFO;
						break;
					case USER_LIST:
						next_page = P_ULIST;
						break;
					case EXIT:
						next_page = P_EXIT;
						break;
					default:
						break;
					}
					is_break = true;
					Shine(hStdOut, side.second - side.first, {side.first, OptionPos[mpos].second});
					break;
				default:
					break;
				}
				// is_break = false;
			}
		}
	}

	SetConsoleMode(hStdOut, dwOldConsoleMode);
	set_cursor_visible(hStdOut, TRUE);
	return ERROR_SUCCESS;
}

DWORD ConsoleIO::to_my_info_page(NextPage &next_page)
{
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	DWORD dwOldConsoleMode;

	enum MenuPos
	{
		// USER
		NAME = 0,
		ROLE,
		LEVEL,
		// CONTRIBUTOR
		CONTRIBUTED,
		// PLAYER
		EXP,
		L_PASSED,
		CHANGE_PSWD,
		CHANGE_ROLE,
		BACK,
		INFO
	};

	if (hStdOut == INVALID_HANDLE_VALUE || hStdIn == INVALID_HANDLE_VALUE)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Show menu failed", last_error);
		Log::WriteLog(std::string("ConIO: Show menu failed: cannot get standard output handle, errorcode: ") + std::to_string(last_error));
		return last_error;
	}

	if (GetConsoleScreenBufferInfo(hStdOut, &csbi) == FALSE)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Show menu failed", last_error);
		Log::WriteLog(std::string("ConIO: Show menu failed: cannot get console screen buffer info, errorcode: ") + std::to_string(last_error));
		return last_error;
	}

	set_cursor_visible(hStdOut, FALSE);
	GetConsoleMode(hStdOut, &dwOldConsoleMode);
	SetConsoleMode(hStdOut, ENABLE_PROCESSED_INPUT);

	DWORD written;
	SMALL_RECT srW = csbi.srWindow;

	SHORT x = srW.Right / 5;
	SHORT y = srW.Top;
	SHORT dx = srW.Right * 3 / 5;
	SHORT dy = srW.Bottom - 1 - y;

	const char *OptionStr[INFO + 1] =
		{
			"Name:",
			"Role:",
			"Level:",
			"Contributed:",
			"XP:",
			"Level passed:",
			"Change my password",
			"Change my role",
			"Back to menu",
			"info, don't use"};

	std::pair<SHORT /*X*/, SHORT /*Y*/> OptionPos[] =
		{
			{x + dx * 1 / 4, y + dy * 1 / 5}, // NAME
			//
			{x + dx * 1 / 4, y + dy * 2 / 5}, // ROLE
			{x + dx * 3 / 4, y + dy * 2 / 5}, // LEVEL
			//
			{x + dx * 1 / 4, y + dy * 3 / 5}, // CONTIBUTED
			/**/
			{x + dx * 1 / 4, y + dy * 3 / 5}, // EXP
			{x + dx * 3 / 4, y + dy * 3 / 5}, // L_PASSED
			//
			{0 / 2, 0},																						// CHANGE_PSWD
			{x + dx * 1 / 3 - static_cast<SHORT>(std::strlen(OptionStr[CHANGE_ROLE])) / 2, y + dy * 4 / 5}, // CHANGE_ROLE
			{x + dx * 2 / 3 - static_cast<SHORT>(std::strlen(OptionStr[BACK])) / 2, y + dy * 4 / 5},		// BACK
			{x + dx * 1 / 2, y + dy * 9 / 10}																// INFO
		};

	// acc_sys->set_current_user();

	// Dont release temp_u

	User *temp_u = nullptr;
	std::unique_ptr<Player> temp_p(new Player);
	std::unique_ptr<Contributor> temp_c(new Contributor);
	if (acc_sys->get_current_usertype() == USERTYPE_C)
	{
		(*temp_c) = acc_sys->get_contributor(acc_sys->get_current_user_str());
		temp_u = temp_c.get();
	}
	else if (acc_sys->get_current_usertype() == USERTYPE_P)
	{
		(*temp_p) = acc_sys->get_player(acc_sys->get_current_user_str());
		temp_u = temp_p.get();
	}
	assert(temp_u);

	for (SHORT row = 0; row < srW.Bottom; row++)
		for (SHORT col = 0; col < srW.Right; col++)
		{
			if (col < x || col > x + dx)
				WriteConsoleOutputCharacter(hStdOut, "%", 1, {col, row}, &written);
			else if (col == x || col == x + dx)
				WriteConsoleOutputCharacter(hStdOut, "#", 1, {col, row}, &written);
			else if (row == y || row == y + dy)
				WriteConsoleOutputCharacter(hStdOut, "$", 1, {col, row}, &written);

			if (col == OptionPos[NAME].first && row == OptionPos[NAME].second)
			{
				WriteConsoleOutputCharacter(hStdOut, OptionStr[NAME], std::strlen(OptionStr[NAME]), {OptionPos[NAME].first - static_cast<SHORT>(std::strlen(OptionStr[NAME])), OptionPos[NAME].second}, &written);
				WriteConsoleOutputCharacter(hStdOut, (*temp_u).get_user_name().c_str(), (*temp_u).get_user_name().size(), {OptionPos[NAME].first + 1, OptionPos[NAME].second}, &written);
			}
			else if (col == OptionPos[ROLE].first && row == OptionPos[ROLE].second)
			{
				std::string rstr = acc_sys->get_utype_str((*temp_u).get_user_type());
				WriteConsoleOutputCharacter(hStdOut, OptionStr[ROLE], std::strlen(OptionStr[ROLE]), {OptionPos[ROLE].first - static_cast<SHORT>(std::strlen(OptionStr[ROLE])), OptionPos[ROLE].second}, &written);
				WriteConsoleOutputCharacter(hStdOut, rstr.c_str(), rstr.size(), {OptionPos[ROLE].first + 1, OptionPos[ROLE].second}, &written);
			}
			else if (col == OptionPos[LEVEL].first && row == OptionPos[LEVEL].second)
			{
				std::string level = std::to_string((*temp_u).get_level());
				WriteConsoleOutputCharacter(hStdOut, OptionStr[LEVEL], std::strlen(OptionStr[LEVEL]), {OptionPos[LEVEL].first - static_cast<SHORT>(std::strlen(OptionStr[LEVEL])), OptionPos[LEVEL].second}, &written);
				WriteConsoleOutputCharacter(hStdOut, level.c_str(), level.size(), {OptionPos[LEVEL].first + 1, OptionPos[LEVEL].second}, &written);
			}
			else if (col == OptionPos[CHANGE_ROLE].first && row == OptionPos[CHANGE_ROLE].second)
			{
				WriteConsoleOutputCharacter(hStdOut, OptionStr[CHANGE_ROLE], std::strlen(OptionStr[CHANGE_ROLE]), {OptionPos[CHANGE_ROLE].first, OptionPos[CHANGE_ROLE].second}, &written);
			}
			else if (col == OptionPos[BACK].first && row == OptionPos[BACK].second)
			{
				WriteConsoleOutputCharacter(hStdOut, OptionStr[BACK], std::strlen(OptionStr[BACK]), {OptionPos[BACK].first, OptionPos[BACK].second}, &written);
			}

			switch ((*temp_u).get_user_type())
			{
			case USERTYPE_C:
				if (col == OptionPos[CONTRIBUTED].first && row == OptionPos[CONTRIBUTED].second)
				{
					std::string contributed_str = std::to_string((dynamic_cast<Contributor *>(temp_u))->get_word_contributed());
					WriteConsoleOutputCharacter(hStdOut, OptionStr[CONTRIBUTED], std::strlen(OptionStr[CONTRIBUTED]), {OptionPos[CONTRIBUTED].first - static_cast<SHORT>(std::strlen(OptionStr[CONTRIBUTED])), OptionPos[CONTRIBUTED].second}, &written);
					WriteConsoleOutputCharacter(hStdOut, contributed_str.c_str(), contributed_str.size(), {OptionPos[CONTRIBUTED].first + 1, OptionPos[CONTRIBUTED].second}, &written);
				}
				break;
			case USERTYPE_P:
				if (col == OptionPos[EXP].first && row == OptionPos[EXP].second)
				{
					std::string exp_str = std::to_string(static_cast<int>((dynamic_cast<Player *>(temp_u))->get_exp()));
					WriteConsoleOutputCharacter(hStdOut, OptionStr[EXP], std::strlen(OptionStr[EXP]), {OptionPos[EXP].first - static_cast<SHORT>(std::strlen(OptionStr[EXP])), OptionPos[EXP].second}, &written);
					WriteConsoleOutputCharacter(hStdOut, exp_str.c_str(), exp_str.size(), {OptionPos[EXP].first + 1, OptionPos[EXP].second}, &written);
				}
				else if (col == OptionPos[L_PASSED].first && row == OptionPos[L_PASSED].second)
				{
					std::string lp_str = std::to_string((dynamic_cast<Player *>(temp_u))->get_level_passed());
					WriteConsoleOutputCharacter(hStdOut, OptionStr[L_PASSED], std::strlen(OptionStr[L_PASSED]), {OptionPos[L_PASSED].first - static_cast<SHORT>(std::strlen(OptionStr[L_PASSED])), OptionPos[L_PASSED].second}, &written);
					WriteConsoleOutputCharacter(hStdOut, lp_str.c_str(), lp_str.size(), {OptionPos[L_PASSED].first + 1, OptionPos[L_PASSED].second}, &written);
				}
				break;
			default:
				break;
			}
		}

	bool is_break = false;
	MenuPos mpos = BACK;

	WriteConsoleOutputAttribute(hStdOut, attribute_fwhite, std::strlen(OptionStr[BACK]), {OptionPos[BACK].first, OptionPos[BACK].second}, &written);

	INPUT_RECORD irKb[12];
	DWORD wNumber;
	bool ack_change_role = false;
	while (!is_break)
	{
		if (ReadConsoleInput(hStdIn, irKb, 12, &wNumber) == FALSE)
		{
			DWORD last_error = GetLastError();
			ErrorMsg("Show welcome page failed", last_error);
			Log::WriteLog(std::string("ConIO: Show welcome page failed: cannot read console input, errorcode: ") + std::to_string(last_error));
			return last_error;
		}

		for (DWORD i = 0; i < wNumber; i++)
		{
			if (irKb[i].EventType == KEY_EVENT && irKb[i].Event.KeyEvent.bKeyDown)
			{
				switch (irKb[i].Event.KeyEvent.wVirtualKeyCode)
				{
				case VK_LEFT:
				case VK_RIGHT:
					if (ack_change_role)
					{
						WriteConsoleOutputCharacter(hStdOut, space_array, dx - 2, {x + 1, OptionPos[INFO].second}, &written);
						WriteConsoleOutputAttribute(hStdOut, attribute_bwhite, dx - 2, {x + 1, OptionPos[INFO].second}, &written);
						ack_change_role = false;
					}
					WriteConsoleOutputAttribute(hStdOut, attribute_bwhite, std::strlen(OptionStr[mpos]), {OptionPos[mpos].first, OptionPos[mpos].second}, &written);
					mpos = (mpos == BACK) ? CHANGE_ROLE : BACK;
					WriteConsoleOutputAttribute(hStdOut, attribute_fwhite, std::strlen(OptionStr[mpos]), {OptionPos[mpos].first, OptionPos[mpos].second}, &written);
					break;
				case VK_RETURN:
					switch (mpos)
					{
					case CHANGE_ROLE:
						if (ack_change_role)
						{
							acc_sys->ChangeRole((*temp_u).get_user_name());
							next_page = P_INFO;
							is_break = true;
							WriteConsoleOutputCharacter(hStdOut, space_array, dx - 2, {x + 1, OptionPos[INFO].second}, &written);
							WriteConsoleOutputAttribute(hStdOut, attribute_bwhite, dx - 2, {x + 1, OptionPos[INFO].second}, &written);
							Shine(hStdOut, std::strlen(OptionStr[CHANGE_ROLE]), {OptionPos[CHANGE_ROLE].first, OptionPos[CHANGE_ROLE].second});
						}
						else
						{
							std::string warning("WARNING: ALL DATA WOULD BE REESET, PRESS AGAIN TO CONTINUE.");
							WriteConsoleOutputCharacter(hStdOut, warning.c_str(), warning.size(), {OptionPos[INFO].first - static_cast<SHORT>(warning.size()) / 2, OptionPos[INFO].second}, &written);
							WriteConsoleOutputAttribute(hStdOut, attribute_fred, warning.size(), {OptionPos[INFO].first - static_cast<SHORT>(warning.size()) / 2, OptionPos[INFO].second}, &written);
							ack_change_role = true;
						}
						break;
					case BACK:
						next_page = P_MENU;
						is_break = true;
						Shine(hStdOut, std::strlen(OptionStr[BACK]), {OptionPos[BACK].first, OptionPos[BACK].second});
						break;
					default:
						break;
					}
					break;
				default:
					break;
				}
			}
		}
	}

	SetConsoleMode(hStdOut, dwOldConsoleMode);
	set_cursor_visible(hStdOut, TRUE);
	return ERROR_SUCCESS;
}

static void DrawUserList(HANDLE h, const std::vector<Player> &p_vec, const char *OptionStr[9 + 1], std::pair<SHORT, SHORT> OptionPos[9 + 1], const std::pair<SHORT, SHORT> &sidex, const SMALL_RECT &srW, int page)
{
	enum UListPos
	{
		HEADER = 0,
		NAME,
		LEVEL,
		EXP,
		LP,
		CON,
		SORT,
		FILTER,
		BACK,
		INFO
	};
	DWORD written;
	UserType utype = USERTYPE_P;
	int user_per_page = OptionPos[SORT].second - OptionPos[NAME].second;
	int add = user_per_page * (page - 1);

	auto beg = p_vec.cbegin(), end = p_vec.cend();

	if (p_vec.size() >= add)
		beg += add;

	for (SHORT row = 0; row < srW.Bottom; row++)
		for (SHORT col = 0; col < srW.Right; col++)
		{
			if (col == OptionPos[HEADER].first && row == OptionPos[HEADER].second)
			{
				std::string header_str = "PLAYERS";
				WriteConsoleOutputCharacter(h, header_str.c_str(), header_str.size(), {OptionPos[HEADER].first - static_cast<SHORT>(header_str.size()) / 2, OptionPos[HEADER].second}, &written);
			}
			else if (col == OptionPos[EXP].first && row == OptionPos[EXP].second)
			{
				WriteConsoleOutputCharacter(h, OptionStr[EXP], std::strlen(OptionStr[EXP]), {OptionPos[EXP].first - static_cast<SHORT>(std::strlen(OptionStr[EXP])) / 2, OptionPos[EXP].second}, &written);
			}
			else if (col == OptionPos[LP].first && row == OptionPos[LP].second)
			{
				WriteConsoleOutputCharacter(h, OptionStr[LP], std::strlen(OptionStr[LP]), {OptionPos[LP].first - static_cast<SHORT>(std::strlen(OptionStr[LP])) / 2, OptionPos[LP].second}, &written);
			}
		}

	for (SHORT row = OptionPos[NAME].second + 1; row < OptionPos[SORT].second - 1; row++)
		WriteConsoleOutputCharacter(h, space_array, sidex.second - sidex.first, {sidex.first, row}, &written);

	for (SHORT row = OptionPos[NAME].second + 1; beg != end && row < OptionPos[SORT].second - 1; row++, beg++)
		for (SHORT col = 0; col < srW.Right; col++)
		{
			if (col == OptionPos[NAME].first)
			{
				std::string name = (*beg).get_user_name();
				WriteConsoleOutputCharacter(h, name.c_str(), name.size(), {OptionPos[NAME].first - static_cast<SHORT>(name.size()) / 2, row}, &written);
			}
			else if (col == OptionPos[LEVEL].first)
			{
				std::string level_str = std::to_string((*beg).get_level());
				WriteConsoleOutputCharacter(h, level_str.c_str(), level_str.size(), {OptionPos[LEVEL].first - static_cast<SHORT>(level_str.size()) / 2, row}, &written);
			}
			else if (col == OptionPos[EXP].first)
			{
				std::string xp_str = std::to_string(static_cast<int>((*beg).get_exp()));
				WriteConsoleOutputCharacter(h, xp_str.c_str(), xp_str.size(), {OptionPos[EXP].first - static_cast<SHORT>(xp_str.size()) / 2, row}, &written);
			}
			else if (col == OptionPos[LP].first)
			{
				std::string lp_str = std::to_string((*beg).get_level_passed());
				WriteConsoleOutputCharacter(h, lp_str.c_str(), lp_str.size(), {OptionPos[LP].first - static_cast<SHORT>(lp_str.size()) / 2, row}, &written);
			}
		}
}

static void DrawUserList(HANDLE h, const std::vector<Contributor> &c_vec, const char *OptionStr[9 + 1], std::pair<SHORT, SHORT> OptionPos[9 + 1], const std::pair<SHORT, SHORT> &sidex, const SMALL_RECT &srW, int page)
{
	enum UListPos
	{
		HEADER = 0,
		NAME,
		LEVEL,
		EXP,
		LP,
		CON,
		SORT,
		FILTER,
		BACK,
		INFO
	};
	DWORD written;
	UserType utype = USERTYPE_P;
	int user_per_page = OptionPos[SORT].second - OptionPos[NAME].second;
	int add = user_per_page * (page - 1);

	auto beg = c_vec.cbegin(), end = c_vec.cend();

	if (c_vec.size() >= add)
		beg += add;

	for (SHORT row = 0; row < srW.Bottom; row++)
		for (SHORT col = 0; col < srW.Right; col++)
		{
			if (col == OptionPos[HEADER].first && row == OptionPos[HEADER].second)
			{
				std::string header_str = "CONTRIBUTORS";
				WriteConsoleOutputCharacter(h, header_str.c_str(), header_str.size(), {OptionPos[HEADER].first - static_cast<SHORT>(header_str.size()) / 2, OptionPos[HEADER].second}, &written);
			}
			else if (col == OptionPos[CON].first && row == OptionPos[CON].second)
			{
				WriteConsoleOutputCharacter(h, OptionStr[CON], std::strlen(OptionStr[CON]), {OptionPos[CON].first - static_cast<SHORT>(std::strlen(OptionStr[CON])) / 2, OptionPos[CON].second}, &written);
			}
		}

	for (SHORT row = OptionPos[NAME].second + 1; row < OptionPos[SORT].second - 1; row++)
		WriteConsoleOutputCharacter(h, space_array, sidex.second - sidex.first, {sidex.first, row}, &written);

	for (SHORT row = OptionPos[NAME].second + 1; beg != end && row < OptionPos[SORT].second - 1; row++, beg++)
		for (SHORT col = 0; col < srW.Right; col++)
		{
			if (col == OptionPos[NAME].first)
			{
				std::string name = (*beg).get_user_name();
				WriteConsoleOutputCharacter(h, name.c_str(), name.size(), {OptionPos[NAME].first - static_cast<SHORT>(name.size()) / 2, row}, &written);
			}
			else if (col == OptionPos[LEVEL].first)
			{
				std::string level_str = std::to_string((*beg).get_level());
				WriteConsoleOutputCharacter(h, level_str.c_str(), level_str.size(), {OptionPos[LEVEL].first - static_cast<SHORT>(level_str.size()) / 2, row}, &written);
			}
			else if (col == OptionPos[CON].first)
			{
				std::string xp_str = std::to_string(static_cast<int>((*beg).get_word_contributed()));
				WriteConsoleOutputCharacter(h, xp_str.c_str(), xp_str.size(), {OptionPos[CON].first - static_cast<SHORT>(xp_str.size()) / 2, row}, &written);
			}
		}
}

static DWORD ShowSortMsgBox(HANDLE hOut, HANDLE hIn, const std::pair<SHORT, SHORT> &sidex, SMALL_RECT srW, UserType utype, SortSelection &s_sel)
{
	enum SortMsgBox
	{
		NAME,
		LEVEL,
		EXP,
		PASS,
		CON
	};
	SHORT sidex_diff = sidex.second - sidex.first;
	SHORT x = sidex.first + sidex_diff / 3;
	SHORT y = srW.Bottom / 3;
	SHORT dx = sidex_diff / 3;
	SHORT dy = srW.Bottom / 3;

	const char *OptionStr[CON + 1] =
	{
		"NAME",
		"LV",
		"XP",
		"PASSED",
		"WORD"};

	std::pair<SHORT, SHORT> OptionPos[CON + 1] =
		{
			{x + dx * 1 / 5 - static_cast<SHORT>(std::strlen(OptionStr[NAME])) / 2, y + dy * 1 / 2},  // NAME
			{x + dx * 2 / 5 - static_cast<SHORT>(std::strlen(OptionStr[LEVEL])) / 2, y + dy * 1 / 2}, // LEVEL
			{x + dx * 3 / 5 - static_cast<SHORT>(std::strlen(OptionStr[EXP])) / 2, y + dy * 1 / 2},   // EXP
			{x + dx * 4 / 5 - static_cast<SHORT>(std::strlen(OptionStr[PASS])) / 2, y + dy * 1 / 2},  // PASS
			{x + dx * 3 / 5 - static_cast<SHORT>(std::strlen(OptionStr[CON])) / 2, y + dy * 1 / 2},   // CON
		};


	DWORD written;

	for (SHORT row = 0; row < srW.Bottom; row++)
		for (SHORT col = 0; col < srW.Right; col++)
		{
			if ((col == x || col == x + dx) && (row >= y && row <= y + dy))
				WriteConsoleOutputCharacter(hOut, "#", 1, {col, row}, &written);
			else if ((row == y || row == y + dy) && (col >= x && col <= x + dx))
				WriteConsoleOutputCharacter(hOut, "#", 1, {col, row}, &written);
			else if (col > x && col < x + dx && row > y && row < y + dy)
				WriteConsoleOutputCharacter(hOut, " ", 1, {col, row}, &written);
		}

	for (SHORT row = 0; row < srW.Bottom; row++)
		for (SHORT col = 0; col < srW.Right; col++)
		{
			if (col == OptionPos[NAME].first && row == OptionPos[NAME].second)
				WriteConsoleOutputCharacter(hOut, OptionStr[NAME], std::strlen(OptionStr[NAME]), {col, row}, &written);
			else if (col == OptionPos[LEVEL].first && row == OptionPos[LEVEL].second)
				WriteConsoleOutputCharacter(hOut, OptionStr[LEVEL], std::strlen(OptionStr[LEVEL]), {col, row}, &written);

			if (utype == USERTYPE_P)
			{
				if (col == OptionPos[EXP].first && row == OptionPos[EXP].second)
					WriteConsoleOutputCharacter(hOut, OptionStr[EXP], std::strlen(OptionStr[EXP]), {col, row}, &written);
				else if (col == OptionPos[PASS].first && row == OptionPos[PASS].second)
					WriteConsoleOutputCharacter(hOut, OptionStr[PASS], std::strlen(OptionStr[PASS]), {col, row}, &written);
			}
			else
			{
				if (col == OptionPos[CON].first && row == OptionPos[CON].second)
					WriteConsoleOutputCharacter(hOut, OptionStr[CON], std::strlen(OptionStr[CON]), {col, row}, &written);
			}
		}

	bool is_break = false;
	auto inc = [&](SortMsgBox &a) {
		a = static_cast<SortMsgBox>(a + 1) > CON ? NAME : static_cast<SortMsgBox>(a + 1);
		if (utype == USERTYPE_C)
			a = (a == EXP) ? CON : a;
		else
			a = (a == CON) ? NAME : a;
	};
	auto dec = [&](SortMsgBox &a) {
		a = static_cast<SortMsgBox>(a - 1) < NAME ? CON : static_cast<SortMsgBox>(a - 1);
		if (utype == USERTYPE_C)
			a = (a == PASS) ? LEVEL : a;
		else
			a = (a == CON) ? PASS : a;
	};
	SortMsgBox mgpos = NAME;
	WriteConsoleOutputAttribute(hOut, attribute_fwhite, std::strlen(OptionStr[mgpos]), {OptionPos[mgpos].first, OptionPos[mgpos].second}, &written);

	INPUT_RECORD irKb[12];
	DWORD wNumber;

	while (!is_break)
	{
		if (ReadConsoleInput(hIn, irKb, 12, &wNumber) == FALSE)
		{
			DWORD last_error = GetLastError();
			ErrorMsg("Show welcome page failed", last_error);
			Log::WriteLog(std::string("ConIO: Show welcome page failed: cannot read console input, errorcode: ") + std::to_string(last_error));
			return last_error;
		}

		for (DWORD i = 0; i < wNumber; i++)
		{
			if (irKb[i].EventType == KEY_EVENT && irKb[i].Event.KeyEvent.bKeyDown)
			{
				switch (irKb[i].Event.KeyEvent.wVirtualKeyCode)
				{
				case VK_LEFT:
					WriteConsoleOutputAttribute(hOut, attribute_bwhite, std::strlen(OptionStr[mgpos]), {OptionPos[mgpos].first, OptionPos[mgpos].second}, &written);
					dec(mgpos);
					WriteConsoleOutputAttribute(hOut, attribute_fwhite, std::strlen(OptionStr[mgpos]), {OptionPos[mgpos].first, OptionPos[mgpos].second}, &written);
					break;
				case VK_RIGHT:
					WriteConsoleOutputAttribute(hOut, attribute_bwhite, std::strlen(OptionStr[mgpos]), {OptionPos[mgpos].first, OptionPos[mgpos].second}, &written);
					inc(mgpos);
					WriteConsoleOutputAttribute(hOut, attribute_fwhite, std::strlen(OptionStr[mgpos]), {OptionPos[mgpos].first, OptionPos[mgpos].second}, &written);
					break;
				case VK_RETURN:
					s_sel = static_cast<SortSelection>(mgpos - NAME);
					WriteConsoleOutputAttribute(hOut, attribute_bwhite, std::strlen(OptionStr[mgpos]), {OptionPos[mgpos].first, OptionPos[mgpos].second}, &written);
					is_break = true;
					break;
				default:
					break;
				}
			}
		}
	}
	return ERROR_SUCCESS;
}

static DWORD ShowFilterMsgBox(HANDLE hOut, HANDLE hIn, const std::pair<SHORT, SHORT> &sidex, SMALL_RECT srW, UserType utype, FilterPack &f_pack)
{
	enum SortMsgBox
	{
		LU = 0,
		RU,
		LD,
		RD,
		NAME,
		LEVEL,
		EXP,
		PASS,
		CON,
		INBOX
	};
	SHORT sidex_diff = sidex.second - sidex.first;
	SHORT x = sidex.first + sidex_diff / 4;
	SHORT y = srW.Bottom / 3;
	SHORT dx = sidex_diff / 2;
	SHORT dy = srW.Bottom / 3;

	const char *OptionStr[INBOX + 1] =
		{
			"",
			"",
			"",
			"",
			"NAME",
			"LV",
			"XP",
			"PASSED",
			"WORD",
			"Value"};
	std::pair<SHORT, SHORT> OptionPos[INBOX + 1] =
		{
			{sidex.first + sidex_diff / 4, srW.Bottom / 3},											  // LU
			{sidex.first + sidex_diff * 3 / 4, srW.Bottom / 3},										  // RU
			{sidex.first + sidex_diff / 4, srW.Bottom * 2 / 3},										  // LD
			{sidex.first + sidex_diff * 3 / 4, srW.Bottom * 2 / 3},									  // RD
			{x + dx * 1 / 5 - static_cast<SHORT>(std::strlen(OptionStr[NAME])) / 2, y + dy * 1 / 3},  // NAME
			{x + dx * 2 / 5 - static_cast<SHORT>(std::strlen(OptionStr[LEVEL])) / 2, y + dy * 1 / 3}, // LEVEL
			{x + dx * 3 / 5 - static_cast<SHORT>(std::strlen(OptionStr[EXP])) / 2, y + dy * 1 / 3},   // EXP
			{x + dx * 4 / 5 - static_cast<SHORT>(std::strlen(OptionStr[PASS])) / 2, y + dy * 1 / 3},  // PASS
			{x + dx * 3 / 5 - static_cast<SHORT>(std::strlen(OptionStr[CON])) / 2, y + dy * 1 / 3},   // CON
			{x + dx * 1 / 5, y + dy * 3 / 4},														  // INBOX
		};

	DWORD written;

	for (SHORT row = 0; row < srW.Bottom; row++)
		for (SHORT col = 0; col < srW.Right; col++)
		{
			if ((col == x || col == x + dx) && (row >= y && row <= y + dy))
				WriteConsoleOutputCharacter(hOut, "#", 1, {col, row}, &written);
			else if ((row == y || row == y + dy) && (col >= x && col <= x + dx))
				WriteConsoleOutputCharacter(hOut, "#", 1, {col, row}, &written);
			else if (col > x && col < x + dx && row > y && row < y + dy)
				WriteConsoleOutputCharacter(hOut, " ", 1, {col, row}, &written);
		}

	for (SHORT row = 0; row < srW.Bottom; row++)
		for (SHORT col = 0; col < srW.Right; col++)
		{
			if (col == OptionPos[NAME].first && row == OptionPos[NAME].second)
				WriteConsoleOutputCharacter(hOut, OptionStr[NAME], std::strlen(OptionStr[NAME]), {col, row}, &written);
			else if (col == OptionPos[LEVEL].first && row == OptionPos[LEVEL].second)
				WriteConsoleOutputCharacter(hOut, OptionStr[LEVEL], std::strlen(OptionStr[LEVEL]), {col, row}, &written);
			else if (col == OptionPos[INBOX].first && row == OptionPos[INBOX].second)
				WriteConsoleOutputCharacter(hOut, OptionStr[INBOX], std::strlen(OptionStr[INBOX]), {col - static_cast<SHORT>(std::strlen(OptionStr[INBOX])), row}, &written);

			if (utype == USERTYPE_P)
			{
				if (col == OptionPos[EXP].first && row == OptionPos[EXP].second)
					WriteConsoleOutputCharacter(hOut, OptionStr[EXP], std::strlen(OptionStr[EXP]), {col, row}, &written);
				else if (col == OptionPos[PASS].first && row == OptionPos[PASS].second)
					WriteConsoleOutputCharacter(hOut, OptionStr[PASS], std::strlen(OptionStr[PASS]), {col, row}, &written);
			}
			else
			{
				if (col == OptionPos[CON].first && row == OptionPos[CON].second)
					WriteConsoleOutputCharacter(hOut, OptionStr[CON], std::strlen(OptionStr[CON]), {col, row}, &written);
			}
		}

	bool is_break = false;
	auto inc = [&](SortMsgBox &a) {
		a = static_cast<SortMsgBox>(a + 1) > CON ? NAME : static_cast<SortMsgBox>(a + 1);
		if (utype == USERTYPE_C)
			a = (a == EXP) ? CON : a;
		else
			a = (a == CON) ? NAME : a;
	};
	auto dec = [&](SortMsgBox &a) {
		a = static_cast<SortMsgBox>(a - 1) < NAME ? CON : static_cast<SortMsgBox>(a - 1);
		if (utype == USERTYPE_C)
			a = (a == PASS) ? LEVEL : a;
		else
			a = (a == CON) ? PASS : a;
	};
	SortMsgBox mgpos = NAME;
	WriteConsoleOutputAttribute(hOut, attribute_fwhite, std::strlen(OptionStr[mgpos]), {OptionPos[mgpos].first, OptionPos[mgpos].second}, &written);

	std::string inbox_str;
	INPUT_RECORD irKb[12];
	DWORD wNumber;

	while (!is_break)
	{
		if (ReadConsoleInput(hIn, irKb, 12, &wNumber) == FALSE)
		{
			DWORD last_error = GetLastError();
			ErrorMsg("Show welcome page failed", last_error);
			Log::WriteLog(std::string("ConIO: Show welcome page failed: cannot read console input, errorcode: ") + std::to_string(last_error));
			return last_error;
		}

		for (DWORD i = 0; i < wNumber; i++)
		{
			if (irKb[i].EventType == KEY_EVENT && irKb[i].Event.KeyEvent.bKeyDown)
			{
				if (std::isalnum(irKb[i].Event.KeyEvent.uChar.AsciiChar))
				{
					if (inbox_str.size() <= 16)
					{
						char temp = irKb[i].Event.KeyEvent.uChar.AsciiChar;
						inbox_str.push_back(temp);
						WriteConsoleOutputCharacter(hOut, &temp, 1, {OptionPos[INBOX].first + 1 + static_cast<SHORT>(inbox_str.size()), OptionPos[INBOX].second}, &written);
						SetConsoleCursorPosition(hOut, {OptionPos[INBOX].first + 2 + static_cast<SHORT>(inbox_str.size()), OptionPos[INBOX].second});
					}
				}
				switch (irKb[i].Event.KeyEvent.wVirtualKeyCode)
				{
				case VK_LEFT:
					if (mgpos != INBOX)
					{
						WriteConsoleOutputAttribute(hOut, attribute_bwhite, std::strlen(OptionStr[mgpos]), {OptionPos[mgpos].first, OptionPos[mgpos].second}, &written);
						dec(mgpos);
						WriteConsoleOutputAttribute(hOut, attribute_fwhite, std::strlen(OptionStr[mgpos]), {OptionPos[mgpos].first, OptionPos[mgpos].second}, &written);
					}
					break;
				case VK_RIGHT:
					if (mgpos != INBOX)
					{
						WriteConsoleOutputAttribute(hOut, attribute_bwhite, std::strlen(OptionStr[mgpos]), {OptionPos[mgpos].first, OptionPos[mgpos].second}, &written);
						inc(mgpos);
						WriteConsoleOutputAttribute(hOut, attribute_fwhite, std::strlen(OptionStr[mgpos]), {OptionPos[mgpos].first, OptionPos[mgpos].second}, &written);
					}
					break;
				case VK_RETURN:
					if (mgpos != INBOX)
					{
						set_cursor_visible(hOut, TRUE);
						inbox_str.clear();
						SetConsoleCursorPosition(hOut, {OptionPos[INBOX].first + 1, OptionPos[INBOX].second});
						WriteConsoleOutputAttribute(hOut, attribute_bwhite, std::strlen(OptionStr[mgpos]), {OptionPos[mgpos].first, OptionPos[mgpos].second}, &written);
						f_pack.type = static_cast<FilterPack::FilterPackType>(mgpos - NAME);
						mgpos = INBOX;
					}
					else
					{
						if (inbox_str.size())
						{
							switch (f_pack.type)
							{
							case FilterPack::FPT_NAME:
								f_pack.name = inbox_str;
								is_break = true;
								set_cursor_visible(hOut, FALSE);
								break;
							case FilterPack::FPT_LV:
							case FilterPack::FPT_CON:
							case FilterPack::FPT_PASS:
								try
								{
									f_pack.integer = std::stoi(inbox_str);
									is_break = true;
									set_cursor_visible(hOut, FALSE);
								}
								catch (const std::exception &e)
								{
									Log::WriteLog(e.what());
								}
								break;
							case FilterPack::FPT_EXP:
								try
								{
									f_pack.exp = std::stoi(inbox_str);
									is_break = true;
									set_cursor_visible(hOut, FALSE);
								}
								catch (const std::exception &e)
								{
									Log::WriteLog(e.what());
								}
								break;
							default:
								break;
							}
						}
					}
					break;
				case VK_BACK:
					if (mgpos == INBOX)
					{
						if (inbox_str.size())
						{
							WriteConsoleOutputCharacter(hOut, " ", 1, {OptionPos[INBOX].first + 1 + static_cast<SHORT>(inbox_str.size()), OptionPos[INBOX].second}, &written);
							inbox_str.pop_back();
							SetConsoleCursorPosition(hOut, {OptionPos[INBOX].first + 2 + static_cast<SHORT>(inbox_str.size()), OptionPos[INBOX].second});
						}
					}
					break;
				default:
					break;
				}
			}
		}
	}
	return ERROR_SUCCESS;
}

DWORD ConsoleIO::to_user_list_page(NextPage &next_page)
{
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	DWORD dwOldConsoleMode;

	enum UListPos
	{
		HEADER = 0,
		NAME,
		LEVEL,
		EXP,
		LP,
		CON,
		SORT,
		FILTER,
		BACK,
		INFO
	};

	if (hStdOut == INVALID_HANDLE_VALUE || hStdIn == INVALID_HANDLE_VALUE)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Show menu failed", last_error);
		Log::WriteLog(std::string("ConIO: Show menu failed: cannot get standard output handle, errorcode: ") + std::to_string(last_error));
		return last_error;
	}

	if (GetConsoleScreenBufferInfo(hStdOut, &csbi) == FALSE)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Show menu failed", last_error);
		Log::WriteLog(std::string("ConIO: Show menu failed: cannot get console screen buffer info, errorcode: ") + std::to_string(last_error));
		return last_error;
	}

	set_cursor_visible(hStdOut, FALSE);
	GetConsoleMode(hStdOut, &dwOldConsoleMode);
	SetConsoleMode(hStdOut, ENABLE_PROCESSED_INPUT);

	DWORD written;
	SMALL_RECT srW = csbi.srWindow;

	SHORT x = srW.Right / 8;
	SHORT y = srW.Top;
	SHORT dx = srW.Right * 3 / 4;
	SHORT dy = srW.Bottom - 1 - y;
	const char *OptionStr[INFO + 1] =
		{
			"",
			"Name",
			"Level",
			"XP",
			"Passed",
			"Words",
			"Sort",
			"Filter",
			"Back",
			""};

	std::pair<SHORT /*X*/, SHORT /*Y*/> OptionPos[] =
		{
			{x + dx * 1 / 2, y + 1},															   // HEADER
			{x + dx * 1 / 7, y + 3},															   // NAME
			{x + dx * 1 / 3, y + 3},															   // LEVEL
			{x + dx * 3 / 5, y + 3},															   // EXP
			{x + dx * 4 / 5, y + 3},															   // LP
			{x + dx * 3 / 5, y + 3},															   // CON
			{x + dx * 1 / 4 - static_cast<SHORT>(std::strlen(OptionStr[SORT])) / 2, y + dy - 4},   // SORT
			{x + dx * 2 / 4 - static_cast<SHORT>(std::strlen(OptionStr[FILTER])) / 2, y + dy - 4}, // FILTER
			{x + dx * 3 / 4 - static_cast<SHORT>(std::strlen(OptionStr[BACK])) / 2, y + dy - 4},   // BACK
			{x + dx * 1 / 2, y + dy - 2},														   // INFO
		};

	for (SHORT row = 0; row < srW.Bottom; row++)
		for (SHORT col = 0; col < srW.Right; col++)
		{
			if (col < x || col > x + dx)
				WriteConsoleOutputCharacter(hStdOut, "%", 1, {col, row}, &written);
			else if (col == x || col == x + dx)
				WriteConsoleOutputCharacter(hStdOut, "#", 1, {col, row}, &written);
			else if (row == y || row == y + dy)
				WriteConsoleOutputCharacter(hStdOut, "$", 1, {col, row}, &written);

			if (col == OptionPos[NAME].first && row == OptionPos[NAME].second)
			{
				WriteConsoleOutputCharacter(hStdOut, OptionStr[NAME], std::strlen(OptionStr[NAME]), {OptionPos[NAME].first - static_cast<SHORT>(std::strlen(OptionStr[NAME])) / 2, OptionPos[NAME].second}, &written);
			}
			else if (col == OptionPos[LEVEL].first && row == OptionPos[LEVEL].second)
			{
				WriteConsoleOutputCharacter(hStdOut, OptionStr[LEVEL], std::strlen(OptionStr[LEVEL]), {OptionPos[LEVEL].first - static_cast<SHORT>(std::strlen(OptionStr[LEVEL])) / 2, OptionPos[LEVEL].second}, &written);
			}
			else if (col == OptionPos[SORT].first && row == OptionPos[SORT].second)
			{
				WriteConsoleOutputCharacter(hStdOut, OptionStr[SORT], std::strlen(OptionStr[SORT]), {OptionPos[SORT].first, OptionPos[SORT].second}, &written);
			}
			else if (col == OptionPos[FILTER].first && row == OptionPos[FILTER].second)
			{
				WriteConsoleOutputCharacter(hStdOut, OptionStr[FILTER], std::strlen(OptionStr[FILTER]), {OptionPos[FILTER].first, OptionPos[FILTER].second}, &written);
			}
			else if (col == OptionPos[BACK].first && row == OptionPos[BACK].second)
			{
				WriteConsoleOutputCharacter(hStdOut, OptionStr[BACK], std::strlen(OptionStr[BACK]), {OptionPos[BACK].first, OptionPos[BACK].second}, &written);
			}
			else if (col == OptionPos[INFO].first && row == OptionPos[INFO].second)
			{
				std::string msg = "PgUp/PgDn : goto next page    ArrowUp/ArrowDn : switch from different usertype";
				WriteConsoleOutputCharacter(hStdOut, msg.c_str(), msg.size(), {OptionPos[INFO].first - static_cast<SHORT>(msg.size()) / 2, OptionPos[INFO].second}, &written);
			}
		}
	UserType utype = USERTYPE_C;
	int current_page = 1;
	int user_per_page = OptionPos[SORT].second - OptionPos[NAME].second;
	int max_p_page = acc_sys->get_user_player_number() / user_per_page + 1;
	int max_c_page = acc_sys->get_user_con_number() / user_per_page + 1;

	auto cb = acc_sys->get_contributors_cbegin(), ce = acc_sys->get_contributors_cend();
	auto pb = acc_sys->get_players_cbegin(), pe = acc_sys->get_players_cend();
	std::vector<Player> p_vec(pb, pe);
	std::vector<Contributor> c_vec(cb, ce);
	std::vector<Player> temp_p_vec(pb, pe);
	std::vector<Contributor> temp_c_vec(cb, ce);
	DrawUserList(hStdOut, c_vec, OptionStr, OptionPos, {x + 1, x + dx - 1}, srW, current_page);

	bool is_break = false;
	UListPos ulpos = BACK;
	SortSelection s_sel;
	FilterPack f_pack;
	WriteConsoleOutputAttribute(hStdOut, attribute_fwhite, std::strlen(OptionStr[BACK]), {OptionPos[BACK].first, OptionPos[BACK].second}, &written);

	INPUT_RECORD irKb[12];
	DWORD wNumber;

	while (!is_break)
	{
		if (ReadConsoleInput(hStdIn, irKb, 12, &wNumber) == FALSE)
		{
			DWORD last_error = GetLastError();
			ErrorMsg("Show welcome page failed", last_error);
			Log::WriteLog(std::string("ConIO: Show welcome page failed: cannot read console input, errorcode: ") + std::to_string(last_error));
			return last_error;
		}

		for (DWORD i = 0; i < wNumber; i++)
		{
			if (irKb[i].EventType == KEY_EVENT && irKb[i].Event.KeyEvent.bKeyDown)
			{
				switch (irKb[i].Event.KeyEvent.wVirtualKeyCode)
				{
				case VK_LEFT:
					switch (ulpos)
					{
					case SORT:
						WriteConsoleOutputAttribute(hStdOut, attribute_bwhite, std::strlen(OptionStr[SORT]), {OptionPos[SORT].first, OptionPos[SORT].second}, &written);
						WriteConsoleOutputAttribute(hStdOut, attribute_fwhite, std::strlen(OptionStr[BACK]), {OptionPos[BACK].first, OptionPos[BACK].second}, &written);
						ulpos = BACK;
						break;
					case FILTER:
						WriteConsoleOutputAttribute(hStdOut, attribute_bwhite, std::strlen(OptionStr[FILTER]), {OptionPos[FILTER].first, OptionPos[FILTER].second}, &written);
						WriteConsoleOutputAttribute(hStdOut, attribute_fwhite, std::strlen(OptionStr[SORT]), {OptionPos[SORT].first, OptionPos[SORT].second}, &written);
						ulpos = SORT;
						break;
					case BACK:
						WriteConsoleOutputAttribute(hStdOut, attribute_bwhite, std::strlen(OptionStr[BACK]), {OptionPos[BACK].first, OptionPos[BACK].second}, &written);
						WriteConsoleOutputAttribute(hStdOut, attribute_fwhite, std::strlen(OptionStr[FILTER]), {OptionPos[FILTER].first, OptionPos[FILTER].second}, &written);
						ulpos = FILTER;
						break;
					default:
						break;
					}
					break;
				case VK_RIGHT:
					switch (ulpos)
					{
					case SORT:
						WriteConsoleOutputAttribute(hStdOut, attribute_bwhite, std::strlen(OptionStr[SORT]), {OptionPos[SORT].first, OptionPos[SORT].second}, &written);
						WriteConsoleOutputAttribute(hStdOut, attribute_fwhite, std::strlen(OptionStr[FILTER]), {OptionPos[FILTER].first, OptionPos[FILTER].second}, &written);
						ulpos = FILTER;
						break;
					case FILTER:
						WriteConsoleOutputAttribute(hStdOut, attribute_bwhite, std::strlen(OptionStr[FILTER]), {OptionPos[FILTER].first, OptionPos[FILTER].second}, &written);
						WriteConsoleOutputAttribute(hStdOut, attribute_fwhite, std::strlen(OptionStr[BACK]), {OptionPos[BACK].first, OptionPos[BACK].second}, &written);
						ulpos = BACK;
						break;
					case BACK:
						WriteConsoleOutputAttribute(hStdOut, attribute_bwhite, std::strlen(OptionStr[BACK]), {OptionPos[BACK].first, OptionPos[BACK].second}, &written);
						WriteConsoleOutputAttribute(hStdOut, attribute_fwhite, std::strlen(OptionStr[SORT]), {OptionPos[SORT].first, OptionPos[SORT].second}, &written);
						ulpos = SORT;
						break;
					default:
						break;
					}
					break;
				case VK_UP:
				case VK_DOWN:
					current_page = 1;
					utype = (utype == USERTYPE_C) ? USERTYPE_P : USERTYPE_C;
					if (utype == USERTYPE_C)
					{
						WriteConsoleOutputCharacter(hStdOut, space_array, (dx - 2), {x + 1, OptionPos[HEADER].second}, &written);
						WriteConsoleOutputCharacter(hStdOut, space_array, (dx - 2) / 2, {x + 1 + (dx - 2) / 2, OptionPos[LEVEL].second}, &written);
						DrawUserList(hStdOut, std::vector<Contributor>(cb, ce), OptionStr, OptionPos, {x + 1, x + dx - 1}, srW, current_page);
					}
					else
					{
						WriteConsoleOutputCharacter(hStdOut, space_array, (dx - 2), {x + 1, OptionPos[HEADER].second}, &written);
						WriteConsoleOutputCharacter(hStdOut, space_array, (dx - 2) / 2, {x + 1 + (dx - 2) / 2, OptionPos[LEVEL].second}, &written);
						DrawUserList(hStdOut, std::vector<Player>(pb, pe), OptionStr, OptionPos, {x + 1, x + dx - 1}, srW, current_page);
					}
					break;
				case VK_PRIOR:
					if (current_page > 1)
					{
						current_page--;
						if (utype == USERTYPE_C)
						{
							WriteConsoleOutputCharacter(hStdOut, space_array, (dx - 2), {x + 1, OptionPos[HEADER].second}, &written);
							WriteConsoleOutputCharacter(hStdOut, space_array, (dx - 2) / 2, {x + 1 + (dx - 2) / 2, OptionPos[LEVEL].second}, &written);
							DrawUserList(hStdOut, std::vector<Contributor>(cb, ce), OptionStr, OptionPos, {x + 1, x + dx - 1}, srW, current_page);
						}
						else
						{
							WriteConsoleOutputCharacter(hStdOut, space_array, (dx - 2), {x + 1, OptionPos[HEADER].second}, &written);
							WriteConsoleOutputCharacter(hStdOut, space_array, (dx - 2) / 2, {x + 1 + (dx - 2) / 2, OptionPos[LEVEL].second}, &written);
							DrawUserList(hStdOut, std::vector<Player>(pb, pe), OptionStr, OptionPos, {x + 1, x + dx - 1}, srW, current_page);
						}
					}
					break;
				case VK_NEXT:
					if ((static_cast<long long>(user_per_page) * (current_page)) < ((utype == USERTYPE_C) ? acc_sys->get_user_con_number() : acc_sys->get_user_player_number()))
						current_page++;
					if (utype == USERTYPE_C)
					{
						WriteConsoleOutputCharacter(hStdOut, space_array, (dx - 2), {x + 1, OptionPos[HEADER].second}, &written);
						WriteConsoleOutputCharacter(hStdOut, space_array, (dx - 2) / 2, {x + 1 + (dx - 2) / 2, OptionPos[LEVEL].second}, &written);
						DrawUserList(hStdOut, std::vector<Contributor>(cb, ce), OptionStr, OptionPos, {x + 1, x + dx - 1}, srW, current_page);
					}
					else
					{
						WriteConsoleOutputCharacter(hStdOut, space_array, (dx - 2), {x + 1, OptionPos[HEADER].second}, &written);
						WriteConsoleOutputCharacter(hStdOut, space_array, (dx - 2) / 2, {x + 1 + (dx - 2) / 2, OptionPos[LEVEL].second}, &written);
						DrawUserList(hStdOut, std::vector<Player>(pb, pe), OptionStr, OptionPos, {x + 1, x + dx - 1}, srW, current_page);
					}
					break;
				case VK_RETURN:
					switch (ulpos)
					{
					case SORT:
						Shine(hStdOut, std::strlen(OptionStr[SORT]), {OptionPos[SORT].first, OptionPos[SORT].second});
						ShowSortMsgBox(hStdOut, hStdIn, {x + 1, x + dx - 1}, srW, utype, s_sel);
						if (utype == USERTYPE_C)
						{
							current_page = 1;
							c_vec.clear();
							std::copy(cb, ce, std::back_inserter(c_vec));
						}
						else
						{
							current_page = 1;
							p_vec.clear();
							std::copy(pb, pe, std::back_inserter(p_vec));
						}
						WriteConsoleOutputCharacter(hStdOut, space_array, (dx - 2), {x + 1, OptionPos[HEADER].second}, &written);
						WriteConsoleOutputCharacter(hStdOut, space_array, (dx - 2) / 2, {x + 1 + (dx - 2) / 2, OptionPos[LEVEL].second}, &written);
						switch (s_sel)
						{
						case SORTS_NAME:
							if (utype == USERTYPE_C)
							{
								std::sort(c_vec.begin(), c_vec.end(), [](const Contributor &a, const Contributor &b) { return a.get_user_name() < b.get_user_name(); });
								DrawUserList(hStdOut, c_vec, OptionStr, OptionPos, {x + 1, x + dx - 1}, srW, current_page);
							}
							else
							{
								std::sort(p_vec.begin(), p_vec.end(), [](const Player &a, const Player &b) { return a.get_user_name() < b.get_user_name(); });
								DrawUserList(hStdOut, p_vec, OptionStr, OptionPos, {x + 1, x + dx - 1}, srW, current_page);
							}
							break;
						case SORTS_LEVEL:
							if (utype == USERTYPE_C)
							{
								std::sort(c_vec.rbegin(), c_vec.rend(), [](const Contributor &a, const Contributor &b) { return a.get_level() < b.get_level(); });
								DrawUserList(hStdOut, c_vec, OptionStr, OptionPos, {x + 1, x + dx - 1}, srW, current_page);
							}
							else
							{
								std::sort(p_vec.rbegin(), p_vec.rend(), [](const Player &a, const Player &b) { return a.get_level() < b.get_level(); });
								DrawUserList(hStdOut, p_vec, OptionStr, OptionPos, {x + 1, x + dx - 1}, srW, current_page);
							}
							break;
						case SORTS_EXP:
							std::sort(p_vec.rbegin(), p_vec.rend(), [](const Player &a, const Player &b) { return a.get_exp() < b.get_exp(); });
							DrawUserList(hStdOut, p_vec, OptionStr, OptionPos, {x + 1, x + dx - 1}, srW, current_page);
							break;
						case SORTS_PASS:
							std::sort(p_vec.rbegin(), p_vec.rend(), [](const Player &a, const Player &b) { return a.get_level_passed() < b.get_level_passed(); });
							DrawUserList(hStdOut, p_vec, OptionStr, OptionPos, {x + 1, x + dx - 1}, srW, current_page);
							break;
						case SORTS_CON:
							std::sort(c_vec.rbegin(), c_vec.rend(), [](const Contributor &a, const Contributor &b) { return a.get_word_contributed() < b.get_word_contributed(); });
							DrawUserList(hStdOut, c_vec, OptionStr, OptionPos, {x + 1, x + dx - 1}, srW, current_page);
							break;
						default:
							break;
						}
						break;
					case FILTER:
						Shine(hStdOut, std::strlen(OptionStr[FILTER]), {OptionPos[FILTER].first, OptionPos[FILTER].second});
						ShowFilterMsgBox(hStdOut, hStdIn, {x + 1, x + dx - 1}, srW, utype, f_pack);
						if (utype == USERTYPE_C)
						{
							current_page = 1;
							c_vec.clear();
							std::copy(cb, ce, std::back_inserter(c_vec));
						}
						else
						{
							current_page = 1;
							p_vec.clear();
							std::copy(pb, pe, std::back_inserter(p_vec));
						}
						WriteConsoleOutputCharacter(hStdOut, space_array, (dx - 2), {x + 1, OptionPos[HEADER].second}, &written);
						WriteConsoleOutputCharacter(hStdOut, space_array, (dx - 2) / 2, {x + 1 + (dx - 2) / 2, OptionPos[LEVEL].second}, &written);
						switch (f_pack.type)
						{
						case FilterPack::FPT_NAME:
							if (utype == USERTYPE_C)
							{
								temp_c_vec.clear();
								std::for_each(c_vec.begin(), c_vec.end(), [&](const Contributor &a) { if (a.get_user_name() == f_pack.name) temp_c_vec.push_back(a); });
								DrawUserList(hStdOut, temp_c_vec, OptionStr, OptionPos, {x + 1, x + dx - 1}, srW, current_page);
							}
							else
							{
								temp_p_vec.clear();
								std::for_each(p_vec.begin(), p_vec.end(), [&](const Player &a) { if (a.get_user_name() == f_pack.name) temp_p_vec.push_back(a); });
								DrawUserList(hStdOut, temp_p_vec, OptionStr, OptionPos, {x + 1, x + dx - 1}, srW, current_page);
							}
							break;
						case FilterPack::FPT_LV:
							if (utype == USERTYPE_C)
							{
								temp_c_vec.clear();
								std::for_each(c_vec.begin(), c_vec.end(), [&](const Contributor &a) { if (a.get_level() == f_pack.integer) temp_c_vec.push_back(a); });
								DrawUserList(hStdOut, temp_c_vec, OptionStr, OptionPos, {x + 1, x + dx - 1}, srW, current_page);
							}
							else
							{
								temp_p_vec.clear();
								std::for_each(p_vec.begin(), p_vec.end(), [&](const Player &a) { if (a.get_level() == f_pack.integer) temp_p_vec.push_back(a); });
								DrawUserList(hStdOut, temp_p_vec, OptionStr, OptionPos, {x + 1, x + dx - 1}, srW, current_page);
							}
							break;
						case FilterPack::FPT_CON:
							temp_c_vec.clear();
							std::for_each(c_vec.begin(), c_vec.end(), [&](const Contributor &a) { if (a.get_word_contributed() == f_pack.integer) temp_c_vec.push_back(a); });
							DrawUserList(hStdOut, temp_c_vec, OptionStr, OptionPos, {x + 1, x + dx - 1}, srW, current_page);
							break;
						case FilterPack::FPT_PASS:
							temp_p_vec.clear();
							std::for_each(p_vec.begin(), p_vec.end(), [&](const Player &a) { if (a.get_level_passed() == f_pack.integer) temp_p_vec.push_back(a); });
							DrawUserList(hStdOut, temp_p_vec, OptionStr, OptionPos, {x + 1, x + dx - 1}, srW, current_page);
							break;
						case FilterPack::FPT_EXP:
							temp_p_vec.clear();
							std::for_each(p_vec.begin(), p_vec.end(), [&](const Player &a) { if (static_cast<int>(a.get_exp()) == static_cast<int>(f_pack.exp)) temp_p_vec.push_back(a); });
							DrawUserList(hStdOut, temp_p_vec, OptionStr, OptionPos, {x + 1, x + dx - 1}, srW, current_page);
							break;
						default:
							break;
						}
						break;
					case BACK:
						next_page = P_MENU;
						is_break = true;
						Shine(hStdOut, std::strlen(OptionStr[BACK]), {OptionPos[BACK].first, OptionPos[BACK].second});
						break;
					default:
						break;
					}
					break;
				}
			}
		}
	}

	SetConsoleMode(hStdOut, dwOldConsoleMode);
	set_cursor_visible(hStdOut, TRUE);
	return ERROR_SUCCESS;
}

DWORD ConsoleIO::to_contributor_play_page(NextPage &next_page)
{
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	DWORD dwOldConsoleMode;

	enum CPlayPos
	{
		HEADER = 0,
		INPUT,
		CON,
		BACK,
		INFO
	};

	if (hStdOut == INVALID_HANDLE_VALUE || hStdIn == INVALID_HANDLE_VALUE)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Show menu failed", last_error);
		Log::WriteLog(std::string("ConIO: Show menu failed: cannot get standard output handle, errorcode: ") + std::to_string(last_error));
		return last_error;
	}

	if (GetConsoleScreenBufferInfo(hStdOut, &csbi) == FALSE)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Show menu failed", last_error);
		Log::WriteLog(std::string("ConIO: Show menu failed: cannot get console screen buffer info, errorcode: ") + std::to_string(last_error));
		return last_error;
	}

	set_cursor_visible(hStdOut, FALSE);
	GetConsoleMode(hStdOut, &dwOldConsoleMode);
	SetConsoleMode(hStdOut, ENABLE_PROCESSED_INPUT);

	DWORD written;
	SMALL_RECT srW = csbi.srWindow;

	SHORT x = srW.Right / 5 + 1;
	SHORT y = srW.Top + 1;
	SHORT dx = (srW.Right * 4 / 5 - 1) - x;
	SHORT dy = (srW.Bottom - 1) - y;

	const char *OptionStr[INFO + 1] =
		{
			"Enter your words",
			"Your word",
			"Submit",
			"Back to menu",
			""};

	std::pair<SHORT /*X*/, SHORT /*Y*/> OptionPos[INFO + 1] =
		{
			{x + dx / 2 - static_cast<SHORT>(std::strlen(OptionStr[HEADER])) / 2, y + dy / 4},		 // HEADER
			{x + dx / 3, y + dy * 2 / 4},															 // INPUT
			{x + dx / 3 - static_cast<SHORT>(std::strlen(OptionStr[CON])) / 2, y + dy * 3 / 4},		 // CON
			{x + dx * 2 / 3 - static_cast<SHORT>(std::strlen(OptionStr[BACK])) / 2, y + dy * 3 / 4}, // BACK
			{x + dx / 2, y + dy - 2}																 // INFO
		};

	for (SHORT row = 0; row < srW.Bottom; row++)
		for (SHORT col = 0; col < srW.Right; col++)
		{
			if (col < x || col > x + dx)
				WriteConsoleOutputCharacter(hStdOut, "%", 1, {col, row}, &written);
			else if (col == x || col == x + dx)
				WriteConsoleOutputCharacter(hStdOut, "#", 1, {col, row}, &written);
			else if (row == y || row == y + dy)
				WriteConsoleOutputCharacter(hStdOut, "$", 1, {col, row}, &written);
			else if (row < y || row > y + dy)
				WriteConsoleOutputCharacter(hStdOut, "%", 1, {col, row}, &written);
		}

	for (SHORT row = 0; row < srW.Bottom; row++)
		for (SHORT col = 0; col < srW.Right; col++)
		{
			if (col == OptionPos[HEADER].first && row == OptionPos[HEADER].second)
				WriteConsoleOutputCharacter(hStdOut, OptionStr[HEADER], std::strlen(OptionStr[HEADER]), {col, row}, &written);
			else if (col == OptionPos[INPUT].first && row == OptionPos[INPUT].second)
				WriteConsoleOutputCharacter(hStdOut, OptionStr[INPUT], std::strlen(OptionStr[INPUT]), {col - static_cast<SHORT>(std::strlen(OptionStr[INPUT])), row}, &written);
			else if (col == OptionPos[CON].first && row == OptionPos[CON].second)
				WriteConsoleOutputCharacter(hStdOut, OptionStr[CON], std::strlen(OptionStr[CON]), {col, row}, &written);
			else if (col == OptionPos[BACK].first && row == OptionPos[BACK].second)
				WriteConsoleOutputCharacter(hStdOut, OptionStr[BACK], std::strlen(OptionStr[BACK]), {col, row}, &written);
		}

	UserType utype = USERTYPE_C;

	bool is_break = false;
	CPlayPos cpos = BACK;

	WriteConsoleOutputAttribute(hStdOut, attribute_fwhite, std::strlen(OptionStr[BACK]), {OptionPos[BACK].first, OptionPos[BACK].second}, &written);
	// acc_sys->set_current_user();
	INPUT_RECORD irKb[12];
	std::string input_str;
	std::string name = acc_sys->get_current_user_str();
	DWORD wNumber;

	while (!is_break)
	{
		if (ReadConsoleInput(hStdIn, irKb, 12, &wNumber) == FALSE)
		{
			DWORD last_error = GetLastError();
			ErrorMsg("Show welcome page failed", last_error);
			Log::WriteLog(std::string("ConIO: Show welcome page failed: cannot read console input, errorcode: ") + std::to_string(last_error));
			return last_error;
		}

		for (DWORD i = 0; i < wNumber; i++)
		{
			if (irKb[i].EventType == KEY_EVENT && irKb[i].Event.KeyEvent.bKeyDown)
			{
				if (std::isalpha(irKb[i].Event.KeyEvent.uChar.AsciiChar))
				{
					if (cpos == INPUT && input_str.size() < 26)
					{
						char temp_input = irKb[i].Event.KeyEvent.uChar.AsciiChar;
						WriteConsoleOutputCharacter(hStdOut, &temp_input, 1, {OptionPos[INPUT].first + 1 + static_cast<SHORT>(input_str.size()), OptionPos[INPUT].second}, &written);
						input_str.push_back(temp_input);
						SetConsoleCursorPosition(hStdOut, {OptionPos[INPUT].first + 1 + static_cast<SHORT>(input_str.size()), OptionPos[INPUT].second});
					}
				}
				switch (irKb[i].Event.KeyEvent.wVirtualKeyCode)
				{
				case VK_UP:
					if (cpos == BACK || cpos == CON)
					{
						WriteConsoleOutputAttribute(hStdOut, attribute_bwhite, std::strlen(OptionStr[cpos]), {OptionPos[cpos].first, OptionPos[cpos].second}, &written);
						cpos = INPUT;
						set_cursor_visible(hStdOut, TRUE);
						SetConsoleCursorPosition(hStdOut, {OptionPos[cpos].first + 1 + static_cast<SHORT>(input_str.size()), OptionPos[cpos].second});
					}
					break;
				case VK_DOWN:
					if (cpos == INPUT)
					{
						set_cursor_visible(hStdOut, FALSE);
						cpos = CON;
						WriteConsoleOutputAttribute(hStdOut, attribute_fwhite, std::strlen(OptionStr[cpos]), {OptionPos[cpos].first, OptionPos[cpos].second}, &written);
					}
					break;
				case VK_LEFT:
					if (cpos == INPUT)
					{
						set_cursor_visible(hStdOut, FALSE);
						cpos = CON;
						WriteConsoleOutputAttribute(hStdOut, attribute_fwhite, std::strlen(OptionStr[cpos]), {OptionPos[cpos].first, OptionPos[cpos].second}, &written);
					}
					else
					{
						WriteConsoleOutputAttribute(hStdOut, attribute_bwhite, std::strlen(OptionStr[cpos]), {OptionPos[cpos].first, OptionPos[cpos].second}, &written);
						cpos = (cpos == CON) ? BACK : CON;
						WriteConsoleOutputAttribute(hStdOut, attribute_fwhite, std::strlen(OptionStr[cpos]), {OptionPos[cpos].first, OptionPos[cpos].second}, &written);
					}
					break;
				case VK_RIGHT:
					if (cpos == INPUT)
					{
						set_cursor_visible(hStdOut, FALSE);
						cpos = BACK;
						WriteConsoleOutputAttribute(hStdOut, attribute_fwhite, std::strlen(OptionStr[cpos]), {OptionPos[cpos].first, OptionPos[cpos].second}, &written);
					}
					else
					{
						WriteConsoleOutputAttribute(hStdOut, attribute_bwhite, std::strlen(OptionStr[cpos]), {OptionPos[cpos].first, OptionPos[cpos].second}, &written);
						cpos = (cpos == CON) ? BACK : CON;
						WriteConsoleOutputAttribute(hStdOut, attribute_fwhite, std::strlen(OptionStr[cpos]), {OptionPos[cpos].first, OptionPos[cpos].second}, &written);
					}
					break;
				case VK_RETURN:
					if (cpos == BACK)
					{
						WriteConsoleOutputAttribute(hStdOut, attribute_bwhite, dx - 1, {x + 1, OptionPos[INFO].second}, &written);
						Shine(hStdOut, std::strlen(OptionStr[BACK]), {OptionPos[BACK].first, OptionPos[BACK].second});
						is_break = true;
						next_page = P_MENU;
					}
					else if (cpos == CON)
					{
						if (!input_str.size())
						{
							std::string emp_str = "No word can be added";
							WriteConsoleOutputCharacter(hStdOut, space_array, dx - 1, {x + 1, OptionPos[INFO].second}, &written);
							WriteConsoleOutputAttribute(hStdOut, attribute_bwhite, dx - 1, {x + 1, OptionPos[INFO].second}, &written);
							WriteConsoleOutputCharacter(hStdOut, emp_str.c_str(), static_cast<SHORT>(emp_str.size()), {OptionPos[INFO].first - static_cast<SHORT>(emp_str.size()) / 2, OptionPos[INFO].second}, &written);
							WriteConsoleOutputAttribute(hStdOut, attribute_fred, static_cast<SHORT>(emp_str.size()), {OptionPos[INFO].first - static_cast<SHORT>(emp_str.size()) / 2, OptionPos[INFO].second}, &written);
						}
						else if (word_list->AddWord(input_str, name))
						{
							Shine(hStdOut, std::strlen(OptionStr[CON]), {OptionPos[CON].first, OptionPos[CON].second});
							std::string suc_str = "Add your word into word list successfully.";
							acc_sys->get_ref_contributor(name).inc_word_contributed();
							WriteConsoleOutputCharacter(hStdOut, space_array, dx - 1, {x + 1, OptionPos[INFO].second}, &written);
							WriteConsoleOutputAttribute(hStdOut, attribute_bwhite, dx - 1, {x + 1, OptionPos[INFO].second}, &written);
							WriteConsoleOutputCharacter(hStdOut, suc_str.c_str(), static_cast<SHORT>(suc_str.size()), {OptionPos[INFO].first - static_cast<SHORT>(suc_str.size()) / 2, OptionPos[INFO].second}, &written);
						}
						else
						{
							std::string fail_str = "This word is already existed";
							WriteConsoleOutputCharacter(hStdOut, space_array, dx - 1, {x + 1, OptionPos[INFO].second}, &written);
							WriteConsoleOutputAttribute(hStdOut, attribute_bwhite, dx - 1, {x + 1, OptionPos[INFO].second}, &written);
							WriteConsoleOutputCharacter(hStdOut, fail_str.c_str(), static_cast<SHORT>(fail_str.size()), {OptionPos[INFO].first - static_cast<SHORT>(fail_str.size()) / 2, OptionPos[INFO].second}, &written);
						}
					}
					break;
				case VK_BACK:
					if (cpos == INPUT && input_str.size())
					{
						WriteConsoleOutputCharacter(hStdOut, " ", 1, {OptionPos[INPUT].first + static_cast<SHORT>(input_str.size()), OptionPos[INPUT].second}, &written);
						input_str.pop_back();
						SetConsoleCursorPosition(hStdOut, {OptionPos[INPUT].first + 1 + static_cast<SHORT>(input_str.size()), OptionPos[INPUT].second});
					}
					break;
				default:
					break;
				}
			}
		}
	}

	SetConsoleMode(hStdOut, dwOldConsoleMode);
	set_cursor_visible(hStdOut, TRUE);
	return ERROR_SUCCESS;
}

static DWORD ShowGameHelpMsgBox(HANDLE hOut, HANDLE hIn, SHORT x, SHORT y, SHORT dx, SHORT dy)
{
	DWORD written;
	enum HelpPos
	{
		LINE0,
		OK
	};

	const char *OptionStr[OK + 1] =
		{
			"Type the word you see and prees Enter after countdown",
			"Yes",
		};

	SHORT _x = x + dx / 3,
		  _dx = dx / 3,
		  _y = y + dy / 3,
		  _dy = dy / 3;

	std::pair<SHORT, SHORT> OptionPos[OK + 1] =
		{
			{_x + _dx / 2 - static_cast<SHORT>(std::strlen(OptionStr[LINE0])) / 2, _y + _dy / 3},  // LINE0
			{_x + _dx / 2 - static_cast<SHORT>(std::strlen(OptionStr[OK])) / 2, _y + _dy * 2 / 3}, // OK
		};
	/*
	for (auto row = _y; row <= _y + _dy; row++)
		for (auto col = _x; col <= _x + _dx; col++)
		{
			if (row == _y || row == _y + _dy || col == _x || col == _x + _dx)
				WriteConsoleOutputCharacter(hOut, "#", 1, {col, row}, &written);
			else if (col == _x + 1)
				WriteConsoleOutputCharacter(hOut, space_array, _dx - 1, {col, row}, &written);
		}
	*/
	for (int i = LINE0; i <= OK; i = i + 1)
		WriteConsoleOutputCharacter(hOut, OptionStr[i], static_cast<SHORT>(std::strlen(OptionStr[i])), {OptionPos[i].first, OptionPos[i].second}, &written);

	INPUT_RECORD irKb[12];
	DWORD wNumber;
	bool is_break = false;

	WriteConsoleOutputAttribute(hOut, attribute_fwhite, static_cast<SHORT>(std::strlen(OptionStr[OK])), {OptionPos[OK].first, OptionPos[OK].second}, &written);

	while (!is_break)
	{
		if (ReadConsoleInput(hIn, irKb, 12, &wNumber) == FALSE)
		{
			DWORD last_error = GetLastError();
			ErrorMsg("Show welcome page failed", last_error);
			Log::WriteLog(std::string("ConIO: Show welcome page failed: cannot read console input, errorcode: ") + std::to_string(last_error));
			return last_error;
		}

		for (DWORD i = 0; i < wNumber; i++)
		{
			if (irKb[i].EventType == KEY_EVENT && irKb[i].Event.KeyEvent.bKeyDown)
			{
				switch (irKb[i].Event.KeyEvent.wVirtualKeyCode)
				{
				case VK_RETURN:
					Shine(hOut, static_cast<SHORT>(std::strlen(OptionStr[OK])), {OptionPos[OK].first, OptionPos[OK].second});
					WriteConsoleOutputAttribute(hOut, attribute_bwhite, static_cast<SHORT>(std::strlen(OptionStr[OK])), {OptionPos[OK].first, OptionPos[OK].second}, &written);
					is_break = true;
					break;
				default:
					break;
				}
			}
		}
	}

	return ERROR_SUCCESS;
}

static DWORD ShowLevelPassMsgBox(HANDLE hOut, HANDLE hIn, SHORT x, SHORT y, SHORT dx, SHORT dy, bool &is_back, bool lv_up, int gain_exp)
{
	DWORD written;
	enum LPassPos
	{
		HEADER,
		LINE0,
		EXP,
		LV,
		OK,
		BACK
	};
	std::string xp_str = std::to_string(gain_exp);
	const char *OptionStr[BACK + 1] =
		{
			"Congratulations",
			"Level passed",
			"Gain XP",
			"LV up",
			"Next level",
			"Back to menu"};

	SHORT _x = x + dx / 5,
		  _dx = dx * 3 / 5,
		  _y = y + dy / 5,
		  _dy = dy * 3 / 5;

	std::pair<SHORT, SHORT> OptionPos[BACK + 1] =
		{
			{_x + _dx / 2 - static_cast<SHORT>(std::strlen(OptionStr[HEADER])) / 2, _y + _dy / 5},		// HEADER
			{_x + _dx / 2 - static_cast<SHORT>(std::strlen(OptionStr[LINE0])) / 2, _y + _dy * 2 / 5},   // LINE1
			{_x + _dx / 4 - static_cast<SHORT>(std::strlen(OptionStr[EXP])), _y + _dy * 3 / 5},			// EXP
			{_x + _dx * 3 / 4 - static_cast<SHORT>(std::strlen(OptionStr[LV])) / 2, _y + _dy * 3 / 5},  // LV
			{_x + _dx / 3 - static_cast<SHORT>(std::strlen(OptionStr[OK])) / 2, _y + _dy * 4 / 5},		// OK
			{_x + _dx * 2 / 3 - static_cast<SHORT>(std::strlen(OptionStr[BACK])) / 2, _y + _dy * 4 / 5} // BACK
		};

	for (auto row = _y; row <= _y + _dy; row++)
		for (auto col = _x; col <= _x + _dx; col++)
		{
			if (row == _y || row == _y + _dy || col == _x || col == _x + _dx)
				WriteConsoleOutputCharacter(hOut, "#", 1, {col, row}, &written);
			else if (col == _x + 1)
				WriteConsoleOutputCharacter(hOut, space_array, _dx - 1, {col, row}, &written);
		}

	for (int i = HEADER; i <= BACK; i = i + 1)
		WriteConsoleOutputCharacter(hOut, OptionStr[i], static_cast<SHORT>(std::strlen(OptionStr[i])), {OptionPos[i].first, OptionPos[i].second}, &written);
	WriteConsoleOutputCharacter(hOut, xp_str.c_str(), static_cast<SHORT>(xp_str.size()), {OptionPos[EXP].first + 1 + static_cast<SHORT>(std::strlen(OptionStr[EXP])), OptionPos[EXP].second}, &written);
	if (!lv_up)
		WriteConsoleOutputCharacter(hOut, space_array, static_cast<SHORT>(std::strlen(OptionStr[LV])), {OptionPos[LV].first, OptionPos[LV].second}, &written);

	LPassPos lppos = OK;
	INPUT_RECORD irKb[12];
	DWORD wNumber;
	bool is_break = false;

	WriteConsoleOutputAttribute(hOut, attribute_fwhite, static_cast<SHORT>(std::strlen(OptionStr[lppos])), {OptionPos[lppos].first, OptionPos[lppos].second}, &written);

	while (!is_break)
	{
		if (ReadConsoleInput(hIn, irKb, 12, &wNumber) == FALSE)
		{
			DWORD last_error = GetLastError();
			ErrorMsg("Show welcome page failed", last_error);
			Log::WriteLog(std::string("ConIO: Show welcome page failed: cannot read console input, errorcode: ") + std::to_string(last_error));
			return last_error;
		}

		for (DWORD i = 0; i < wNumber; i++)
		{
			if (irKb[i].EventType == KEY_EVENT && irKb[i].Event.KeyEvent.bKeyDown)
			{
				switch (irKb[i].Event.KeyEvent.wVirtualKeyCode)
				{
				case VK_RETURN:
					Shine(hOut, static_cast<SHORT>(std::strlen(OptionStr[lppos])), {OptionPos[lppos].first, OptionPos[lppos].second});
					WriteConsoleOutputAttribute(hOut, attribute_bwhite, static_cast<SHORT>(std::strlen(OptionStr[lppos])), {OptionPos[lppos].first, OptionPos[lppos].second}, &written);
					if (lppos == BACK)
						is_back = true;
					else
						is_back = false;
					is_break = true;
					break;
				case VK_LEFT:
				case VK_RIGHT:
					WriteConsoleOutputAttribute(hOut, attribute_bwhite, static_cast<SHORT>(std::strlen(OptionStr[lppos])), {OptionPos[lppos].first, OptionPos[lppos].second}, &written);
					lppos = (lppos == BACK) ? OK : BACK;
					WriteConsoleOutputAttribute(hOut, attribute_fwhite, static_cast<SHORT>(std::strlen(OptionStr[lppos])), {OptionPos[lppos].first, OptionPos[lppos].second}, &written);
					break;
				default:
					break;
				}
			}
		}
	}

	return ERROR_SUCCESS;
}

static DWORD ShowLevelFailMsgBox(HANDLE hOut, HANDLE hIn, SHORT x, SHORT y, SHORT dx, SHORT dy, bool &is_back)
{
	DWORD written;
	enum LFailPos
	{
		HEADER,
		LINE0,
		OK,
		BACK
	};

	const char *OptionStr[BACK + 1] =
		{
			"Opps",
			"You failed",
			"Try again",
			"Back to menu"};

	SHORT _x = x + dx / 4,
		  _dx = dx / 2,
		  _y = y + dy / 4,
		  _dy = dy / 2;

	std::pair<SHORT, SHORT> OptionPos[BACK + 1] =
		{
			{_x + _dx / 2 - static_cast<SHORT>(std::strlen(OptionStr[HEADER])) / 2, _y + _dy / 4},		// HEADER
			{_x + _dx / 2 - static_cast<SHORT>(std::strlen(OptionStr[LINE0])) / 2, _y + _dy * 2 / 4},   // LINE1
			{_x + _dx / 4 - static_cast<SHORT>(std::strlen(OptionStr[OK])) / 2, _y + _dy * 3 / 4},		// OK
			{_x + _dx * 3 / 4 - static_cast<SHORT>(std::strlen(OptionStr[BACK])) / 2, _y + _dy * 3 / 4} // BACK
		};

	for (auto row = _y; row <= _y + _dy; row++)
		for (auto col = _x; col <= _x + _dx; col++)
		{
			if (row == _y || row == _y + _dy || col == _x || col == _x + _dx)
				WriteConsoleOutputCharacter(hOut, "#", 1, {col, row}, &written);
			else if (col == _x + 1)
				WriteConsoleOutputCharacter(hOut, space_array, _dx - 1, {col, row}, &written);
		}

	for (int i = HEADER; i <= BACK; i = i + 1)
		WriteConsoleOutputCharacter(hOut, OptionStr[i], static_cast<SHORT>(std::strlen(OptionStr[i])), {OptionPos[i].first, OptionPos[i].second}, &written);

	LFailPos lfpos = OK;
	INPUT_RECORD irKb[12];
	DWORD wNumber;
	bool is_break = false;

	WriteConsoleOutputAttribute(hOut, attribute_fwhite, static_cast<SHORT>(std::strlen(OptionStr[lfpos])), {OptionPos[lfpos].first, OptionPos[lfpos].second}, &written);

	while (!is_break)
	{
		if (ReadConsoleInput(hIn, irKb, 12, &wNumber) == FALSE)
		{
			DWORD last_error = GetLastError();
			ErrorMsg("Show welcome page failed", last_error);
			Log::WriteLog(std::string("ConIO: Show welcome page failed: cannot read console input, errorcode: ") + std::to_string(last_error));
			return last_error;
		}

		for (DWORD i = 0; i < wNumber; i++)
		{
			if (irKb[i].EventType == KEY_EVENT && irKb[i].Event.KeyEvent.bKeyDown)
			{
				switch (irKb[i].Event.KeyEvent.wVirtualKeyCode)
				{
				case VK_RETURN:
					Shine(hOut, static_cast<SHORT>(std::strlen(OptionStr[lfpos])), {OptionPos[lfpos].first, OptionPos[lfpos].second});
					WriteConsoleOutputAttribute(hOut, attribute_bwhite, static_cast<SHORT>(std::strlen(OptionStr[lfpos])), {OptionPos[lfpos].first, OptionPos[lfpos].second}, &written);
					if (lfpos == BACK)
						is_back = true;
					else
						is_back = false;
					is_break = true;
					break;
				case VK_LEFT:
				case VK_RIGHT:
					WriteConsoleOutputAttribute(hOut, attribute_bwhite, static_cast<SHORT>(std::strlen(OptionStr[lfpos])), {OptionPos[lfpos].first, OptionPos[lfpos].second}, &written);
					lfpos = (lfpos == BACK) ? OK : BACK;
					WriteConsoleOutputAttribute(hOut, attribute_fwhite, static_cast<SHORT>(std::strlen(OptionStr[lfpos])), {OptionPos[lfpos].first, OptionPos[lfpos].second}, &written);
					break;
				default:
					break;
				}
			}
		}
	}

	return ERROR_SUCCESS;
}

static DWORD DrawGameFrame(HANDLE hOut, SMALL_RECT srW, SHORT x, SHORT y, SHORT dx, SHORT dy)
{
	DWORD written;
	for (SHORT row = 0; row < srW.Bottom; row++)
		for (SHORT col = 0; col < srW.Right; col++)
		{
			if (col < x || col > x + dx)
				WriteConsoleOutputCharacter(hOut, "%", 1, {col, row}, &written);
			else if (col == x || col == x + dx)
				WriteConsoleOutputCharacter(hOut, "#", 1, {col, row}, &written);
			else if (row == srW.Top || row == y + dy)
				WriteConsoleOutputCharacter(hOut, "$", 1, {col, row}, &written);
		}
	return ERROR_SUCCESS;
}

static DWORD CleanGameBoard(HANDLE hOut, SHORT x, SHORT y, SHORT dx, SHORT dy)
{
	DWORD written;
	for (SHORT row = y + 1; row < y + dy; row++)
		WriteConsoleOutputCharacter(hOut, space_array, dx - 1, {x + 1, row}, &written);
	return ERROR_SUCCESS;
}

static void ShowGameWord(HANDLE hOut, SHORT x, SHORT y, SHORT dx, SHORT dy, const std::string &word)
{
	DWORD written;
	int len = word.size();
	dx = dx - len;
	std::random_device rd;
	std::uniform_int_distribution<int> uid_x(x + 1, x + dx - 1);
	std::uniform_int_distribution<int> uid_y(y + 1, y + dy - 1);
	SHORT pos_x = uid_x(rd), pos_y = uid_y(rd);
	WriteConsoleOutputCharacter(hOut, word.c_str(), word.size(), {pos_x, pos_y}, &written);
}

static void RidKeyInput(HANDLE hIn)
{
	INPUT_RECORD irKb[100];
	DWORD wNumber;
	ReadConsoleInput(hIn, irKb, 100, &wNumber);
}

DWORD ConsoleIO::to_player_play_page(NextPage &next_page)
{
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	DWORD dwOldConsoleMode;

	if (hStdOut == INVALID_HANDLE_VALUE || hStdIn == INVALID_HANDLE_VALUE)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Show menu failed", last_error);
		Log::WriteLog(std::string("ConIO: Show menu failed: cannot get standard output handle, errorcode: ") + std::to_string(last_error));
		return last_error;
	}

	if (GetConsoleScreenBufferInfo(hStdOut, &csbi) == FALSE)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Show menu failed", last_error);
		Log::WriteLog(std::string("ConIO: Show menu failed: cannot get console screen buffer info, errorcode: ") + std::to_string(last_error));
		return last_error;
	}

	set_cursor_visible(hStdOut, FALSE);
	GetConsoleMode(hStdOut, &dwOldConsoleMode);
	SetConsoleMode(hStdOut, ENABLE_PROCESSED_INPUT);

	enum PPlayPos
	{
		TIME,
		LEVEL,
		INPUT
	};

	DWORD written;
	SMALL_RECT srW = csbi.srWindow;

	SHORT x = srW.Right / 6 + 1;
	SHORT y = srW.Top + 1;
	SHORT dx = (srW.Right * 5 / 6 - 1) - x;
	SHORT dy = (srW.Bottom - 1) - y;

	const char *OptionStr[INPUT + 1] =
		{
			"Time",
			"Level",
			"Word"};

	std::pair<SHORT, SHORT> OptionPos[INPUT + 1] =
		{
			{x + dx / 3, y + 2},
			{x + dx * 2 / 3, y + 2},
			{x + dx * 2 / 7, y + dy - 3},
		};

	DrawGameFrame(hStdOut, srW, x, y, dx, dy);

	for (int i = TIME; i <= INPUT; i++)
		WriteConsoleOutputCharacter(hStdOut, OptionStr[i], static_cast<SHORT>(std::strlen(OptionStr[i])), {OptionPos[i].first - static_cast<SHORT>(std::strlen(OptionStr[i])), OptionPos[i].second}, &written);
	for (SHORT col = x + 1; col < x + dx; col++)
		WriteConsoleOutputCharacter(hStdOut, "#", 1, {col, OptionPos[TIME].second + 1}, &written);
	ShowGameHelpMsgBox(hStdOut, hStdIn, x, y, dx, dy);

	CleanGameBoard(hStdOut, x, OptionPos[TIME].second + 1, dx, OptionPos[INPUT].second - OptionPos[TIME].second - 1);
	//
	// acc_sys->set_current_user();
	//
	bool is_back = false;
	INPUT_RECORD irKb[12];
	DWORD wNumber;
	std::string input_str;
	std::string cntdn_str;
	std::string name = acc_sys->get_current_user_str();
	Player &player = acc_sys->get_ref_player(name);
	int current_level = player.get_level_passed() + 1;
	// current_level = 50;

	while (!is_back)
	{
		double time_cost = 0.0;
		int total_difficulty = 0;
		int round = get_round(current_level);
		int countdown = get_round_time(current_level);
		std::string level_str = std::to_string(current_level);
		bool is_fail = false;

		while (round-- && !is_fail)
		{
			Word game_word = word_list->get_word(current_level);
			std::string game_word_str = game_word.word;
			cntdn_str = std::to_string(countdown);
			input_str.clear();
			CleanGameBoard(hStdOut, x, OptionPos[TIME].second + 1, dx, OptionPos[INPUT].second - OptionPos[TIME].second - 1);
			WriteConsoleOutputCharacter(hStdOut, space_array, static_cast<SHORT>(level_str.size()), {OptionPos[LEVEL].first + 1, OptionPos[LEVEL].second}, &written);
			WriteConsoleOutputCharacter(hStdOut, level_str.c_str(), static_cast<SHORT>(level_str.size()), {OptionPos[LEVEL].first + 1, OptionPos[LEVEL].second}, &written);
			WriteConsoleOutputCharacter(hStdOut, space_array, 6, {OptionPos[TIME].first + 1, OptionPos[TIME].second}, &written);
			WriteConsoleOutputCharacter(hStdOut, cntdn_str.c_str(), static_cast<SHORT>(cntdn_str.size()), {OptionPos[TIME].first + 1, OptionPos[TIME].second}, &written);

			ShowGameWord(hStdOut, x, OptionPos[TIME].second + 1, dx, OptionPos[INPUT].second - OptionPos[TIME].second - 1, game_word_str);

			for (int i = countdown; i > 0;)
			{
				i -= 43;
				if (i < 0)
					i = 0;
				cntdn_str = std::to_string(i);
				WriteConsoleOutputCharacter(hStdOut, space_array, 6, {OptionPos[TIME].first + 1, OptionPos[TIME].second}, &written);
				WriteConsoleOutputCharacter(hStdOut, cntdn_str.c_str(), static_cast<SHORT>(cntdn_str.size()), {OptionPos[TIME].first + 1, OptionPos[TIME].second}, &written);
				Sleep(43);
			}

			CleanGameBoard(hStdOut, x, OptionPos[TIME].second + 1, dx, OptionPos[INPUT].second - OptionPos[TIME].second - 1);
			SetConsoleCursorPosition(hStdOut, {OptionPos[INPUT].first + 1, OptionPos[INPUT].second});
			set_cursor_visible(hStdOut, TRUE);

			bool is_break = false;
			RidKeyInput(hStdIn);
			auto start = std::chrono::system_clock::now();
			while (!is_break)
			{
				if (ReadConsoleInput(hStdIn, irKb, 12, &wNumber) == FALSE)
				{
					DWORD last_error = GetLastError();
					ErrorMsg("Show welcome page failed", last_error);
					Log::WriteLog(std::string("ConIO: Show welcome page failed: cannot read console input, errorcode: ") + std::to_string(last_error));
					return last_error;
				}

				for (DWORD i = 0; i < wNumber; i++)
				{
					if (irKb[i].EventType == KEY_EVENT && irKb[i].Event.KeyEvent.bKeyDown)
					{
						if (std::isalpha(irKb[i].Event.KeyEvent.uChar.AsciiChar) && input_str.size() < 25)
						{
							char temp_input = irKb[i].Event.KeyEvent.uChar.AsciiChar;
							WriteConsoleOutputCharacter(hStdOut, &temp_input, 1, {OptionPos[INPUT].first + 1 + static_cast<SHORT>(input_str.size()), OptionPos[INPUT].second}, &written);
							input_str.push_back(temp_input);
							SetConsoleCursorPosition(hStdOut, {OptionPos[INPUT].first + 1 + static_cast<SHORT>(input_str.size()), OptionPos[INPUT].second});
						}
						switch (irKb[i].Event.KeyEvent.wVirtualKeyCode)
						{
						case VK_RETURN:
							if (input_str.size())
							{
								WriteConsoleOutputCharacter(hStdOut, space_array, static_cast<SHORT>(input_str.size()), {OptionPos[INPUT].first + 1, OptionPos[INPUT].second}, &written);
								set_cursor_visible(hStdOut, FALSE);
								is_break = true;
							}
							break;
						case VK_BACK:
							if (input_str.size())
							{
								WriteConsoleOutputCharacter(hStdOut, " ", 1, {OptionPos[INPUT].first + static_cast<SHORT>(input_str.size()), OptionPos[INPUT].second}, &written);
								input_str.pop_back();
								SetConsoleCursorPosition(hStdOut, {OptionPos[INPUT].first + 1 + static_cast<SHORT>(input_str.size()), OptionPos[INPUT].second});
							}
							break;
						default:
							break;
						}
					}
				}
			}
			auto end = std::chrono::system_clock::now();
			std::chrono::duration<double> elapsed_seconds = end - start;
			time_cost += elapsed_seconds.count() * 1000.0;
			if (input_str == game_word_str)
			{
				total_difficulty += game_word.difficulty;
			}
			else
			{
				is_fail = true;
			}
		}
		if (is_fail)
		{
			ShowLevelFailMsgBox(hStdOut, hStdIn, x, y, dx, dy, is_back);
			CleanGameBoard(hStdOut, x, OptionPos[TIME].second + 1, dx, OptionPos[INPUT].second - OptionPos[TIME].second - 1);
		}
		else
		{
			int lv_before = player.get_level();
			double gain_exp = get_gain_exp(current_level, time_cost, total_difficulty);
			player.raise_exp_(gain_exp);
			player.inc_level_passed();
			int lv_after = player.get_level();
			current_level++;
			ShowLevelPassMsgBox(hStdOut, hStdIn, x, y, dx, dy, is_back, !(lv_before == lv_after), gain_exp);
			CleanGameBoard(hStdOut, x, OptionPos[TIME].second + 1, dx, OptionPos[INPUT].second - OptionPos[TIME].second - 1);
		}
	}
	next_page = P_MENU;

	SetConsoleMode(hStdOut, dwOldConsoleMode);
	set_cursor_visible(hStdOut, TRUE);
	return ERROR_SUCCESS;
}
