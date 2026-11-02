#pragma once

#include <cstdint>
#include <string>
#include <optional>

struct ReceivedDatagram
{
    std::string message;
    std::string sender_ip;
    std::uint16_t sender_port;
};

class UdpSocket
{
public:

    // Prevent implicit conversion from int
    explicit UdpSocket(std::uint16_t port);
    ~UdpSocket();

    // Broadcast function which sends to all devices
    void send_broadcast(const std::string& message);

    // Try and receive a message with this socket
    std::optional<ReceivedDatagram> try_receive();

    int get_fd() const {return fd;}
   
private:
    std::uint16_t port;

    int fd; // If we extend this to windows this would have to refer to
    // the windows socket handle, and have to change the type also to intptr_t
};