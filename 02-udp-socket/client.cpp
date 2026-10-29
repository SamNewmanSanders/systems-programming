#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <string>

int main()
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock < 0) {
        perror("socket");
        return 1;
    }

    std::cout << "Socket created: " << sock << std::endl;

    // Describe where to send the datagram. sockaddr_in is the IPv4 address struct
    // ("sin_" = sockaddr_in; old C needed unique field prefixes). {} zeroes every
    // field, including the sin_zero padding that must be zero.
    sockaddr_in server{};
    // Tell the kernel which address type this is, since sendto() only sees a generic sockaddr*
    server.sin_family = AF_INET;
    // The network expects big-endian ("network byte order"); htons swaps the bytes on
    // little-endian CPUs. Without it, 9000 (0x2328) would go out as port 10275 (0x2823)
    server.sin_port = htons(9000);
    // Convert the text "127.0.0.1" into the 4-byte binary form, already in network
    // byte order. The manual equivalent is: server.sin_addr.s_addr = htonl(0x7F000001);
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

    const char* message = "hello";

    // sendto() is a C API that must accept IPv4, IPv6 and Unix addresses, so it takes
    // a generic sockaddr* plus the struct's size. C has no templates or overloading,
    // so we cast our sockaddr_in* to sockaddr*; the kernel reads sin_family to know
    // the real type and uses the size to know how many bytes are valid.
    sendto(
        sock,
        message,
        strlen(message),
        0,
        reinterpret_cast<sockaddr*>(&server),
        sizeof(server)
    );

    char buffer[1024];

    ssize_t bytes = recvfrom(
        sock,
        buffer,
        sizeof(buffer),
        0,
        nullptr,
        nullptr
    );

    if (bytes < 0) {
        perror("recvfrom");
        return 1;
    }

    std::cout << "Received: "
              << std::string(buffer, bytes) << '\n';

    close(sock);
}

// A lot of the oddness in this file is historical baggage from the BSD sockets API
// (early 1980s, written in C), kept so old code still compiles:
//   - reinterpret_cast<sockaddr*>: the API predates standard C's void* (1989), so it
//     invented struct sockaddr as its own "generic address" type
//   - htons() / network byte order in sockaddr_in: the struct stores values exactly as
//     they appear in the packet header, so the kernel can copy them without converting
//   - sin_ prefixes (sin_family, sin_port...): early C compilers shared one namespace for
//     all struct fields, so every struct prefixed its own
//   - sin_addr.s_addr instead of a plain uint32_t: in_addr used to be a union for the old
//     class A/B/C address scheme; the struct wrapper stayed after the union was removed
//   - sin_zero padding: makes sockaddr_in the same size as the generic sockaddr
//   - AF_INET passed to both socket() and sockaddr_in: the original design kept protocol
//     families (PF_) and address families (AF_) separate, but they never diverged
// Modern code usually hides most of this behind getaddrinfo().