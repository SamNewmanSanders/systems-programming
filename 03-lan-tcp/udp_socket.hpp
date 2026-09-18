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

    // Explicit constructor to avoid accidental type conversion
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

   
private:
    std::uint16_t port;
    std::intptr_t socket_handle;

};