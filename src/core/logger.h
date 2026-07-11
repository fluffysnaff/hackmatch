#pragma once

#include <string_view>

namespace hackmatch {
enum class LogLevel {
    Info,
    Success,
    Warning,
    Error,
};

namespace logger {
bool initialize_console();
void shutdown_console();
bool active();
void write(LogLevel level, std::string_view message);
void info(std::string_view message);
void success(std::string_view message);
void warning(std::string_view message);
void error(std::string_view message);
}
}
