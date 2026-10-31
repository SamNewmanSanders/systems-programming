#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

// Represents one established TCP connection, whether it was initiated with
// connect_to() or returned by TcpListener::accept().
class TcpConnection
{
public:
	// Connect to a listening peer at its IPv4 address and TCP port.
	static TcpConnection connect_to(
		const std::string& ip_address,
		std::uint16_t port
	);

	// A connection owns its OS socket, so it may be moved but not copied.
	~TcpConnection();

	TcpConnection(const TcpConnection&) = delete;
	TcpConnection& operator=(const TcpConnection&) = delete;
	TcpConnection(TcpConnection&& other) noexcept;
	TcpConnection& operator=(TcpConnection&& other) noexcept; // Suggested noexcept - learn more about this

	// Keep sending until all bytes have been accepted by the socket or an error occurs.
	void send_all(const char* data, std::size_t size);
	// Receive up to capacity bytes. Returns 0 when the peer has closed its send side.
	std::size_t receive_some(char* buffer, std::size_t capacity);

private:
	explicit TcpConnection(std::uintptr_t socket_handle);

	std::uintptr_t socket_handle; // Stored without exposing OS-specific socket headers here.

	friend class TcpListener;
};

// Owns the listening socket. accept() waits for a peer and returns a separate
// TcpConnection for communicating with that peer; the listener remains open.
class TcpListener
{
public:
	// Bind to a known local TCP port and begin listening for incoming connections.
	explicit TcpListener(std::uint16_t port);
	~TcpListener();

	TcpListener(const TcpListener&) = delete;
	TcpListener& operator=(const TcpListener&) = delete;

	// Blocking: waits until a connection arrives or the OS reports an error.
	TcpConnection accept();

private:
	std::uintptr_t socket_handle;
};
