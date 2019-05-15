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

#include "console_io.h"
#include "log.h"
#include "account_sys.h"
#include "word_list.h"

static const int FOREGROUND_WHITE = (FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN);
static const int BACKGROUND_WHITE = (BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED);

static AccountSys *acc_sys;
static WordList *word_list;

static COORD coordOrigincsbiSize = {151, 9000};
static bool is_fullscreen = false;
static bool has_scrollbar = true;

static const int kAttrBufSize = 120;
static bool attr_inited = false;
static WORD attribute_fwhite[kAttrBufSize];
static WORD attribute_bwhite[kAttrBufSize];
static WORD attribute_fred[kAttrBufSize];

static char space_array[kAttrBufSize];

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

static void Shine(HANDLE h, SHORT len, COORD pos)
{
	static DWORD written;
	static constexpr int kShineTime = 7;
	for (int i = 0; i < kShineTime; i++)
	{
		Sleep(50);
		if (i % 2)
			WriteConsoleOutputAttribute(h, attribute_bwhite, len, pos, &written);
		else
			WriteConsoleOutputAttribute(h, attribute_fwhite, len, pos, &written);
	}
}

static DWORD press_alt_enter();
static DWORD set_cursor_visible(HANDLE h, BOOL visible)
{
	CONSOLE_CURSOR_INFO cci;
	GetConsoleCursorInfo(h, &cci);
	cci.bVisible = visible;
	SetConsoleCursorInfo(h, &cci);
	return ERROR_SUCCESS;
}

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

DWORD ConsoleIO::IO_Start()
{
	DWORD dwErrCode;
	NextPage np;
	bool is_exit = false;
	dwErrCode = to_welcome_page();
	np = P_MENU;
	if (dwErrCode != ERROR_SUCCESS)
		return dwErrCode;

	clear_screen();

	while (!is_exit)
	{
		switch (np)
		{
		case P_WELCOME:
			dwErrCode = to_welcome_page();
			if (dwErrCode)
				return dwErrCode;
			else
				np = P_MENU;
			clear_screen();
			break;
		case P_MENU:
			dwErrCode = to_menu_page(np);
			if (dwErrCode)
				return dwErrCode;
			clear_screen();
			break;
		case P_INFO:
			dwErrCode = to_my_info_page(np);
			if (dwErrCode)
				return dwErrCode;
			clear_screen();
			break;
		case P_EXIT:
			clear_screen();
			is_exit = true;
			break;
		default:
			break;
		}
	}
	return 0;
}

DWORD ConsoleIO::IOD_Start()
{
	NextPage np;
	to_my_info_page(np);
	return ERROR_SUCCESS;
}

DWORD ConsoleIO::set_console_window_size()
{
	return set_console_window_size(150, 40);
}

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

	if (dwHeight < csbiStdOut.dwMaximumWindowSize.Y - 1)
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

		// set console buffer size to a larger one

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

static DWORD press_alt_enter()
{
	INPUT inputTemp[2];

	KEYBDINPUT kbinputAlt = {0, MapVirtualKey(VK_MENU, MAPVK_VK_TO_VSC), KEYEVENTF_SCANCODE, 0, 0},
			   kbinputEnter = {0, MapVirtualKey(VK_RETURN, MAPVK_VK_TO_VSC), KEYEVENTF_SCANCODE, 0, 0};
	int cbSize = sizeof(INPUT);

	inputTemp[0].type = inputTemp[1].type = INPUT_KEYBOARD;
	inputTemp[0].ki = kbinputAlt, inputTemp[1].ki = kbinputEnter;

	if (SendInput(2, inputTemp, cbSize) == FALSE)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Change display mode failed", last_error);
		Log::WriteLog(std::string("ConIO: Change display mode failed: cannot send key event, errorcode: ") + std::to_string(last_error));
		return last_error;
	}

	inputTemp[0].ki.dwFlags = inputTemp[1].ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE;

	if (SendInput(2, inputTemp, cbSize) == FALSE)
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

	if (press_alt_enter() != ERROR_SUCCESS)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Change display mode failed", last_error);
		Log::WriteLog(std::string("ConIO: Change display mode failed: cannot send key event, errorcode: ") + std::to_string(last_error));
		return last_error;
	}

	is_fullscreen = true;
	Sleep(50);
	if (disable_scrollbar() != ERROR_SUCCESS)
	{
		DWORD last_error = GetLastError();
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

DWORD ConsoleIO::to_window()
{
	if (!is_fullscreen)
		return ERROR_SUCCESS;

	if (press_alt_enter() != ERROR_SUCCESS)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Change display mode failed", last_error);
		Log::WriteLog(std::string("ConIO: Change display mode failed: cannot send key event, errorcode: ") + std::to_string(last_error));
		return last_error;
	}

	Sleep(50);
	is_fullscreen = false;

	if (enable_scrollbar() != ERROR_SUCCESS)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Change display mode failed", last_error);
		Log::WriteLog(std::string("ConIO: Change display mode failed: cannot enable scroll bar, errorcode: ") + std::to_string(last_error));
		return last_error;
	}

	if (set_console_window_size() != ERROR_SUCCESS)
	{
		DWORD last_error = GetLastError();
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

	if (SetConsoleScreenBufferSize(hStdOut, csbiInfo.dwSize) == FALSE)
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

DWORD ConsoleIO::write_console_in_colour(HANDLE hOut, COORD pos, const wchar_t *wstr, WORD colour)
{
	DWORD written;
	WriteConsoleOutputAttribute(hOut, &colour, std::wcslen(wstr), pos, &written);
	WriteConsoleOutputCharacterW(hOut, wstr, std::wcslen(wstr), pos, &written);
	return ERROR_SUCCESS;
}

/*
DWORD ConsoleIO::to_welcome_page()
{
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	if (hStdOut == INVALID_HANDLE_VALUE)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Show menu failed", last_error);
		Log::WriteLog(std::string("ConIO: Show menu failed: cannot get standard output handle");
		return last_error;
	}

	if (GetConsoleScreenBufferInfo(hStdOut, &csbi) == FALSE)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Show menu failed", last_error);
		Log::WriteLog(std::string("ConIO: Show menu failed: cannot get console screen buffer info");
		return last_error;
	}
}
*/

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

	DWORD written;
	SMALL_RECT srW = csbi.srWindow;
	std::pair<SHORT /*X*/, SHORT /*Y*/> OptionPos[] =
		{
			{srW.Right / 3, 12},						 // ACC
			{srW.Right / 3, 15},						 // PAS
			{srW.Right / 3 + std::strlen("Log in"), 19}, // LOG
			{srW.Right * 4 / 7, 19},					 // SIGN
			{srW.Right / 2, 23}							 // INFO
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

			if (col == OptionPos[ACC].first - std::strlen("Account") && row == OptionPos[ACC].second)
				WriteConsoleOutputCharacter(hStdOut, "Account", std::strlen("Account"), {col, row}, &written);

			if (col == OptionPos[PAS].first - std::strlen("Password") && row == OptionPos[PAS].second)
				WriteConsoleOutputCharacter(hStdOut, "Password", std::strlen("Password"), {col, row}, &written);

			if (col == OptionPos[LOG].first && row == OptionPos[LOG].second)
				WriteConsoleOutputCharacter(hStdOut, "Log in", std::strlen("Log in"), {col, row}, &written);

			if (col == OptionPos[SIGN].first && row == OptionPos[SIGN].second)
				WriteConsoleOutputCharacter(hStdOut, "Sign up", std::strlen("Sign up"), {col, row}, &written);
		}

	SetConsoleCursorPosition(hStdOut, {OptionPos[ACC].first + 1, OptionPos[ACC].second});
	std::string account_name, password;
	INPUT_RECORD irKb[12];
	DWORD wNumber;
	WelPos mpos = ACC;
	DWORD dwOldConsoleMode;
	bool is_break = false;
	GetConsoleMode(hStdOut, &dwOldConsoleMode);
	SetConsoleMode(hStdOut, ENABLE_PROCESSED_INPUT);
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
			switch (irKb[i].EventType)
			{
			case KEY_EVENT:
				if (irKb[i].Event.KeyEvent.bKeyDown == TRUE)
				{
					KEY_EVENT_RECORD ker = irKb[i].Event.KeyEvent;
					CONSOLE_CURSOR_INFO cci;

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
						case LOG:
							break;
						case SIGN:
							break;
						default:
							break;
						}
					}
					else if (ker.wVirtualKeyCode == VK_UP)
					{
						switch (mpos)
						{
						case ACC:
							set_cursor_visible(hStdOut, FALSE);

							WriteConsoleOutputAttribute(hStdOut, attribute_fwhite, std::strlen("Sign Up"), {OptionPos[SIGN].first, OptionPos[SIGN].second}, &written);
							mpos = SIGN;
							break;
						case PAS:
							SetConsoleCursorPosition(hStdOut, {OptionPos[ACC].first + 1 + (SHORT)account_name.length(), OptionPos[ACC].second});
							mpos = ACC;
							break;
						case LOG:
							set_cursor_visible(hStdOut, TRUE);

							WriteConsoleOutputAttribute(hStdOut, attribute_bwhite, std::strlen("Log In"), {OptionPos[LOG].first, OptionPos[LOG].second}, &written);

							SetConsoleCursorPosition(hStdOut, {OptionPos[PAS].first + 1 + (SHORT)password.length(), OptionPos[PAS].second});
							mpos = PAS;
							break;
						case SIGN:
							WriteConsoleOutputAttribute(hStdOut, attribute_bwhite, std::strlen("Sign Up"), {OptionPos[SIGN].first, OptionPos[SIGN].second}, &written);

							WriteConsoleOutputAttribute(hStdOut, attribute_fwhite, std::strlen("Log In"), {OptionPos[LOG].first, OptionPos[LOG].second}, &written);
							mpos = LOG;
							break;
						default:
							break;
						}
					}
					else if (ker.wVirtualKeyCode == VK_DOWN)
					{
						switch (mpos)
						{
						case ACC:
							SetConsoleCursorPosition(hStdOut, {OptionPos[PAS].first + 1 + (SHORT)password.length(), OptionPos[PAS].second});
							mpos = PAS;
							break;
						case PAS:
							set_cursor_visible(hStdOut, FALSE);
							WriteConsoleOutputAttribute(hStdOut, attribute_fwhite, std::strlen("Log In"), {OptionPos[LOG].first, OptionPos[LOG].second}, &written);
							mpos = LOG;
							break;
						case LOG:
							WriteConsoleOutputAttribute(hStdOut, attribute_bwhite, std::strlen("Log In"), {OptionPos[LOG].first, OptionPos[LOG].second}, &written);
							WriteConsoleOutputAttribute(hStdOut, attribute_fwhite, std::strlen("Sign Up"), {OptionPos[SIGN].first, OptionPos[SIGN].second}, &written);
							mpos = SIGN;
							break;
						case SIGN:
							WriteConsoleOutputAttribute(hStdOut, attribute_bwhite, std::strlen("Sign Up"), {OptionPos[SIGN].first, OptionPos[SIGN].second}, &written);
							set_cursor_visible(hStdOut, TRUE);
							SetConsoleCursorPosition(hStdOut, {OptionPos[ACC].first + 1 + (SHORT)account_name.length(), OptionPos[ACC].second});
							mpos = ACC;
							break;
						default:
							break;
						}
					}
					else if (ker.wVirtualKeyCode == VK_LEFT || ker.wVirtualKeyCode == VK_RIGHT)
					{
						switch (mpos)
						{
						case ACC:
							break;
						case PAS:
							break;
						case LOG:
							WriteConsoleOutputAttribute(hStdOut, attribute_bwhite, std::strlen("Log In"), {OptionPos[LOG].first, OptionPos[LOG].second}, &written);
							WriteConsoleOutputAttribute(hStdOut, attribute_fwhite, std::strlen("Sign Up"), {OptionPos[SIGN].first, OptionPos[SIGN].second}, &written);
							mpos = SIGN;
							break;
						case SIGN:
							WriteConsoleOutputAttribute(hStdOut, attribute_bwhite, std::strlen("Sign Up"), {OptionPos[SIGN].first, OptionPos[SIGN].second}, &written);
							WriteConsoleOutputAttribute(hStdOut, attribute_fwhite, std::strlen("Log In"), {OptionPos[LOG].first, OptionPos[LOG].second}, &written);
							mpos = LOG;
							break;
						default:
							break;
						}
					}
					else if (ker.wVirtualKeyCode == VK_RETURN)
					{
						switch (mpos)
						{
						case ACC:
							break;
						case PAS:
							break;
						case LOG:
							Shine(hStdOut, std::strlen("Log In"), {OptionPos[LOG].first, OptionPos[LOG].second});

							if (account_name.length() < acc_sys->get_min_acc_len())
							{
								char msg[] = "Account must have 4 characters at least.";
								WriteConsoleOutputCharacter(hStdOut, space_array, srW.Right - srW.Left - 2, {srW.Left + 1, OptionPos[INFO].second}, &written);
								WriteConsoleOutputCharacter(hStdOut, msg, std::strlen(msg), {OptionPos[INFO].first - static_cast<SHORT>(std::strlen(msg)) / 2, OptionPos[INFO].second}, &written);
							}
							else if (password.length() < acc_sys->get_min_pswd_len())
							{
								char msg[] = "Password must have 9 characters at least.";
								WriteConsoleOutputCharacter(hStdOut, space_array, srW.Right - srW.Left - 2, {srW.Left + 1, OptionPos[INFO].second}, &written);
								WriteConsoleOutputCharacter(hStdOut, msg, std::strlen(msg), {OptionPos[INFO].first - static_cast<SHORT>(std::strlen(msg)) / 2, OptionPos[INFO].second}, &written);
							}
							else if (!acc_sys->LogIn(account_name, password))
							{
								char msg[] = "Failed to log in, check your account name and password.";
								WriteConsoleOutputCharacter(hStdOut, space_array, srW.Right - srW.Left - 2, {srW.Left + 1, OptionPos[INFO].second}, &written);
								WriteConsoleOutputCharacter(hStdOut, msg, std::strlen(msg), {OptionPos[INFO].first - static_cast<SHORT>(std::strlen(msg)) / 2, OptionPos[INFO].second}, &written);
							}
							else
							{
								is_break = true;
							}
							break;
						case SIGN:
							Shine(hStdOut, std::strlen("Sign Up"), {OptionPos[SIGN].first, OptionPos[SIGN].second});

							if (account_name.length() < acc_sys->get_min_acc_len())
							{
								char msg[] = "Account must have 4 characters at least.";
								WriteConsoleOutputCharacter(hStdOut, space_array, srW.Right - OptionPos[INFO].first + static_cast<SHORT>(std::strlen(msg)) / 2 - 1, {OptionPos[INFO].first - static_cast<SHORT>(std::strlen(msg)) / 2, OptionPos[INFO].second}, &written);
								WriteConsoleOutputCharacter(hStdOut, msg, std::strlen(msg), {OptionPos[INFO].first - static_cast<SHORT>(std::strlen(msg)) / 2, OptionPos[INFO].second}, &written);
							}
							else if (password.length() < acc_sys->get_min_pswd_len())
							{
								char msg[] = "Password must have 9 characters at least.";
								WriteConsoleOutputCharacter(hStdOut, space_array, srW.Right - OptionPos[INFO].first + static_cast<SHORT>(std::strlen(msg)) / 2 - 1, {OptionPos[INFO].first - static_cast<SHORT>(std::strlen(msg)) / 2, OptionPos[INFO].second}, &written);
								WriteConsoleOutputCharacter(hStdOut, msg, std::strlen(msg), {OptionPos[INFO].first - static_cast<SHORT>(std::strlen(msg)) / 2, OptionPos[INFO].second}, &written);
							}
							else if (!acc_sys->SignUp(account_name, password, UserType::USERTYPE_C))
							{
								char msg[] = "Sign up failed, account name has been used.";
								WriteConsoleOutputCharacter(hStdOut, space_array, srW.Right - OptionPos[INFO].first + static_cast<SHORT>(std::strlen(msg)) / 2 - 1, {OptionPos[INFO].first - static_cast<SHORT>(std::strlen(msg)) / 2, OptionPos[INFO].second}, &written);
								WriteConsoleOutputCharacter(hStdOut, msg, std::strlen(msg), {OptionPos[INFO].first - static_cast<SHORT>(std::strlen(msg)) / 2, OptionPos[INFO].second}, &written);
							}
							else
							{
								is_break = true;
							}
							break;
						default:
							break;
						}
					}
					else if (ker.wVirtualKeyCode == VK_BACK)
					{
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
						case LOG:
							break;
						case SIGN:
							break;
						default:
							break;
						}
					}
				}
				break;
			default:
				break;
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
	std::pair<SHORT /*X*/, SHORT /*Y*/> OptionPos[] =
		{
			{srW.Right / 2, srW.Bottom * 1 / 6}, // START_SINGLE
			{srW.Right / 2, srW.Bottom * 2 / 6}, // START_MULTIPLE
			{srW.Right / 2, srW.Bottom * 3 / 6}, // MY_INFO
			{srW.Right / 2, srW.Bottom * 4 / 6}, // USER_LIST
			{srW.Right / 2, srW.Bottom * 5 / 6}, // EXIT
		};

	const char *OptionStr[EXIT + 1] =
		{
			"Play Offline",
			"Play Online",
			"My Infomation",
			"Other Players",
			"Exit"};

	for (SHORT row = 0; row < srW.Bottom; row++)
		for (SHORT col = 0; col < srW.Right; col++)
		{
			if (col < srW.Right / 3 || col > srW.Right * 2 / 3)
				WriteConsoleOutputCharacter(hStdOut, "%", 1, {col, row}, &written);
			else if (col == srW.Right / 3 || col == srW.Right * 2 / 3)
				WriteConsoleOutputCharacter(hStdOut, "#", 1, {col, row}, &written);
			else if (row == 0 || row == srW.Bottom - 1)
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

			if (row == OptionPos[START_SINGLE].second && col == OptionPos[START_SINGLE].first)
				WriteConsoleOutputCharacter(hStdOut, OptionStr[START_SINGLE], std::strlen(OptionStr[START_SINGLE]), {col - static_cast<SHORT>(std::strlen(OptionStr[START_SINGLE])) / 2, row}, &written);

			else if (row == OptionPos[START_MULT].second && col == OptionPos[START_MULT].first)
				WriteConsoleOutputCharacter(hStdOut, OptionStr[START_MULT], std::strlen(OptionStr[START_MULT]), {col - static_cast<SHORT>(std::strlen(OptionStr[START_MULT])) / 2, row}, &written);

			else if (row == OptionPos[MY_INFO].second && col == OptionPos[MY_INFO].first)
				WriteConsoleOutputCharacter(hStdOut, OptionStr[MY_INFO], std::strlen(OptionStr[MY_INFO]), {col - static_cast<SHORT>(std::strlen(OptionStr[MY_INFO])) / 2, row}, &written);

			else if (row == OptionPos[USER_LIST].second && col == OptionPos[USER_LIST].first)
				WriteConsoleOutputCharacter(hStdOut, OptionStr[USER_LIST], std::strlen(OptionStr[USER_LIST]), {col - static_cast<SHORT>(std::strlen(OptionStr[USER_LIST])) / 2, row}, &written);

			else if (row == OptionPos[EXIT].second && col == OptionPos[EXIT].first)
				WriteConsoleOutputCharacter(hStdOut, OptionStr[EXIT], std::strlen(OptionStr[EXIT]), {col - static_cast<SHORT>(std::strlen(OptionStr[EXIT])) / 2, row}, &written);
		}

	bool is_break = false;
	MenuPos mpos = START_SINGLE;

	std::pair<SHORT /*L-Side*/, SHORT /*R-Side*/> side = {srW.Right / 3 + 1, srW.Right * 2 / 3 - 1};
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
					switch (mpos)
					{
					case START_SINGLE:
						WriteConsoleOutputAttribute(hStdOut, attribute_bwhite, side.second - side.first, {side.first, OptionPos[START_SINGLE].second}, &written);
						WriteConsoleOutputAttribute(hStdOut, attribute_fwhite, side.second - side.first, {side.first, OptionPos[EXIT].second}, &written);
						mpos = EXIT;
						break;
					case START_MULT:
						WriteConsoleOutputAttribute(hStdOut, attribute_bwhite, side.second - side.first, {side.first, OptionPos[START_MULT].second}, &written);
						WriteConsoleOutputAttribute(hStdOut, attribute_fwhite, side.second - side.first, {side.first, OptionPos[START_SINGLE].second}, &written);
						mpos = START_SINGLE;
						break;
					case MY_INFO:
						WriteConsoleOutputAttribute(hStdOut, attribute_bwhite, side.second - side.first, {side.first, OptionPos[MY_INFO].second}, &written);
						WriteConsoleOutputAttribute(hStdOut, attribute_fwhite, side.second - side.first, {side.first, OptionPos[START_MULT].second}, &written);
						mpos = START_MULT;
						break;
					case USER_LIST:
						WriteConsoleOutputAttribute(hStdOut, attribute_bwhite, side.second - side.first, {side.first, OptionPos[USER_LIST].second}, &written);
						WriteConsoleOutputAttribute(hStdOut, attribute_fwhite, side.second - side.first, {side.first, OptionPos[MY_INFO].second}, &written);
						mpos = MY_INFO;
						break;
					case EXIT:
						WriteConsoleOutputAttribute(hStdOut, attribute_bwhite, side.second - side.first, {side.first, OptionPos[EXIT].second}, &written);
						WriteConsoleOutputAttribute(hStdOut, attribute_fwhite, side.second - side.first, {side.first, OptionPos[USER_LIST].second}, &written);
						mpos = USER_LIST;
						break;
					default:
						break;
					}
					break;
				case VK_DOWN:
					switch (mpos)
					{
					case START_SINGLE:
						WriteConsoleOutputAttribute(hStdOut, attribute_bwhite, side.second - side.first, {side.first, OptionPos[START_SINGLE].second}, &written);
						WriteConsoleOutputAttribute(hStdOut, attribute_fwhite, side.second - side.first, {side.first, OptionPos[START_MULT].second}, &written);
						mpos = START_MULT;
						break;
					case START_MULT:
						WriteConsoleOutputAttribute(hStdOut, attribute_bwhite, side.second - side.first, {side.first, OptionPos[START_MULT].second}, &written);
						WriteConsoleOutputAttribute(hStdOut, attribute_fwhite, side.second - side.first, {side.first, OptionPos[MY_INFO].second}, &written);
						mpos = MY_INFO;
						break;
					case MY_INFO:
						WriteConsoleOutputAttribute(hStdOut, attribute_bwhite, side.second - side.first, {side.first, OptionPos[MY_INFO].second}, &written);
						WriteConsoleOutputAttribute(hStdOut, attribute_fwhite, side.second - side.first, {side.first, OptionPos[USER_LIST].second}, &written);
						mpos = USER_LIST;
						break;
					case USER_LIST:
						WriteConsoleOutputAttribute(hStdOut, attribute_bwhite, side.second - side.first, {side.first, OptionPos[USER_LIST].second}, &written);
						WriteConsoleOutputAttribute(hStdOut, attribute_fwhite, side.second - side.first, {side.first, OptionPos[EXIT].second}, &written);
						mpos = EXIT;
						break;
					case EXIT:
						WriteConsoleOutputAttribute(hStdOut, attribute_bwhite, side.second - side.first, {side.first, OptionPos[EXIT].second}, &written);
						WriteConsoleOutputAttribute(hStdOut, attribute_fwhite, side.second - side.first, {side.first, OptionPos[START_SINGLE].second}, &written);
						mpos = START_SINGLE;
						break;
					default:
						break;
					}
					break;
				case VK_RETURN:
					switch (mpos)
					{
					case START_SINGLE:
						next_page = P_SP;
						is_break = true;
						Shine(hStdOut, side.second - side.first, {side.first, OptionPos[START_SINGLE].second});

						break;
					case START_MULT:
						next_page = P_MP;
						is_break = true;
						Shine(hStdOut, side.second - side.first, {side.first, OptionPos[START_MULT].second});

						break;
					case MY_INFO:
						next_page = P_INFO;
						is_break = true;
						Shine(hStdOut, side.second - side.first, {side.first, OptionPos[MY_INFO].second});

						break;
					case USER_LIST:
						next_page = P_ULIST;
						is_break = true;
						Shine(hStdOut, side.second - side.first, {side.first, OptionPos[USER_LIST].second});

						break;
					case EXIT:
						next_page = P_EXIT;
						is_break = true;
						Shine(hStdOut, side.second - side.first, {side.first, OptionPos[EXIT].second});

						break;
					default:
						break;
					}
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

	std::pair<SHORT /*L-Side*/, SHORT /*R-Side*/> sidex = {srW.Right / 5 + 1, srW.Right * 4 / 5 - 1};
	std::pair<SHORT /*L-Side*/, SHORT /*R-Side*/> sidey = {srW.Top + 1, srW.Bottom - 1};

	SHORT sidex_diff = sidex.second - sidex.first;
	SHORT sidey_diff = sidey.second - sidey.first;

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
			{sidex.first + sidex_diff * 1 / 4, sidey.first + sidey_diff * 1 / 5}, // NAME
			//
			{sidex.first + sidex_diff * 1 / 4, sidey.first + sidey_diff * 2 / 5}, // ROLE
			{sidex.first + sidex_diff * 3 / 4, sidey.first + sidey_diff * 2 / 5}, // LEVEL
			//
			{sidex.first + sidex_diff * 1 / 4, sidey.first + sidey_diff * 3 / 5}, // CONTIBUTED
			/**/
			{sidex.first + sidex_diff * 1 / 4, sidey.first + sidey_diff * 3 / 5}, // EXP
			{sidex.first + sidex_diff * 3 / 4, sidey.first + sidey_diff * 3 / 5}, // L_PASSED
			//
			{0 / 2, 0},																															// CHANGE_PSWD
			{sidex.first + sidex_diff * 1 / 3 - static_cast<SHORT>(std::strlen(OptionStr[CHANGE_ROLE])) / 2, sidey.first + sidey_diff * 4 / 5}, // CHANGE_ROLE
			{sidex.first + sidex_diff * 2 / 3 - static_cast<SHORT>(std::strlen(OptionStr[BACK])) / 2, sidey.first + sidey_diff * 4 / 5},		// BACK
			{sidex.first + sidex_diff * 1 / 2, sidey.first + sidey_diff * 9 / 10}																// INFO
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
			if (col < srW.Right / 5 || col > srW.Right * 4 / 5)
				WriteConsoleOutputCharacter(hStdOut, "%", 1, {col, row}, &written);
			else if (col == srW.Right / 5 || col == srW.Right * 4 / 5)
				WriteConsoleOutputCharacter(hStdOut, "#", 1, {col, row}, &written);
			else if (row == 0 || row == srW.Bottom - 1)
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
				case VK_LEFT: case VK_RIGHT:
					switch (mpos)
					{
					case CHANGE_ROLE:
						if (ack_change_role)
						{
							WriteConsoleOutputCharacter(hStdOut, space_array, sidex_diff, {sidex.first, OptionPos[INFO].second}, &written);
							WriteConsoleOutputAttribute(hStdOut, attribute_bwhite, sidex_diff, {sidex.first, OptionPos[INFO].second}, &written);
							ack_change_role = false;
						}
						WriteConsoleOutputAttribute(hStdOut, attribute_fwhite, std::strlen(OptionStr[BACK]), {OptionPos[BACK].first, OptionPos[BACK].second}, &written);
						WriteConsoleOutputAttribute(hStdOut, attribute_bwhite, std::strlen(OptionStr[CHANGE_ROLE]), {OptionPos[CHANGE_ROLE].first, OptionPos[CHANGE_ROLE].second}, &written);
						mpos = BACK;
						break;
					case BACK:
						if (ack_change_role)
						{
							WriteConsoleOutputCharacter(hStdOut, space_array, sidex_diff, {sidex.first, OptionPos[INFO].second}, &written);
							WriteConsoleOutputAttribute(hStdOut, attribute_bwhite, sidex_diff, {sidex.first, OptionPos[INFO].second}, &written);
							ack_change_role = false;
						}
						WriteConsoleOutputAttribute(hStdOut, attribute_fwhite, std::strlen(OptionStr[CHANGE_ROLE]), {OptionPos[CHANGE_ROLE].first, OptionPos[CHANGE_ROLE].second}, &written);
						WriteConsoleOutputAttribute(hStdOut, attribute_bwhite, std::strlen(OptionStr[BACK]), {OptionPos[BACK].first, OptionPos[BACK].second}, &written);
						mpos = CHANGE_ROLE;
						break;
					default:
						break;
					}
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
							WriteConsoleOutputCharacter(hStdOut, space_array, sidex_diff, {sidex.first, OptionPos[INFO].second}, &written);
							WriteConsoleOutputAttribute(hStdOut, attribute_bwhite, sidex_diff, {sidex.first, OptionPos[INFO].second}, &written);
							Shine(hStdOut, std::strlen(OptionStr[CHANGE_ROLE]), {OptionPos[CHANGE_ROLE].first, OptionPos[CHANGE_ROLE].second});
						}
						else
						{
							std::string warning("WARNING: ALL DATA WOULD BE REESET, PRESS AGAIN TO CONTINUE.");
							WriteConsoleOutputCharacter(hStdOut, warning.c_str(), warning.size(), {OptionPos[INFO].first - static_cast<SHORT>(warning.size()) / 2 , OptionPos[INFO].second}, &written);
							WriteConsoleOutputAttribute(hStdOut, attribute_fred, warning.size(), {OptionPos[INFO].first - static_cast<SHORT>(warning.size()) / 2 , OptionPos[INFO].second}, &written);
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
