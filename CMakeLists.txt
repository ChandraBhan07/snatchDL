cmake_minimum_required(VERSION 3.15)
project(SnatchDL LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

add_executable(app src/main.cpp src/parser.cpp src/source.cpp src/fileManager.cpp src/cookie.cpp src/cookieManager.cpp src/downloader.cpp src/jobManager.cpp src/logger.cpp src/clipboardListener.cpp src/config.cpp src/commandRunner.cpp src/cmdOptions)

target_include_directories(app PRIVATE include)

