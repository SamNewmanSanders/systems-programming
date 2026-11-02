#include "../udpSocket.hpp"

#include <stdexcept>

// Linux socket APIs
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>

UdpSocket::UdpSocket(std::uint16_t port)
	// Start with an invalid fd so the object has a known initial state.
	: port(port)
	, fd(-1)
{
	// Create an IPv4 UDP socket.
    // Created with SOCK_NONBLOCK: if no datagram is waiting, recvfrom() returns -1 with
    // errno EAGAIN instead of sleeping. poll() decides when to wait; a readable result
    // is not a guarantee (e.g. a corrupt datagram may be discarded), so a blocking
    // receive could still freeze the whole event loop.
	const int sock = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
	if (sock < 0)
	{
		throw std::runtime_error("socket creation failed");
	}

	// Keep the descriptor so other methods and the destructor can use it.
	fd = sock;

	// Allow this socket to send datagrams to a broadcast address.
	int broadcast_enabled = 1;
	// Use the generic socket configuration syscall
	if (setsockopt(
			fd,
			SOL_SOCKET, 		// General socket-level option (could be IPv4 specific, etc)
			SO_BROADCAST,		// The actual option name
			&broadcast_enabled,	// Option value
			sizeof(broadcast_enabled)) < 0)	// Size of option_value
	{
		perror("setsockopt");
		close(fd);
		throw std::runtime_error("setsockopt failed");
	}

	// Describe the local IPv4 address and port for bind().
	sockaddr_in address{};
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	address.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(
			fd,
			reinterpret_cast<sockaddr*>(&address),
			sizeof(address)) < 0)
	{
		// Release the socket because construction cannot continue.
		perror("bind socket");
		close(fd);
		throw std::runtime_error("bind failed");
	}
}

UdpSocket::~UdpSocket()
{
	if (fd >= 0)
	{
		// Release the kernel socket when the C++ object is destroyed.
		close(fd);
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
        fd,
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

std::optional<ReceivedDatagram> UdpSocket::try_receive()
{
	char buffer[1024];
	sockaddr_in sender{};
	socklen_t sender_length = sizeof(sender);

	// Wait for one UDP datagram (NON BLOCKING) and record the sender's address.
	const ssize_t bytes_received = recvfrom(
		fd,
		buffer,
		sizeof(buffer),
		0,
		reinterpret_cast<sockaddr*>(&sender),
		&sender_length
	);

	if (bytes_received < 0)
	{   

        if (errno == EAGAIN)
        {
            return std::nullopt;   // nothing waiting right now; not an error
        }
        
        // Otherwise a more serious error has occured
		perror("Receive datagram");
		throw std::runtime_error("recvfrom failed");        // Check if the errno 
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

	// A string's end is known either from a length or a '\0' terminator; use whichever
	// the API provides. recvfrom() returns a length but the datagram has no '\0', so
	// pass the length. inet_ntop() writes a '\0' but returns no length, so let the
	// string stop at it (sizeof(sender_ip) would include the unused trailing zeros).
	return ReceivedDatagram{
		std::string(buffer, bytes_received),
		std::string(sender_ip),					// Construct C++ string from C style string
		ntohs(sender.sin_port)					// Convert port from network to machine byte order
	};
}

