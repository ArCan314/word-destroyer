#pragma once 

// WARNING : OS(WIN32) SPECIFIC CODE

namespace ConsoleIO
{
#include "Windows.h"
	// return ErrorCode
	DWORD set_console_window_size();
	DWORD set_console_window_size(SHORT dwWidth, SHORT dwHeight);

	// return ErrorCode
	DWORD set_console_font_size();
	DWORD set_console_font_size(DWORD dwFontSize);

	DWORD clear_screen();

	DWORD to_fullscreen();
};