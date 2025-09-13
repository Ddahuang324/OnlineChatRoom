#pragma once

// Placeholder for the logging utility as per T004.
// Implementation will be provided by the user.

#include <iostream>
#include <string>
#include <mutex>

#define log_debug(fmt , ...)LOG::getInstance()->log(LOG::LOG_LEVEL_DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define log_info(fmt , ...)LOG::getInstance()->log(LOG::LOG_LEVEL_INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define log_warn(fmt , ...)LOG::getInstance()->log(LOG::LOG_LEVEL_WARN, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define log_error(fmt , ...)LOG::getInstance()->log(LOG::LOG_LEVEL_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)


class LOG{

public:
    enum LogLevel {
        LOG_LEVEL_DEBUG,
        LOG_LEVEL_INFO,
        LOG_LEVEL_WARN,
        LOG_LEVEL_ERROR,
    };
public:
    static LOG* getInstance();
    void log(LogLevel level, const char* filename, int line, const char* fmt , ...);
    // Allow tests to inject an output stream (defaults to std::cout)
    void setOutputStream(std::ostream* os) { output_stream_ = os; }
    std::ostream* getOutputStream() const { return output_stream_; }
private:
    LOG() = default;
    ~LOG() = default;
    LOG(const LOG&) = delete;
    LOG& operator=(const LOG&) = delete;
    
    std::mutex m_mutex;
    std::ostream* output_stream_ = &std::cout;
};