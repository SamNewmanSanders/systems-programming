#include "udp_socket.hpp"

#include <stdexcept>

// Linux socket APIs
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

// Linux represents sockets as integer file descriptors.
using NativeSocket = int;

UdpSocket::UdpSocket(std::uint16_t port)
	// Start with an invalid handle so the object has a known initial state.
	: port(port)
	, socket_handle(-1)
{
	// Create an IPv4 UDP socket.
	const NativeSocket native_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (native_socket < 0)
	{
		throw std::runtime_error("socket creation failed");
	}

	// Keep the descriptor so other methods and the destructor can use it.
	socket_handle = native_socket;

	// Allow this socket to send datagrams to a broadcast address.
	int broadcast_enabled = 1;
	if (setsockopt(
			native_socket,
			SOL_SOCKET,
			SO_BROADCAST,
			&broadcast_enabled,
			sizeof(broadcast_enabled)) < 0)
	{
		close(native_socket);
		throw std::runtime_error("setsockopt failed");
	}

	// Describe the local IPv4 address and port for bind().
	sockaddr_in address{};
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	address.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(
			native_socket,
			reinterpret_cast<sockaddr*>(&address),
			sizeof(address)) < 0)
	{
		// Release the socket because construction cannot continue.
		close(native_socket);
		throw std::runtime_error("bind failed");
	}
}

UdpSocket::~UdpSocket()
{
	// Convert the shared storage back to Linux's native descriptor type.
	const NativeSocket native_socket =
		static_cast<NativeSocket>(socket_handle);

	if (native_socket >= 0)
	{
		// Release the kernel socket when the C++ object is destroyed.
		close(native_socket);
	}
}


void UdpSocket::send_broadcast(const std::string& message)
{
	sockaddr_in destination{};
    destination.sin_family = AF_INET;
	// Use the port number that all program Udp sockets have here
    destination.sin_port = htons(port);
    inet_pton(AF_INET, "255.255.255.255", &destination.sin_addr);
	
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
		throw std::runtime_error("sendto failed");
	}
}

std::optional<ReceivedDatagram> UdpSocket::receive()
{
	char buffer[1024];
	sockaddr_in sender{};
	socklen_t sender_length = sizeof(sender);

	// Wait for one UDP datagram and record the sender's address.
	const ssize_t bytes_received = recvfrom(
		static_cast<NativeSocket>(socket_handle),
		buffer,
		sizeof(buffer),
		0,
		reinterpret_cast<sockaddr*>(&sender),
		&sender_length
	);

	if (bytes_received < 0)
	{
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
		sender_ip,
		ntohs(sender.sin_port)
	};
}