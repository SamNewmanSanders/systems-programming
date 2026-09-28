#pragma once

#include <chrono>
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

    // Disable copy constructor and assignment
    UdpSocket(const UdpSocket&) = delete;
    UdpSocket& operator=(const UdpSocket&) = delete;

    // Broadcast function which sends to all devices
    void send_broadcast(
        const std::string& message
    );

    std::optional<ReceivedDatagram> receive();
    std::optional<ReceivedDatagram> receive_for(
        std::chrono::milliseconds timeout
    );

   
private:
    std::uint16_t port;
    std::intptr_t socket_handle; // Refers to either the linux file descriptor or Windows equivalent
    // intptr_t conveys that the integer is intended to contain a pointer
};