#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>

#include "peerDiscovery.h"


int main()
{
    // Always use port 9001 - hopefully is free! (Otherwise systemcall error)
    const std::uint16_t discoveryPort = 9001;
    // Constexpr is stronger than const - it is constant and KNOWN at COMPILE TIME
    constexpr auto broadcastInterval = std::chrono::milliseconds(1000);
    const std::string broadcastMessageText = "SAMNS_LAN_PEER_DISCOVERY_";

    PeerDiscoverer peerDiscoverer(discoveryPort, broadcastMessageText, broadcastInterval);
    peerDiscoverer.discoverPeers();

    peerDiscoverer.discoverPeers();


    // Depending on whether we are the server or the client, do different things
    switch (peerDiscoverer.role)
    {
    case Role::CLIENT:
        // Connect to peerDiscoverer.peer
        break;

    case Role::SERVER:
        // Accept the client's connection
        break;

    case Role::UNKNOWN:
        // Discovery did not assign a role
        break;
    }

    return 0;
}