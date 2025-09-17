# This file defines the library targets for the application's source code.
# Source files will be added to these libraries in subsequent tasks.

# Data model library (T010, T011)
add_library(model_lib INTERFACE)
target_include_directories(model_lib INTERFACE ../include)
target_sources(model_lib INTERFACE
    ../include/model/Entities.hpp
    ../include/model/Room.hpp
)

# As per T001, define the library targets.
# These are currently empty and will be populated later.
add_library(storage_lib
    storage/SQLiteStorage.cpp
    storage/SQLiteStorage.hpp
)
target_link_libraries(storage_lib PUBLIC model_lib SQLite::SQLite3)  # Link SQLite
target_include_directories(storage_lib PUBLIC ../include .)  # Add current directory to include path

add_library(net_lib
    net/MiniEventAdapterImpl.cpp
    net/MiniEventAdapterImpl.h
    ../MiniEventWork/src/EVConnConnector.cpp
    ../MiniEventWork/src/BufferEvent.cpp
    ../MiniEventWork/src/Channel.cpp
    ../MiniEventWork/src/EventBase.cpp
    ../MiniEventWork/src/MiniEventLog.cpp
    ../MiniEventWork/src/Buffer.cpp
    ../MiniEventWork/src/MultiplexerSelector.cpp
    ../MiniEventWork/src/ConnectionManager.cpp
    ../MiniEventWork/src/InheritedFromIO_Multiplexer/Select_multiplexer.cpp
    ../MiniEventWork/src/InheritedFromIO_Multiplexer/Kqueue_multiplexer.cpp
)
target_link_libraries(net_lib PUBLIC model_lib nlohmann_json::nlohmann_json)
target_include_directories(net_lib PUBLIC ../include . ${CMAKE_SOURCE_DIR}/MiniEventWork/include)

add_library(viewmodel_lib
    viewmodel/LoginViewModel.cpp
    viewmodel/LoginViewModel.h
    viewmodel/ChatViewModel.cpp
    viewmodel/ChatViewModel.h
)
target_link_libraries(viewmodel_lib PUBLIC model_lib Qt6::Core)
target_include_directories(viewmodel_lib PUBLIC ../include .)  # Add current directory to include path

add_library(transfer_lib
    transfer/FileTransferManager.cpp
    transfer/FileTransferManager.h
)
target_link_libraries(transfer_lib PUBLIC model_lib)
target_include_directories(transfer_lib PUBLIC ../include .)  # Add current directory to include path

# Library for common utilities (T004)
add_library(common_lib
    common/Log.cpp
)
# Make headers in src/ available to other targets
target_include_directories(common_lib PUBLIC .)

# Library for config parser (T005)
add_library(config_lib
    config/Config.cpp
)
target_include_directories(config_lib PUBLIC .)

# Library for utility functions (T064, T065, T068)
add_library(util_lib
    util/Uuid.cpp
    util/Uuid.h
    util/Sha256.cpp
    util/Sha256.h
    util/ThreadNaming.cpp
    util/ThreadNaming.h
)
target_include_directories(util_lib PUBLIC .)

# Library for internationalization (T071)
add_library(i18n_lib
    i18n/I18n.cpp
    i18n/I18n.h
)
target_include_directories(i18n_lib PUBLIC .)

message(STATUS "src/CMakeLists.txt configured with placeholder libraries.")
