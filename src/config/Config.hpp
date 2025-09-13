#pragma once

// Placeholder for the configuration parsing module as per T005.
// Implementation will be provided by the user.
#include <string>
#include <map>
#include <iostream>


class Configure {
public:
    Configure(const std::string& configFile);
    ~Configure() = default;

    bool isLoaded() const { return isLoaded_; }

    // Type-safe getters
    int getPort() const { return port_; }
    std::string getLogLevel() const { return logLevel_; }
    int getTimeout() const { return timeout_; }

private:
    bool load(const std::string& configFile);
    std::string get(const std::string& key, const std::string& defaultValue = "");

    std::map<std::string, std::string> configMap_;
    bool isLoaded_;

    // Parsed values
    int port_;
    std::string logLevel_;
    int timeout_;
};
