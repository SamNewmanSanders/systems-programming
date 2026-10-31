#include "tcpSocket.hpp"

#include <cerrno>
#include <limits>
#include <stdexcept>
#include <utility>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace
{
// Use a value that cannot be a valid Linux file descriptor to mark moved-from objects.
constexpr std::uintptr_t invalid_socket_handle =
	std::numeric_limits<std::uintptr_t>::max();

// Linux socket calls use int file descriptors; the shared header stores the handle portably.
int native_socket(std::uintptr_t socket_handle)
{
	return static_cast<int>(socket_handle);
}

void close_socket(std::uintptr_t socket_handle) noexcept
{
	if (socket_handle != invalid_socket_handle)
	{
		// Release the kernel socket when its owning C++ object is destroyed.
		close(native_socket(socket_handle));
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
	if (inet_pton(AF_INET, ip_address.c_str(), &address.sin_addr) != 1)
	{
		throw std::runtime_error("invalid IPv4 address: " + ip_address);
	}

	const int socket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (socket < 0)
	{
		perror("socket");
		throw std::runtime_error("TCP socket creation failed");
	}

	if (connect(socket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0)
	{
		perror("connect");
		close(socket);
		throw std::runtime_error("TCP connect failed");
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
		const ssize_t result = send(
			native_socket(socket_handle),
			data + sent,
			size - sent,
			MSG_NOSIGNAL);

		if (result > 0)
		{
			sent += static_cast<std::size_t>(result);
			continue;
		}
		if (result < 0 && errno == EINTR)
		{
			// A signal interrupted this system call; retry the unsent bytes.
			continue;
		}

		if (result < 0)
		{
			perror("send");
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

	while (true)
	{
		const ssize_t result = recv(
			native_socket(socket_handle),
			buffer,
			capacity,
			0);

		if (result >= 0)
		{
			// recv() returns the bytes available now; zero means orderly peer shutdown.
			return static_cast<std::size_t>(result);
		}
		if (errno == EINTR)
		{
			// Retry if a signal interrupted the blocking receive.
			continue;
		}

		perror("recv");
		throw std::runtime_error("TCP receive failed");
	}
}

TcpListener::TcpListener(std::uint16_t port)
	: socket_handle(invalid_socket_handle)
{
	const int socket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (socket < 0)
	{
		perror("socket");
		throw std::runtime_error("TCP listener socket creation failed");
	}

	// Listen on this port on every local IPv4 interface.
	sockaddr_in address{};
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	address.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(socket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0)
	{
		perror("bind");
		close(socket);
		throw std::runtime_error("TCP listener bind failed");
	}

	if (listen(socket, SOMAXCONN) < 0)
	{
		perror("listen");
		close(socket);
		throw std::runtime_error("TCP listen failed");
	}

	socket_handle = static_cast<std::uintptr_t>(socket);
}

TcpListener::~TcpListener()
{
	close_socket(socket_handle);
}

TcpConnection TcpListener::accept()
{
	while (true)
	{
		// accept() blocks until a client connects and returns a new socket for that connection.
		const int accepted_socket = ::accept(
			native_socket(socket_handle),
			nullptr,
			nullptr);

		if (accepted_socket >= 0)
		{
			return TcpConnection(static_cast<std::uintptr_t>(accepted_socket));
		}
		if (errno == EINTR)
		{
			// Retry when a signal interrupts accept().
			continue;
		}

		perror("accept");
		throw std::runtime_error("TCP accept failed");
	}
}
