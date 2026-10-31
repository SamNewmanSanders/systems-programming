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
            
            // If we have received a message, lets handle it and update state
            std::cout << "Datagram from " << received->sender_ip << ':'
                    << received->sender_port << ": " << received->message
                    << '\n';
            
            // Check that the received datagram is actually from our program
            if (received->message.rfind(broadcastMessageText, 0) != 0)
            {
                continue;
            }
            

            // Extract the peer ID
            const auto prefixLength = broadcastMessageText.size();
            const auto underscore = received->message.find('_', prefixLength);

            if (underscore == std::string::npos)
            {
                continue;
            }
            
            // String to unsinged long long
            const std::uint64_t otherPeerID = std::stoull(
                received->message.substr(prefixLength, underscore - prefixLength)
            );

            // If we have the higher peerID, we are the server
            role = peerID > otherPeerID ? Role::SERVER : Role::CLIENT;

            // Populate information about the peer we have found
            peer.id = otherPeerID;
            peer.ipAddress = received->sender_ip;

            break;
        }
    }

    return;
}