#include "logger.h"

#include <windows.h>

#include <array>
#include <cstdio>
#include <filesystem>
#include <mutex>
#include <string>

namespace hackmatch::logger {
namespace {
std::mutex output_mutex;
bool console_owned = false;
std::FILE* log_file = nullptr;

constexpr WORD timestamp_color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
constexpr WORD project_color = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
constexpr WORD text_color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;

struct LevelStyle {
    const char* name;
    WORD color;
};

LevelStyle style(LogLevel level)
{
    switch (level) {
    case LogLevel::Info:
        return {"Info", FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY};
    case LogLevel::Success:
        return {"Success", FOREGROUND_GREEN | FOREGROUND_INTENSITY};
    case LogLevel::Warning:
        return {"Warning", FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY};
    case LogLevel::Error:
        return {"Error", FOREGROUND_RED | FOREGROUND_INTENSITY};
    }
    return {"Info", text_color};
}

HANDLE output_handle()
{
    const HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    return handle && handle != INVALID_HANDLE_VALUE && GetConsoleMode(handle, &mode) ? handle : nullptr;
}

void output(HANDLE handle, WORD color, std::string_view value)
{
    if (value.empty()) {
        return;
    }
    SetConsoleTextAttribute(handle, color);
    DWORD written = 0;
    WriteConsoleA(handle, value.data(), static_cast<DWORD>(value.size()), &written, nullptr);
}

void initialize_file()
{
    wchar_t app_data[MAX_PATH]{};
    const DWORD length = GetEnvironmentVariableW(L"APPDATA", app_data, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) return;
    const std::filesystem::path directory = std::filesystem::path(app_data) / L"Hackmatch";
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    if (!error) _wfopen_s(&log_file, (directory / L"hackmatch.log").c_str(), L"w");
}
}

bool initialize_console()
{
    std::scoped_lock lock(output_mutex);
    if (!log_file) initialize_file();
    if (output_handle()) {
        return true;
    }
    if (!AllocConsole()) {
        return false;
    }
    console_owned = true;
    SetConsoleTitleW(L"Hackmatch");
    return output_handle() != nullptr;
}

void shutdown_console()
{
    std::scoped_lock lock(output_mutex);
    if (log_file) {
        std::fclose(log_file);
        log_file = nullptr;
    }
    if (console_owned) {
        FreeConsole();
        console_owned = false;
    }
}

bool active()
{
    return output_handle() != nullptr;
}

void write(LogLevel level, std::string_view message)
{
    std::scoped_lock lock(output_mutex);
    const HANDLE handle = output_handle();

    SYSTEMTIME time{};
    GetLocalTime(&time);
    std::array<char, 16> timestamp{};
    const int timestamp_length = std::snprintf(
        timestamp.data(), timestamp.size(), "[%02u:%02u:%02u] ",
        static_cast<unsigned>(time.wHour), static_cast<unsigned>(time.wMinute), static_cast<unsigned>(time.wSecond));
    const LevelStyle level_style = style(level);
    const std::string severity = std::string(level_style.name) + ": ";

    if (log_file) {
        if (timestamp_length > 0) std::fwrite(timestamp.data(), 1, static_cast<std::size_t>(timestamp_length), log_file);
        std::fprintf(log_file, "[hackmatch] %s%.*s\n", severity.c_str(), static_cast<int>(message.size()), message.data());
        std::fflush(log_file);
    }

    if (!handle) return;

    if (timestamp_length > 0) {
        output(handle, timestamp_color, std::string_view(timestamp.data(), static_cast<std::size_t>(timestamp_length)));
    }
    output(handle, project_color, "[hackmatch] ");
    output(handle, level_style.color, severity);
    output(handle, text_color, message);
    output(handle, text_color, "\r\n");
}

void info(std::string_view message)
{
    write(LogLevel::Info, message);
}

void success(std::string_view message)
{
    write(LogLevel::Success, message);
}

void warning(std::string_view message)
{
    write(LogLevel::Warning, message);
}

void error(std::string_view message)
{
    write(LogLevel::Error, message);
}
}
