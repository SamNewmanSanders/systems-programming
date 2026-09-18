#include <string>
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

    const std::uint16_t discovery_port = 9001;
    const std::uint64_t peer_id = generate_peer_id();

    UdpSocket socket(discovery_port);

    while (!peer_found)
    {
        socket.send_broadcast(
            "LAN_PEER_DISCOVERY " + std::to_string(peer_id)
        );

        auto received = socket.receive();

        if (received)
        {
            // inspect the other peer
        }

        sleep briefly;
    }


    return 0;
}