
// Placeholder for the configuration parsing module implementation as per T005.
// Implementation will be provided by the user.
#include "config/Config.hpp"
#include "common/Log.hpp"

#include <fstream>
#include <iostream>


static void trim(std::string& str) {
    if(str.empty()) return;

    // 去除字符串 str 开头的空白字符（包括空格、制表符、换行符和回车符）
    str.erase(0, str.find_first_not_of(" \t\n\r"));
    // 去除字符串 str 结尾的空白字符（包括空格、制表符、换行符和回车符）
    str.erase(str.find_last_not_of(" \t\n\r") + 1);
}


Configure::Configure(const std::string& configFile)
    : isLoaded_(false), port_(0), timeout_(0) {
    // 尝试加载配置文件，若文件缺失或解析失败则抛出异常（测试依赖）
    if (!load(configFile)) {
        log_error("Failed to load or parse configure file: %s", configFile.c_str());
        throw std::runtime_error("Failed to load configuration");
    }
}


bool Configure::load(const std::string& configFile) {
    std::ifstream ifs(configFile.c_str());
    if (!ifs.is_open()) {
        log_error("Configuration file not found: %s", configFile.c_str());
        return false;
    }

    std::string line;

    while (std::getline(ifs, line)) {
        trim(line);
        if (line.empty() || line[0] == '#') continue; // 跳过空行和注释行

        std::string::size_type pos = line.find('=');
        if (pos == std::string::npos) continue; // 跳过不包含

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        trim(key);
        trim(value);

        if (!key.empty()) {
            configMap_[key] = value;
        }
    }

    // --- T027: Parse specific values ---
    // Port (required)
    std::string port_str = get("port");
    if (port_str.empty()) {
        log_error("'port' is a required configuration.");
        return false;
    }
    try {
        port_ = std::stoi(port_str);
    } catch (const std::exception& e) {
        log_error("Invalid port number '%s'", port_str.c_str());
        return false;
    }
    // Validate port range
    if (port_ <= 0 || port_ > 65535) {
        log_error("Port out of valid range (1-65535): %d", port_);
        return false;
    }

    // Log Level (with default)
    logLevel_ = get("log_level", "INFO");

    // Timeout (with default)
    std::string timeout_str = get("timeout_seconds", "60");
    try {
        timeout_ = std::stoi(timeout_str);
    } catch (const std::exception& e) {
        log_warn("Invalid timeout_seconds '%s', using default 60", timeout_str.c_str());
        timeout_ = 60;
    }
    // 标记加载成功
    isLoaded_ = true;
    return true;
}

std::string Configure::get(const std::string& key, const std::string& defaultValue) {
    std::map<std::string, std::string>::const_iterator it = configMap_.find(key);
    if (it != configMap_.end()) {
        return it->second;
    }
    return defaultValue;
}