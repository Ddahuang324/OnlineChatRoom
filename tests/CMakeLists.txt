# This file defines the test targets.
# Test source files will be added in subsequent tasks.

if(NOT CHAT_BUILD_TESTS OR NOT GTest_FOUND)
    message(STATUS "Skipping tests configuration.")
    return()
endif()

# As per T001, define the 'tests' target.
# This will be populated with test sources later.
add_executable(unit_tests
    main.cpp
    config/test_config.cpp
    contract/test_network_connect.cpp
    contract/test_network_file_chunk.cpp
    contract/test_storage_user.cpp
    contract/test_storage_messages.cpp
    storage/test_sqlite.cpp
    net/test_mini_event_adapter.cpp
    viewmodel/test_login.cpp
    viewmodel/test_chat.cpp
    transfer/test_file_transfer.cpp
    integration/test_e2e_login_send.cpp
    integration/test_e2e_file_reconnect.cpp
)

target_include_directories(unit_tests PRIVATE
    .
    mocks
    support
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(unit_tests PRIVATE
    GTest::gtest_main
    GTest::gmock
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

message(STATUS "tests/CMakeLists.txt configured with placeholder test target.")
