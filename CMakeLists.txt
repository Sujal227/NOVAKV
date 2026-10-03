cmake_minimum_required(VERSION 3.20)

project(NovaKV VERSION 1.0 LANGUAGES CXX)


# ============================================================
# C++ STANDARD
# ============================================================

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_EXTENSIONS OFF)


# ============================================================
# WINDOWS
# ============================================================

if(WIN32)

    add_definitions(
        -D_WIN32_WINNT=0x0601
    )

endif()


# ============================================================
# CORE LIBRARY
# ============================================================

add_library(
    novakv_core

    src/kv_store.cpp
    src/persistence.cpp
    src/thread_pool.cpp
)


target_include_directories(
    novakv_core

    PUBLIC

    ${PROJECT_SOURCE_DIR}/include
)


# ============================================================
# SERVER
# ============================================================

add_executable(
    novakv

    src/main.cpp
    src/server.cpp
)


target_link_libraries(
    novakv

    PRIVATE

    novakv_core
)


if(WIN32)

    target_link_libraries(
        novakv

        PRIVATE

        ws2_32
    )

endif()


# ============================================================
# CLIENT
# ============================================================

add_executable(
    novakv_client

    src/client_main.cpp
    src/client.cpp
)


target_include_directories(
    novakv_client

    PRIVATE

    ${PROJECT_SOURCE_DIR}/include
)


if(WIN32)

    target_link_libraries(
        novakv_client

        PRIVATE

        ws2_32
    )

endif()


# ============================================================
# GOOGLE TEST
# ============================================================

enable_testing()


find_package(
    GTest REQUIRED
)


# ============================================================
# TEST EXECUTABLE
# ============================================================

add_executable(
    novakv_tests

    tests/kv_store_test.cpp
    tests/thread_pool_test.cpp
)


target_include_directories(
    novakv_tests

    PRIVATE

    ${PROJECT_SOURCE_DIR}/include
)


target_link_libraries(
    novakv_tests

    PRIVATE

    novakv_core
    GTest::gtest
    GTest::gtest_main
)


include(
    GoogleTest
)


gtest_discover_tests(
    novakv_tests
)