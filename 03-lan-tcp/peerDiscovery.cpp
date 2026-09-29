#include "peerDiscovery.h"

#include <random>
#include <iostream>


// Generate random peer_id
std::uint64_t PeerDiscoverer::generatePeerID()
{   
    std::random_device random_device;
    std::mt19937_64 generator(random_device());
    return generator();
    // random_device provides an unpredictable seed; mt19937_64 uses it to
    // generate one pseudorandom 64-bit value. The generator is destroyed
    // when the function returns, so its state is not reused between calls.
    // Obviously fine here
}

void PeerDiscoverer::discoverPeers()
{
    // Broadcast immediately
    auto nextBroadcast = std::chrono::steady_clock::now();

    while (true)
    {
        const auto now = std::chrono::steady_clock::now();
        if (now >= nextBroadcast)
        {
            udpSocket.send_broadcast(broadcastMessage);
            nextBroadcast = now + broadcastInterval;
        }

        const auto waitTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            nextBroadcast - std::chrono::steady_clock::now());
        
        const auto received = udpSocket.receive_for(waitTime);

        if (received)
        {   
            // Stop self detection (if peer ID matches)
            if (received->message == broadcastMessage)
            {
                continue;
            }
            
            std::cout << "Datagram from " << received->sender_ip << ':'
                    << received->sender_port << ": " << received->message
                    << '\n';

        }
    }
}