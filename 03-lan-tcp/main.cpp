#include "udpSocket.hpp"
#include "beacon.hpp"

#include <iostream>
#include <cstdint>
#include <vector>
#include <poll.h>
#include <optional>
#include <cerrno>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <stdexcept>
#include <string>

int main()
{
    constexpr std::uint16_t discovery_port = 9001;

    // Construct local udpSocket
    UdpSocket udp_socket(discovery_port);

    // Our ID never changes, so the beacon text can be built once
    const std::uint64_t my_id = generate_peer_id();
    const std::string beacon_text = make_beacon(my_id);
    std::cout << "My peer ID: " << std::hex << my_id << std::dec << std::endl;

    // steady_clock only moves forward, unlike the system wall clock, which can be adjusted
    using steady_clock = std::chrono::steady_clock;
    constexpr auto beacon_interval = std::chrono::seconds(1);
    auto next_beacon = steady_clock::now();   // First beacon goes out immediately

    /* struct pollfd {
    *  int   fd;       // you set:  which fd to watch
    *  short events;   // you set:  what you care about (POLLIN = "readable")
    *  short revents;  // kernel sets: what actually happened
    */

    // Dynamic array of pollfd objects so can vary at runtime
    std::vector<pollfd> fds;

    pollfd udp_fd{};
    udp_fd.fd = udp_socket.get_fd();
    udp_fd.events = POLLIN;
    fds.push_back(udp_fd);

    // Main loop
    while (true)
    {
        // Send only when the deadline has passed. Received packets (including our own
        // broadcast coming back) wake poll() early but never trigger a send.
        const auto now = steady_clock::now();
        if (now >= next_beacon)
        {
            udp_socket.send_broadcast(beacon_text);
            next_beacon = now + beacon_interval;
        }

        // Sleep at most until the next beacon is due. The subtraction gives
        // nanoseconds, but poll() takes milliseconds; ns -> ms loses precision,
        // so chrono requires an explicit duration_cast (truncates toward zero).
        // The result can be negative: the check above only guarantees the
        // deadline was in the future at `now`, and it may pass before this
        // second now() call (e.g. it was microseconds away, or we were
        // DESCHEDULED in between). Clamp at 0: a negative timeout would make
        // poll() wait forever.
        const auto wait = std::chrono::duration_cast<std::chrono::milliseconds>(
            next_beacon - steady_clock::now());
        const int timeout_ms = std::max(0, static_cast<int>(wait.count()));

        // Use the poll system call
        int ready = poll(fds.data(), fds.size(), timeout_ms);

        if (ready == -1)
        {
            // If a signal handler is ever added we would check for EINTR errno
            perror("poll");
		    throw std::runtime_error("poll failed");
        }

        // Probe revents with a bitmask (fds[0] is UDP)
        else if (ready > 0 && (fds[0].revents & POLLIN))
        {
            auto received = udp_socket.try_receive();

            if (received == std::nullopt)
            {
                std::cout << "Polled a UDP packet but could not receive" << std::endl;
            }

            else
            {
                // Ignore anything that is not a beacon, and our own beacons
                const auto peer_id = parse_beacon(received->message);
                if (peer_id && *peer_id != my_id)
                {
                    std::cout <<
                    "Beacon from peer " <<
                    std::hex << *peer_id << std::dec <<
                    " at IP " <<
                    received->sender_ip <<
                    std::endl;
                }
            }
        }

        // Otherwise poll returns 0 if the system call timed out - do nothing
    }

    return 0;
}
