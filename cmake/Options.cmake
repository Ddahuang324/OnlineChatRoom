# Build options as defined in T002

option(CHAT_ENABLE_SQLITE "Enable SQLite support" ON)
option(CHAT_ENABLE_QML "Enable QML support" ON)
option(CHAT_BUILD_TESTS "Build tests" ON)
option(CHAT_WARN_AS_ERR "Treat warnings as errors" OFF)

# Compiler warning settings
if(MSVC)
    # MSVC specific warning flags can be added here
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(CHAT_WARN_AS_ERR)
        add_compile_options(-Werror)
    endif()
endif()

message(STATUS "cmake/Options.cmake loaded.")
