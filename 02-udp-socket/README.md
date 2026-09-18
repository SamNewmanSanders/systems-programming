# 02 - Linux Networking Intro

First networking project: a UDP client and server running on the same machine over the loopback address `127.0.0.1`.

The point of this project is to learn the basics: sockets, addresses, ports, buffers, and how a program uses the operating system to send and receive packets.

## What we learned

### 1. A socket is a software object in the OS

A socket is not magic. It is a kernel-managed software object that represents a communication endpoint.

When we call:

```cpp
int sock = socket(AF_INET, SOCK_DGRAM, 0);
```

we are asking the operating system to create a socket for us. The OS gives us back a small integer value, called a file descriptor.

This is important: a socket is handled like a file descriptor because the OS exposes it through the same general I/O model as files. We can read from it, write to it, and close it.

That is why this works:

```cpp
sendto(...)
recvfrom(...)
close(sock);
```

The OS knows that this integer is a socket, not a normal file.

### 2. Why a socket is a file descriptor

A file descriptor is just an integer index into a process table of open resources managed by the kernel.

For regular files, the descriptor points to a file object. For sockets, it points to a socket object. The kernel keeps track of:

- the socket type (UDP/TCP)
- local and remote addresses
- queueing of incoming data
- protocol state
- associated buffers and metadata

This is why the program does not directly manipulate network packets. The kernel does that for us. The application only interacts with the socket abstraction.

### 3. Why we need a buffer

We receive data into a memory buffer because packets are not always delivered as a final, nicely formatted string object. They are just bytes arriving from the network.

In the client:

```cpp
char buffer[1024];
ssize_t bytes = recvfrom(sock, buffer, sizeof(buffer), 0, nullptr, nullptr);
```

the kernel writes incoming data into the memory area named `buffer`.

The buffer is necessary because:

- the network delivers bytes, not C++ strings
- the program needs somewhere to store them temporarily
- we may not know how much data will arrive ahead of time
- the receiver must decide how many bytes were actually received

This is one reason `recvfrom` returns a byte count. The program receives that count and can decide how to interpret the bytes.

### 4. UDP can be variable length

UDP is a datagram-oriented protocol. Each UDP packet is a self-contained datagram. That means the payload length can vary from one packet to another.

In our code, we allocate a fixed-size buffer:

```cpp
char buffer[1024];
```

but the actual incoming packet can be shorter or longer than that. If the packet is longer than the buffer, the kernel will truncate it, and the byte count tells us how many bytes actually arrived.

This is a key idea:

- a buffer is a temporary storage area
- the socket API gives us the actual number of bytes received
- the application is responsible for handling the data size correctly

So the program does this:

```cpp
std::string(buffer, bytes)
```

which creates a C++ string from exactly the bytes that arrived, not from the entire buffer.

### 5. We fill `sockaddr_in` with the IP and port

The `sockaddr_in` structure is the IPv4 socket address structure.

```cpp
sockaddr_in server{};
server.sin_family = AF_INET;
server.sin_port = htons(9000);
inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);
```

This tells the OS:

- use IPv4 (`AF_INET`)
- send to port 9000
- send to the host `127.0.0.1`

This object is not just a random container. It is the address the OS needs to know where to send traffic.

From the application perspective:

- `sin_addr` = network-layer IP address
- `sin_port` = transport-layer port number
- `sin_family` = address family

The OS combines these to build the actual packet headers.

This is the practical boundary between the programmer and the operating system: the programmer supplies the endpoint, and the OS builds the actual network packet.

### 6. The programmer does not build the packet by hand

The code does not manually create an Ethernet frame or an IP packet. It does not write raw UDP headers.

Instead, the program calls functions like:

```cpp
socket()
sendto()
recvfrom()
bind()
close()
```

These are system calls. The operating system handles the network protocol machinery underneath.

The kernel does things like:

- decide the route
- choose the source address/interface
- attach the IP header
- attach the UDP/TCP header
- compute checksums
- queue incoming packets
- deliver the payload to the correct process

This is an important lesson: the application speaks to the OS through a well-defined API, and the OS handles the low-level networking details.

### 7. Why the code looks low-level

This project uses system APIs directly, so the C++ code looks more low-level than normal app code.

Examples include `sockaddr_in`, `reinterpret_cast`, `nullptr`, `ssize_t`, `htons()`, and `inet_pton()`. These are the normal pieces of the socket API that let C++ talk to the OS.

The main thing I still need to learn is how all of this fits together: the exact API contracts, the C++ syntax around them, and how the OS handles sockets underneath.

### 8. What the libraries do

The headers in this project are mostly giving declarations and helper functions:

- `sys/socket.h` for sockets
- `arpa/inet.h` for IP/address helpers like `htons` and `inet_pton`
- `unistd.h` for `close`
- `cstring` for `strlen`
- `iostream` for `std::cout`

They are the bridge between the C++ code and the OS.

### 9. Next steps

This project gives the basic mental model for the next stage:

- sockets are OS-managed communication objects
- addresses include both IP and port information
- data moves through buffers
- the OS handles the low-level packet work behind the scenes

Next I want to learn more about Linux system calls, kernel networking, and then TCP, which is the next big step after UDP.

## Summary

This project introduced the key ideas I needed:

- sockets are OS-created communication objects
- they behave like file descriptors from the program’s point of view
- data moves through buffers
- UDP packets can vary in size
- `sockaddr_in` gives the OS the destination IP and port
- the OS does the actual packet construction and delivery
