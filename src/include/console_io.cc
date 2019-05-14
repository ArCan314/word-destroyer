#include <iostream>
#include <string>
#include <cwchar>
#include <cstring>
#include <cctype>
#include <utility>
#include <vector>
#undef UNICODE
#define _WIN32_WINNT 0x0502
#include "Windows.h"

#include "console_io.h"
#include "log.h"
#include "account_sys.h"

static const int FOREGROUND_WHITE = (FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN);
static const int BACKGROUND_WHITE = (BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED);

static AccountSys *acc_sys;

static COORD coordOrigincsbiSize = {151, 9000};
static bool is_fullscreen = false;
static bool has_scrollbar = true;

static DWORD press_alt_enter();
static DWORD set_cursor_visible(HANDLE h, BOOL visible)
{
	CONSOLE_CURSOR_INFO cci;
	GetConsoleCursorInfo(h, &cci);
	cci.bVisible = visible;
	SetConsoleCursorInfo(h, &cci);
	return ERROR_SUCCESS;
}

enum MenuPos
{
	ACC = 0,
	PAS,
	LOG,
	SIGN,
	INFO
};

static void ErrorMsg(const std::string &msg, DWORD last_error)
{
	std::cerr << msg << " ErrorCode: " << last_error << std::endl;
}

void ConsoleIO::set_account_sys_ptr(AccountSys *as)
{
	acc_sys = as;
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
DWORD ConsoleIO::show_menu()
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

DWORD ConsoleIO::show_menu()
{
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;

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
				WriteConsoleOutputCharacter(hStdOut, "*", 1, {col, row}, &written);

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
	MenuPos mpos = ACC;
	DWORD dwOldConsoleMode;
	bool is_break = false;
	GetConsoleMode(hStdOut, &dwOldConsoleMode);
	SetConsoleMode(hStdOut, ENABLE_PROCESSED_INPUT);
	while (!is_break)
	{
		if (ReadConsoleInput(hStdIn, irKb, 12, &wNumber) == FALSE)
		{
			DWORD last_error = GetLastError();
			ErrorMsg("Show menu failed", last_error);
			Log::WriteLog(std::string("ConIO: Show menu failed: cannot read console input, errorcode: ") + std::to_string(last_error));
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

					static const int attr_buf_size = 120;
					static bool attr_inited = false;
					static WORD attribute_fwhite[attr_buf_size];
					static WORD attribute_bwhite[attr_buf_size];
					static char space_array[attr_buf_size];
					if (!attr_inited)
					{
						attr_inited = true;
						for (int i = 0; i < attr_buf_size; i++)
						{
							attribute_fwhite[i] = FOREGROUND_WHITE;
							attribute_bwhite[i] = BACKGROUND_WHITE;
							space_array[i] = ' ';
						}
					}
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
							else if (!acc_sys->LogIn(account_name, password))
							{
								char msg[] = "Failed to log in, check your account name and password.";
								WriteConsoleOutputCharacter(hStdOut, space_array, srW.Right - OptionPos[INFO].first + static_cast<SHORT>(std::strlen(msg)) / 2 - 1, {OptionPos[INFO].first - static_cast<SHORT>(std::strlen(msg)) / 2, OptionPos[INFO].second}, &written);
								WriteConsoleOutputCharacter(hStdOut, msg, std::strlen(msg), {OptionPos[INFO].first - static_cast<SHORT>(std::strlen(msg)) / 2, OptionPos[INFO].second}, &written);
							}
							else
							{
								is_break = true;
							}
							break;
						case SIGN:
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