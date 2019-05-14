#include <string>
#include <sstream>
#include <fstream>
#include <utility>
#include <map>
#include <chrono>
#include <ctime>
#include <cctype>

#include "log.h"

static bool InitLog();
static std::string GetCurrentTimestamp();
static std::string GetCurrentTime();

static bool is_inited = false;
static std::ofstream log_stream;
static std::string log_path;
static const std::string log_doc("../log/"); // dir of log files
static const std::string log_ext(".log");    // extension of log

// return a string consists of number in time_str
static std::string RemoveAllPunct(const std::string &time_str)
{
  std::string res;
  for (auto c : time_str)
    if (isdigit(c))
      res += c;
  return res;
}

std::string GetCurrentTimestamp()
{
  auto current_time = std::chrono::system_clock::to_time_t(
      std::chrono::system_clock::now());
  std::string timestamp = std::ctime(&current_time);
  std::stringstream ss(timestamp);

  std::string month_str, day_str, time_str, year_str;
  std::string res;

  ss >> month_str >> month_str;
  ss >> day_str;
  ss >> time_str;
  ss >> year_str;
  time_str = RemoveAllPunct(time_str);

  res = year_str + month_str + day_str + "_" + time_str;
  return res;
}

// for generating the file name of log
std::string GetCurrentTime()
{
  auto current_time = std::chrono::system_clock::to_time_t(
      std::chrono::system_clock::now());
  std::string timestamp = std::ctime(&current_time);
  std::stringstream ss(timestamp);

  std::string month_str, day_str, time_str, year_str;
  std::string res;

  ss >> month_str >> month_str;
  ss >> day_str;
  ss >> time_str;
  ss >> year_str;

  return time_str;
}

// automatically initialize
bool InitLog()
{
  log_path = log_doc + GetCurrentTimestamp() + log_ext;
  log_stream.open(log_path);
  return log_stream.is_open();
}

// write the mesg into the log with fixed pattern(TIME MESG)
bool Log::WriteLog(const std::string &log_str)
{
  if (!is_inited)
  {
    bool is_init_success = InitLog();
	if (is_init_success)
	{
		is_inited = true;
		Log::WriteLog("Log set up");
	}
  }
  if (log_stream)
  {
    log_stream << GetCurrentTime() << "  " << log_str << std::endl;
    return true;
  }
  else
  {
    return false;
  }
}

// for debuging
const std::string &Log::get_log_path()
{
  return log_path;
}

bool Log::CloseLog()
{
  log_stream.close();
  return !log_stream.is_open();
}