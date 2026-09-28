#include "udp_socket.hpp"

#include <stdexcept>

// Linux socket APIs
#include <arpa/inet.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>


UdpSocket::UdpSocket(std::uint16_t port)
	// Start with an invalid handle so the object has a known initial state.
	: port(port)
	, socket_handle(-1)
{
	// Create an IPv4 UDP socket.
	const int native_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (native_socket < 0)
	{
		throw std::runtime_error("socket creation failed");
	}

	// Keep the descriptor so other methods and the destructor can use it.
	socket_handle = native_socket;

	// Allow this socket to send datagrams to a broadcast address.
	int broadcast_enabled = 1;
	// Use the generic socket configuration syscall
	if (setsockopt(
			socket_handle,
			SOL_SOCKET, 		// General socket-level option (could be IPv4 specific, etc)
			SO_BROADCAST,		// The actual option name
			&broadcast_enabled,	// Option value
			sizeof(broadcast_enabled)) < 0)	// Size of option_value
	{
		perror("setsockopt");
		close(socket_handle);
		throw std::runtime_error("setsockopt failed");
	}

	// Describe the local IPv4 address and port for bind().
	sockaddr_in address{};
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	address.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(
			socket_handle,
			reinterpret_cast<sockaddr*>(&address),
			sizeof(address)) < 0)
	{
		// Release the socket because construction cannot continue.
		perror("bind socket");
		close(socket_handle);
		throw std::runtime_error("bind failed");
	}
}

UdpSocket::~UdpSocket()
{
	if (socket_handle >= 0)
	{
		// Release the kernel socket when the C++ object is destroyed.
		close(socket_handle);
	}
}


void UdpSocket::send_broadcast(const std::string& message)
{
	sockaddr_in destination{};
    destination.sin_family = AF_INET;
	// Use the port number that all program Udp sockets have here
    destination.sin_port = htons(port);
    inet_pton(AF_INET, "255.255.255.255", &destination.sin_addr);
	
	// Signed size_t as can return negative
	const ssize_t bytes_sent = sendto(
        socket_handle,
        message.data(),
        message.size(),
        0,
        reinterpret_cast<sockaddr*>(&destination),
        sizeof(destination)
    );

	if (bytes_sent < 0)
	{
		perror("Send broadcast");
		throw std::runtime_error("send broadcast failed");
	}
}

std::optional<ReceivedDatagram> UdpSocket::receive()
{
	char buffer[1024];
	sockaddr_in sender{};
	socklen_t sender_length = sizeof(sender);

	// Wait for one UDP datagram and record the sender's address.
	const ssize_t bytes_received = recvfrom(
		socket_handle,
		buffer,
		sizeof(buffer),
		0,
		reinterpret_cast<sockaddr*>(&sender),
		&sender_length
	);

	if (bytes_received < 0)
	{
		perror("Receive datagram");
		throw std::runtime_error("recvfrom failed");
	}

	// Reserve enough space for the longest printable IPv4 address and its null terminator.
	char sender_ip[INET_ADDRSTRLEN]{};
	// Convert the sender's binary IPv4 address into readable text such as "192.168.1.20".
	if (inet_ntop(
			AF_INET,
			&sender.sin_addr,
			sender_ip,
			sizeof(sender_ip)) == nullptr)
	{
		throw std::runtime_error("inet_ntop failed");
	}

	return ReceivedDatagram{
		std::string(buffer, bytes_received),
		sender_ip,								// Construct C++ string from C style string
		ntohs(sender.sin_port)					// CONvert port from network to machine byte order
	};
}

std::optional<ReceivedDatagram> UdpSocket::receive_for(
	std::chrono::milliseconds timeout)
{
	const int timeout_ms = static_cast<int>(timeout.count());

	pollfd socket_readable{};
	socket_readable.fd = static_cast<int>(socket_handle);
	socket_readable.events = POLLIN;

	const int result = poll(&socket_readable, 1, static_cast<int>(timeout_ms));
	if (result == 0)
	{
		return std::nullopt;
	}
	if (result < 0)
	{
		perror("poll");
		throw std::runtime_error("poll failed");
	}

	// revents is a bitmask of events that actually occurred; & POLLIN checks
	// whether the "data available to read" flag is set. The flags are not the
	// packet data itself — poll() only reports that data is ready for recvfrom().
	if ((socket_readable.revents & POLLIN) == 0)
	{
		throw std::runtime_error("socket became ready without a datagram");
	}

	return receive();
}