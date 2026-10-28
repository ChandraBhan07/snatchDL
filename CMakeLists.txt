cmake_minimum_required(VERSION 3.15)
project(SnatchDL LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

add_executable(snatchDL src/main.cpp 
src/cli/cliOptions.cpp 
src/config/config.cpp
src/service/serviceRegistry.cpp
src/url/url.cpp 
src/input/inputFile.cpp
src/job/jobHistory.cpp
src/job/jobManager.cpp
src/job/jobBuilder.cpp
src/cookie/cookieManager.cpp 
src/downloader/downloader.cpp
src/listener/listenerWorker.cpp


)

# Platform-specific sources
if(WIN32)
    target_sources(snatchDL PRIVATE
        src/config/platform/configWindows.cpp
        src/listener/platform/listenerWindows.cpp
        src/downloader/platform/processRunnerWindows.cpp
        src/shutdown/platform/shutdownWindows.cpp
        )

endif() 

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(snatchDL PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow 
        -Wconversion 
    ) 
endif()


# Statically link MinGW runtime libraries on Windows.
if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_options(snatchDL PRIVATE
        -static
        -static-libgcc
        -static-libstdc++
    )
endif()