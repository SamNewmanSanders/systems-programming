cmake_minimum_required(VERSION 3.16)

project(lan-tcp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compile only the socket implementation for the current operating system.
# Both files define UdpSocket, so compiling both would cause duplicate symbols.
if (WIN32)
	set(UDP_SOCKET_SOURCE udp_socket_windows.cpp)
else()
	set(UDP_SOCKET_SOURCE udp_socket_linux.cpp)
endif()

add_executable(main
	main.cpp
	${UDP_SOCKET_SOURCE}
)

# Winsock is a separate Windows networking library. Linux provides these
# basic socket functions through its standard system libraries.
if (WIN32)
	target_link_libraries(main PRIVATE ws2_32)
endif()
