cmake_minimum_required(VERSION 3.10.2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(telemetry)

add_executable(telemetry
    telemetry.cpp
)

find_package(MAVSDK REQUIRED)
find_package(Threads REQUIRED)

target_link_libraries(telemetry
    MAVSDK::mavsdk
    Threads::Threads
)

if(NOT MSVC)
    add_compile_options(telemetry PRIVATE -Wall -Wextra)
else()
    add_compile_options(telemetry PRIVATE -WX -W2)
endif()