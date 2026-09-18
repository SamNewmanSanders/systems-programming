#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>

int main()
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    std::cout << "Socket created: " << sock << std::endl;

    if (sock < 0) {
        perror("socket");
        return 1;
    }

    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(9000);
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

    const char* message = "hello";

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