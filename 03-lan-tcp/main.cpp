#include <chrono>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>

#include "udp_socket.hpp"

std::uint64_t generate_peer_id()
{
    std::random_device random_device;
    std::mt19937_64 generator(random_device());
    return generator();
    // random_device provides an unpredictable seed; mt19937_64 uses it to
    // generate one pseudorandom 64-bit value. The generator is destroyed
    // when the function returns, so its state is not reused between calls.
    // Obviously fine here
}


int main()
{
    // Always use port 9001 - hopefully is free! (Otherwise systemcall error)
    const std::uint16_t discovery_port = 9001;
    const std::uint64_t peer_id = generate_peer_id();

    UdpSocket socket(discovery_port);

    const std::string discovery_message =
        "SAMNS_LAN_PEER_DISCOVERY " + std::to_string(peer_id);

    // Constexpr is stronger than const - it is constant and KNOWN at COMPILE TIME
    constexpr auto broadcast_interval = std::chrono::seconds(1);
    // Make the first broadcast happen immediately
    auto next_broadcast = std::chrono::steady_clock::now();

    while (true)
    {
        const auto now = std::chrono::steady_clock::now();
        if (now >= next_broadcast)
        {
            socket.send_broadcast(discovery_message);
            next_broadcast = now + broadcast_interval;
        }

        const auto wait_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            next_broadcast - std::chrono::steady_clock::now());
        const auto received = socket.receive_for(wait_time);

        if (received)
        {   
            // Stop self detection (if peer ID matches)
            if (received->message == discovery_message)
            {
                continue;
            }
            
            std::cout << "Datagram from " << received->sender_ip << ':'
                      << received->sender_port << ": " << received->message
                      << '\n';
        }
    }

    return 0;
}