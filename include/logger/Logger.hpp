#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include "config/config.hpp"

enum class LogLevel
{
	Debug = 0,
	Info  = 1,
	Warn  = 2,
	Error = 3
};

class Logger
{
private:
	static LogLevel _level;
	static bool     _enabled;

	static void emit(LogLevel level, const std::string& tag, const std::string& msg);

public:
	static void setLevel(LogLevel level);
	static void setEnabled(bool enabled);

	static LogLevel level();
	static bool     isEnabled();

	static void debug(const std::string& tag, const std::string& msg);
	static void info(const std::string& tag, const std::string& msg);
	static void warn(const std::string& tag, const std::string& msg);
	static void error(const std::string& tag, const std::string& msg);
};

#endif
