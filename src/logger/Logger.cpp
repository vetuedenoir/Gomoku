#include "logger/Logger.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <sstream>

static constexpr const char* RESET  = "\033[0m";
static constexpr const char* GRAY   = "\033[90m";
static constexpr const char* CYAN   = "\033[1;36m";
static constexpr const char* YELLOW = "\033[1;33m";
static constexpr const char* RED    = "\033[1;31m";
static constexpr const char* GREEN  = "\033[1;32m";

LogLevel Logger::_level   = LogLevel::Debug;
bool     Logger::_enabled = true;

void Logger::setLevel(LogLevel l)
{
	_level = l;
}
void Logger::setEnabled(bool e)
{
	_enabled = e;
}

LogLevel Logger::level()
{
	return _level;
}
bool Logger::isEnabled()
{
	return _enabled;
}

void Logger::debug(const std::string& tag, const std::string& msg)
{
	emit(LogLevel::Debug, tag, msg);
}
void Logger::info(const std::string& tag, const std::string& msg)
{
	emit(LogLevel::Info, tag, msg);
}
void Logger::warn(const std::string& tag, const std::string& msg)
{
	emit(LogLevel::Warn, tag, msg);
}
void Logger::error(const std::string& tag, const std::string& msg)
{
	emit(LogLevel::Error, tag, msg);
}

void Logger::emit(LogLevel lv, const std::string& tag, const std::string& msg)
{
	if (!_enabled || lv < _level)
		return;

	static const auto t0 = std::chrono::steady_clock::now();
	const auto        ms =
		std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();

	const char* color;
	const char* badge;

	switch (lv)
	{
		case LogLevel::Debug:
			color = GRAY;
			badge = "DEBUG";
			break;
		case LogLevel::Info:
			color = CYAN;
			badge = "INFO ";
			break;
		case LogLevel::Warn:
			color = YELLOW;
			badge = "WARN ";
			break;
		case LogLevel::Error:
			color = RED;
			badge = "ERROR";
			break;
		default:
			color = RESET;
			badge = "DEFAULT";
			break;
	}

	std::ostream& out = (lv >= LogLevel::Warn) ? std::cerr : std::cout;

	out << color << "[" << badge << "]" << RESET << GRAY << " " << std::setw(7) << std::right << ms << "ms " << RESET
		<< GREEN << "[" << std::left << std::setw(10) << tag << "]" << RESET << "  " << msg << "\n";
}
