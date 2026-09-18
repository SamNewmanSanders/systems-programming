#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>

int main()
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock < 0) {
        perror("socket");
        return 1;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(9000);
    address.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, reinterpret_cast<sockaddr*>(&address),
             sizeof(address)) < 0) {
        perror("bind");
        return 1;
    }

    std::cout << "Listening on UDP port 9000...\n";

    char buffer[1024];

    sockaddr_in client{};
    socklen_t client_len = sizeof(client);

    ssize_t bytes = recvfrom(
        sock,
        buffer,
        sizeof(buffer),
        0,
        reinterpret_cast<sockaddr*>(&client),
        &client_len
    );

    if (bytes < 0) {
        perror("recvfrom");
        return 1;
    }

    std::cout << "Received " << bytes << " bytes: "
              << std::string(buffer, bytes) << '\n';

    sendto(
        sock,
        buffer,
        bytes,
        0,
        reinterpret_cast<sockaddr*>(&client),
        client_len
    );

    close(sock);
}