#pragma once

#include <chrono>
#include "udpSocket.hpp"

class PeerDiscoverer
{

public:

    std::string broadcastMessage;
    UdpSocket udpSocket;
    std::chrono::milliseconds broadcastInterval;

    PeerDiscoverer(const std::uint16_t discoveryPort,
                   const std::string broadcastMessageText,
                   const std::chrono::milliseconds broadcastInterval)
    : udpSocket(discoveryPort), broadcastInterval(broadcastInterval)
    {
        std::uint64_t peerId = generatePeerID();
        broadcastMessage = broadcastMessageText + std::to_string(peerId);
    }

    void discoverPeers();

private:

    std::uint64_t generatePeerID();

};