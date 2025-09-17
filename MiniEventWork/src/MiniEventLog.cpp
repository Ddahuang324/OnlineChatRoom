#include <mutex>
#include <chrono>
#include <cstdarg>
#include <iostream>
#include <iomanip>
#include <sstream>
#include "../include/MiniEventLog.hpp"

namespace MiniEventWork {

MiniEventLog* MiniEventLog::getInstance(){
    static MiniEventLog instance;
    return &instance;
 }

void MiniEventLog::log(LogLevel level, const char* filename, int line, const char* fmt , ...) { 
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // 获取当前时间并格式化为 UTC ISO8601 (e.g. 2025-09-11T10:05:03.123Z)
    auto now = std::chrono::system_clock::now();
    auto current_time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::tm tm_utc;
    #if defined(_WIN32) || defined(_WIN64)
        gmtime_s(&tm_utc, &current_time);
    #else
        gmtime_r(&current_time, &tm_utc);
    #endif

    std::stringstream ss;
    ss << std::put_time(&tm_utc, "%Y-%m-%dT%H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(3) << ms.count() << "Z";

    const char* level_str ;

    switch (level) {
    case LOG_LEVEL_DEBUG:
        level_str = "TRACE"; // map DEBUG->TRACE for more granular level if desired
        break;
    case LOG_LEVEL_INFO:
        level_str = "INFO";
        break;
    case LOG_LEVEL_WARN:
        level_str = "WARN";
        break;
    case LOG_LEVEL_ERROR:
        level_str = "ERROR";
        break;
    default:
        level_str = "UNKNOWN";
        break;
    }
    
    char final_buf[2048] = {0};
    char content_buf[1024] = {0};

    va_list args;
    va_start(args, fmt);
    vsnprintf(content_buf, sizeof(content_buf)-1, fmt, args);
    va_end(args);
    // derive component name from filename (basename)
    const char* comp = filename;
    const char* slash = std::strrchr(filename, '/');
    if (slash) comp = slash + 1;
    // format: <ISO8601> <LEVEL> <COMPONENT> - <MESSAGE>
    snprintf(final_buf, sizeof(final_buf) - 1, "%s %s %s - %s\n", ss.str().c_str(), level_str, comp, content_buf);
    // write to injectable output stream (default std::cout)
    MiniEventLog* inst = MiniEventLog::getInstance();
    std::ostream* out = inst ? inst->getOutputStream() : &std::cout;
    if (out) {
        (*out) << final_buf;
        out->flush();
    }

 }

} // namespace MiniEventWork
