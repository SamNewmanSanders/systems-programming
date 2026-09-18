cmake_minimum_required(VERSION 3.16)

project(udp_intro LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(udp_server server.cpp)
add_executable(udp_client client.cpp)
