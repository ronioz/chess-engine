cmake_minimum_required(VERSION 3.20)
project(ChessEngine CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(CHESS_ENGINE_BUILD_TESTS "Build tests" OFF)

add_subdirectory(src)

if(CHESS_ENGINE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
    list(APPEND CMAKE_CTEST_ARGUMENTS "--output-on-failure")
endif()
