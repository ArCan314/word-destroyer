#pragma once

#include <string>

namespace Log
{
// automatically initialize
bool WriteLog(const std::string &log_str);

// for debug
const std::string &get_log_path();

// remember to close log file stream
bool CloseLog();
};