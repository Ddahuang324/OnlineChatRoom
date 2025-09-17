cmake_minimum_required(VERSION 3.16)
project(OnlineChat VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

include(cmake/Options.cmake)

# --- Fetch Dependencies ---
include(FetchContent)

FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

# --- Find Packages ---
find_package(Qt6 COMPONENTS Core Gui Qml Quick REQUIRED)

# 启用 QML 语言服务器配置文件生成
set(QT_QML_GENERATE_QMLLS_INI ON)

# 设置 QML 导入路径
set(QML_IMPORT_PATH "${CMAKE_SOURCE_DIR}/qml" CACHE STRING "QML Import Path")

# 获取 Qt6 QML 插件路径
get_target_property(QT_QML_PLUGIN_PATH Qt6::Qml IMPORTED_LOCATION)
get_filename_component(QT_QML_PLUGIN_DIR ${QT_QML_PLUGIN_PATH} DIRECTORY)
set(QML2_IMPORT_PATH "${QT_QML_PLUGIN_DIR}/../qml" CACHE STRING "QML2 Import Path")

if(CHAT_ENABLE_SQLITE)
    find_package(SQLite3) # Not set to REQUIRED to allow configuration without it
endif()

if(CHAT_BUILD_TESTS)
    if(APPLE)
        # Hint for Homebrew-installed googletest
        list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/googletest")
    endif()
    find_package(GTest)
endif()

# --- Subdirectories ---
add_subdirectory(src)

if(CHAT_BUILD_TESTS)
    add_subdirectory(tests)
endif()

# --- Main Executable Target ---
# The main_app executable will be fully defined in a later task (T043)
# For now, we declare it to have other targets link against it if necessary.
add_executable(chat_app src/app/main.cpp resources.qrc)

target_link_libraries(chat_app PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    model_lib
    common_lib
    config_lib
    util_lib
    i18n_lib
    storage_lib
    net_lib
    viewmodel_lib
    transfer_lib
)

if(CHAT_ENABLE_SQLITE AND SQLite3_FOUND)
    target_link_libraries(chat_app PRIVATE SQLite::SQLite3)
endif()

# Message for user
message(STATUS "Top-level CMakeLists.txt configured.")
message(STATUS "  - SQLite support: ${CHAT_ENABLE_SQLITE}")
message(STATUS "  - QML support: ${CHAT_ENABLE_QML}")
message(STATUS "  - Build tests: ${CHAT_BUILD_TESTS}")
