// AI was used to generate this file without checking it. Learning Windows API's are not a priority for me

#include "udp_socket.hpp"

#include <stdexcept>

// Import platform specific libraries
#include <winsock2.h>
#include <ws2tcpip.h>

// Use an alias
using NativeSocket = SOCKET;

UdpSocket::UdpSocket(std::uint16_t port)
	: port(port)
{
	WSADATA windows_socket_data{};
	if (WSAStartup(MAKEWORD(2, 2), &windows_socket_data) != 0)
	{
		throw std::runtime_error("WSAStartup failed");
	}

	const NativeSocket native_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (native_socket == INVALID_SOCKET)
	{
		WSACleanup();
		throw std::runtime_error("socket creation failed");
	}

	socket_handle = static_cast<std::intptr_t>(native_socket);

	// Allow this socket to send datagrams to a broadcast address.
	int broadcast_enabled = 1;
	if (setsockopt(
			native_socket,
			SOL_SOCKET,
			SO_BROADCAST,
			reinterpret_cast<const char*>(&broadcast_enabled),
			sizeof(broadcast_enabled)) == SOCKET_ERROR)
	{
		closesocket(native_socket);
		WSACleanup();
		throw std::runtime_error("setsockopt failed");
	}

	sockaddr_in address{};
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	address.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(
			native_socket,
			reinterpret_cast<sockaddr*>(&address),
			sizeof(address)) < 0)
	{
		closesocket(native_socket);
		WSACleanup();
		throw std::runtime_error("bind failed");
	}
}

void UdpSocket::send_broadcast(const std::string& message)
{
	sockaddr_in destination{};
	destination.sin_family = AF_INET;
	destination.sin_port = htons(port);
	destination.sin_addr.s_addr = htonl(INADDR_BROADCAST);

	const NativeSocket native_socket =
		static_cast<NativeSocket>(socket_handle);
	const int bytes_sent = sendto(
		native_socket,
		message.data(),
		static_cast<int>(message.size()),
		0,
		reinterpret_cast<sockaddr*>(&destination),
		sizeof(destination)
	);

	if (bytes_sent == SOCKET_ERROR)
	{
		throw std::runtime_error("sendto failed");
	}
}

std::optional<ReceivedDatagram> UdpSocket::receive()
{
	char buffer[1024];
	sockaddr_in sender{};
	int sender_length = sizeof(sender);

	// Wait for one UDP datagram and record the sender's address.
	const NativeSocket native_socket =
		static_cast<NativeSocket>(socket_handle);
	const int bytes_received = recvfrom(
		native_socket,
		buffer,
		sizeof(buffer),
		0,
		reinterpret_cast<sockaddr*>(&sender),
		&sender_length
	);

	if (bytes_received == SOCKET_ERROR)
	{
		throw std::runtime_error("recvfrom failed");
	}

	// Reserve enough space for the longest printable IPv4 address and its null terminator.
	char sender_ip[INET_ADDRSTRLEN]{};
	// Convert the sender's binary IPv4 address into readable text.
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

UdpSocket::~UdpSocket()
{
	const NativeSocket native_socket =
		static_cast<NativeSocket>(socket_handle);

	if (native_socket != INVALID_SOCKET)
	{
		closesocket(native_socket);
		WSACleanup();
	}
}