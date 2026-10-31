// AI was used to generate this file without checking it. Learning Windows API's are not a priority for me

#include "tcpSocket.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

#include <winsock2.h>
#include <ws2tcpip.h>

namespace
{
// Use a value that cannot be a valid Winsock socket to mark moved-from objects.
constexpr std::uintptr_t invalid_socket_handle =
	std::numeric_limits<std::uintptr_t>::max();

SOCKET native_socket(std::uintptr_t socket_handle)
{
	return static_cast<SOCKET>(socket_handle);
}

void start_winsock()
{
	// Winsock must be initialized before using its socket functions.
	WSADATA data{};
	if (WSAStartup(MAKEWORD(2, 2), &data) != 0)
	{
		throw std::runtime_error("WSAStartup failed");
	}
}

void throw_winsock_error(const char* operation)
{
	throw std::runtime_error(
		std::string(operation) + " failed (Winsock error " +
		std::to_string(WSAGetLastError()) + ")");
}

void close_socket(std::uintptr_t socket_handle) noexcept
{
	if (socket_handle != invalid_socket_handle)
	{
		// Close the OS socket and release this object's Winsock initialization.
		closesocket(native_socket(socket_handle));
		WSACleanup();
	}
}
}

TcpConnection::TcpConnection(std::uintptr_t socket_handle)
	: socket_handle(socket_handle)
{
}

TcpConnection::~TcpConnection()
{
	close_socket(socket_handle);
}

TcpConnection::TcpConnection(TcpConnection&& other) noexcept
	// Transfer ownership and invalidate the source so it will not close this socket.
	: socket_handle(std::exchange(other.socket_handle, invalid_socket_handle))
{
}

TcpConnection& TcpConnection::operator=(TcpConnection&& other) noexcept
{
	if (this != &other)
	{
		close_socket(socket_handle);
		// Take the other object's socket and leave it empty.
		socket_handle = std::exchange(other.socket_handle, invalid_socket_handle);
	}
	return *this;
}

TcpConnection TcpConnection::connect_to(
	const std::string& ip_address,
	std::uint16_t port)
{
	// Build the IPv4 endpoint that connect() will contact.
	sockaddr_in address{};
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	if (InetPtonA(AF_INET, ip_address.c_str(), &address.sin_addr) != 1)
	{
		throw std::runtime_error("invalid IPv4 address: " + ip_address);
	}

	start_winsock();
	const SOCKET socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socket == INVALID_SOCKET)
	{
		const int error = WSAGetLastError();
		WSACleanup();
		throw std::runtime_error(
			"TCP socket creation failed (Winsock error " + std::to_string(error) + ")");
	}

	if (connect(socket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR)
	{
		const int error = WSAGetLastError();
		closesocket(socket);
		WSACleanup();
		throw std::runtime_error(
			"TCP connect failed (Winsock error " + std::to_string(error) + ")");
	}

	return TcpConnection(static_cast<std::uintptr_t>(socket));
}

void TcpConnection::send_all(const char* data, std::size_t size)
{
	if (size > 0 && data == nullptr)
	{
		throw std::invalid_argument("send_all received a null data pointer");
	}

	std::size_t sent = 0;
	while (sent < size)
	{
		// TCP is a byte stream: one send() can accept fewer bytes than requested.
		const int chunk_size = static_cast<int>(std::min<std::size_t>(
			size - sent,
			static_cast<std::size_t>(std::numeric_limits<int>::max())));
		const int result = send(
			native_socket(socket_handle),
			data + sent,
			chunk_size,
			0);

		if (result > 0)
		{
			sent += static_cast<std::size_t>(result);
			continue;
		}
		if (result == SOCKET_ERROR && WSAGetLastError() == WSAEINTR)
		{
			continue;
		}
		if (result == SOCKET_ERROR)
		{
			throw_winsock_error("send");
		}
		throw std::runtime_error("TCP send failed or connection closed");
	}
}

std::size_t TcpConnection::receive_some(char* buffer, std::size_t capacity)
{
	if (capacity == 0 || buffer == nullptr)
	{
		throw std::invalid_argument("receive_some requires a non-empty buffer");
	}

	const int chunk_size = static_cast<int>(std::min<std::size_t>(
		capacity,
		static_cast<std::size_t>(std::numeric_limits<int>::max())));
	const int result = recv(native_socket(socket_handle), buffer, chunk_size, 0);
	if (result == SOCKET_ERROR)
	{
		throw_winsock_error("recv");
	}

	// recv() returns the bytes available now; zero means orderly peer shutdown.
	return static_cast<std::size_t>(result);
}

TcpListener::TcpListener(std::uint16_t port)
	: socket_handle(invalid_socket_handle)
{
	start_winsock();
	const SOCKET socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socket == INVALID_SOCKET)
	{
		const int error = WSAGetLastError();
		WSACleanup();
		throw std::runtime_error(
			"TCP listener socket creation failed (Winsock error " + std::to_string(error) + ")");
	}

	// Listen on this port on every local IPv4 interface.
	sockaddr_in address{};
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	address.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(socket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR)
	{
		const int error = WSAGetLastError();
		closesocket(socket);
		WSACleanup();
		throw std::runtime_error(
			"TCP listener bind failed (Winsock error " + std::to_string(error) + ")");
	}

	if (listen(socket, SOMAXCONN) == SOCKET_ERROR)
	{
		const int error = WSAGetLastError();
		closesocket(socket);
		WSACleanup();
		throw std::runtime_error(
			"TCP listen failed (Winsock error " + std::to_string(error) + ")");
	}

	socket_handle = static_cast<std::uintptr_t>(socket);
}

TcpListener::~TcpListener()
{
	close_socket(socket_handle);
}

TcpConnection TcpListener::accept()
{
	start_winsock();
	// accept() blocks until a client connects and returns a new socket for that connection.
	const SOCKET accepted_socket = ::accept(
		native_socket(socket_handle),
		nullptr,
		nullptr);
	if (accepted_socket == INVALID_SOCKET)
	{
		const int error = WSAGetLastError();
		WSACleanup();
		throw std::runtime_error(
			"TCP accept failed (Winsock error " + std::to_string(error) + ")");
	}

	return TcpConnection(static_cast<std::uintptr_t>(accepted_socket));
}
