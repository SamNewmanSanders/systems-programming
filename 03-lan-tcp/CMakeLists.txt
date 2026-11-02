cmake_minimum_required(VERSION 3.16)
project(lan-tcp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compile only the socket implementation for the current operating system.
# Both files define UdpSocket, so compiling both would cause duplicate symbols.

add_executable(main
	main.cpp
	beacon.cpp
	linux/udpSocketLinux.cpp
)


