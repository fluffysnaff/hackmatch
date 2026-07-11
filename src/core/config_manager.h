#pragma once

#include "settings.h"

#include <filesystem>
#include <string>
#include <vector>

namespace hackmatch {
struct ConfigValidation {
    bool ok = false;
    std::string message;
    AppSettings value{};
};

class ConfigManager {
public:
    static constexpr int schema_version = 1;
    static constexpr size_t maximum_document_size = 256 * 1024;

    explicit ConfigManager(std::filesystem::path profile_directory = {});

    bool initialize(AppSettings& live, std::string& error);
    std::vector<std::string> profiles() const;
    const std::string& active_profile() const;
    bool dirty(const AppSettings& live) const;

    bool create(const std::string& name, const AppSettings& live, std::string& error);
    bool save(const AppSettings& live, std::string& error);
    bool load(const std::string& name, AppSettings& live, std::string& error);
    bool rename_active(const std::string& name, std::string& error);
    bool remove(const std::string& name, AppSettings& live, std::string& error);
    void reset(AppSettings& live) const;

    std::string export_string(const AppSettings& live) const;
    ConfigValidation inspect_import(const std::string& encoded) const;
    bool copy_to_clipboard(const AppSettings& live, std::string& error) const;
    ConfigValidation inspect_clipboard(std::string& error) const;

    static std::string serialize(const AppSettings& value);
    static ConfigValidation deserialize(const std::string& document);
    static bool valid_profile_name(const std::string& name, std::string& error);

private:
    std::filesystem::path path_for(const std::string& name) const;
    bool write_atomic(const std::filesystem::path& path, const std::string& contents, std::string& error) const;
    bool remember_active(std::string& error) const;
    bool read_profile(const std::string& name, AppSettings& value, std::string& error) const;

    std::filesystem::path directory_;
    std::string active_profile_ = "Default";
    AppSettings saved_{};
};

ConfigManager& config_manager();
}
