#pragma once

#include <chrono>
#include "sockets/udpSocket.hpp"

// Expands to a C-style string literal, usable with std::cout or std::string.
#if defined(_WIN32)
#define OS_NAME "Windows"
#elif defined(__linux__)
#define OS_NAME "Linux"
#else
#define OS_NAME "Unknown OS"
#endif

enum class Role{
    SERVER,
    CLIENT,
    UNKNOWN
};

// Group info about the peer in a struct - somewhat similar to datagram struct but cba
struct PeerInfo
{
    std::uint64_t id;
    std::string ipAddress;
    // OTHER STUFF TO DO WITH TCP??
};

class PeerDiscoverer
{
public:

    Role role;

    PeerInfo peer;

    std::uint64_t peerID;
    std::string broadcastMessage;
    std::string broadcastMessageText; // String representing the text part to compare later (bad code)
    UdpSocket udpSocket;
    std::chrono::milliseconds broadcastInterval;

    PeerDiscoverer(const std::uint16_t discoveryPort,
                   const std::string broadcastMessageText,
                   const std::chrono::milliseconds broadcastInterval)
    : role(Role::UNKNOWN), udpSocket(discoveryPort), 
      broadcastInterval(broadcastInterval), broadcastMessageText(broadcastMessageText)
    {
        peerID = generatePeerID();
        broadcastMessage =  broadcastMessageText + 
                            std::to_string(peerID) +
                            "_" + OS_NAME;
    }

    void discoverPeers();

private:

    std::uint64_t generatePeerID();
};