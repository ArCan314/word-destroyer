#include <iostream>
#include <string>
#define _WIN32_WINNT 0x0502
#include "Windows.h"

#include "console_io.h"
#include "log.h"

static void ErrorMsg(const std::string &msg, DWORD last_error)
{
	std::cerr << msg << " ErrorCode: " << last_error << std::endl;
}

DWORD ConsoleIO::set_console_window_size()
{
	return set_console_window_size(150, 40);
}

DWORD ConsoleIO::set_console_window_size(SHORT dwWidth, SHORT dwHeight)
{
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

	if (hStdOut == INVALID_HANDLE_VALUE)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Set console window size failed", last_error);
		Log::WriteLog("Set console window failed: cannot get handle of stdout");
		return last_error;
	}

	CONSOLE_SCREEN_BUFFER_INFO csbiStdOut;

	if (GetConsoleScreenBufferInfo(hStdOut, &csbiStdOut) == FALSE)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Set console window size failed", last_error);
		Log::WriteLog("Set console window failed: cannot get console screen buffer info");
		return last_error;
	}

	SMALL_RECT srWnd = csbiStdOut.srWindow;
	srWnd.Bottom = dwHeight;
	srWnd.Right = dwWidth;

	// set console buffer size to a larger one
	if (SetConsoleScreenBufferSize(hStdOut, {dwWidth + 1, (csbiStdOut.dwSize.Y > dwHeight) ? csbiStdOut.dwSize.Y : (dwHeight + 1)}) == FALSE)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Set console window size failed", last_error);
		Log::WriteLog("Set console window failed: cannot set console screen buffer size");
		return last_error;
	}

	// change console window size
	if (SetConsoleWindowInfo(hStdOut, TRUE, &srWnd) == FALSE)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Set console window size failed", last_error);
		Log::WriteLog("Set console window failed: cannot set console window info");
		return last_error;
	}

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
		Log::WriteLog("Clear screen failed: cannot get standard output handle");
		return last_error;
	}

	CONSOLE_SCREEN_BUFFER_INFO csbiInfo;

	// get console buffer size and attributes
	if (GetConsoleScreenBufferInfo(hStdOut, &csbiInfo) == FALSE)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Clear screen failed", last_error);
		Log::WriteLog("Clear screen failed: cannot get console screen buffer info");
		return last_error;
	}
	
	DWORD dwCharsWritten;
	DWORD dwBufLen = csbiInfo.dwSize.X * csbiInfo.dwSize.Y;
	std::cout.flush();	// clear the buffer of std::cout
	// fill console buffer with spaces
	if (FillConsoleOutputCharacter(hStdOut, TEXT(' '), dwBufLen, coordLeftTop, &dwCharsWritten) == FALSE)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Clear screen failed", last_error);
		Log::WriteLog("Clear screen failed: cannot fill console with spaces");
		return last_error;
	}

	if (FillConsoleOutputAttribute(hStdOut, csbiInfo.wAttributes, dwBufLen, coordLeftTop, &dwCharsWritten) == FALSE)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Clear screen failed", last_error);
		Log::WriteLog("Clear screen failed: cannot fill console attributes");
		return last_error;
	}

	if (SetConsoleCursorPosition(hStdOut, coordLeftTop) == FALSE)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Clear screen failed", last_error);
		Log::WriteLog("Clear screen failed: cannot set cursor position");
		return last_error;
	}

	return 0;
}

DWORD ConsoleIO::to_fullscreen()
{
	DWORD dwCurrentConsoleMode;
	COORD coordConsole;
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

	if (hStdOut == INVALID_HANDLE_VALUE)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Display in fullscreen failed", last_error);
		Log::WriteLog("Display in fullscreen failed: cannot get standard output handle");
		return last_error;
	}

	if (GetConsoleDisplayMode(&dwCurrentConsoleMode) == FALSE)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Display in fullscreen failed", last_error);
		Log::WriteLog("Display in fullscreen failed: cannot get current display mode");
		return last_error;
	}

	if (dwCurrentConsoleMode == CONSOLE_FULLSCREEN_MODE)
		return 0;
	
	if (SetConsoleDisplayMode(hStdOut, CONSOLE_FULLSCREEN_MODE, &coordConsole) == FALSE)
	{
		DWORD last_error = GetLastError();
		ErrorMsg("Display in fullscreen failed", last_error);
		Log::WriteLog("Display in fullscreen failed: cannot set to fullscreen mode");
		return last_error;
	}

	return 0;
}