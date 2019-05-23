#include <string>
#include <sstream>
#include <fstream>
#include <utility>
#include <map>
#include <chrono>
#include <ctime>
#include <cctype>
#include <thread>
#include <mutex>
#include <sstream>

#include "log.h"

static bool InitLog();
static std::string GetCurrentTimestamp();
static std::string GetCurrentTime();

static bool is_inited = false;
static std::ofstream log_stream;
static std::string log_path;
static const std::string log_doc("../log/"); // dir of log files
static const std::string log_ext(".log");    // extension of log
static std::mutex mutex_;

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
  std::lock_guard<std::mutex> lock(mutex_);
  if (!is_inited)
  {
    bool is_init_success = InitLog();
	if (is_init_success)
	{
		is_inited = true;
	}
  }
  if (log_stream)
  {
	  log_stream << GetCurrentTime() << " t_id: " << std::this_thread::get_id() << " " << log_str << std::endl;
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
	Log::WriteLog(std::string("Log") + " : close.");
  log_stream.close();
  return !log_stream.is_open();
}

std::string Log::get_thread_str()
{
	std::stringstream ss;
	std::thread::id temp = std::this_thread::get_id();
	ss << temp;
	return ss.str();
}
