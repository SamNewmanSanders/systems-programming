# 03 - LAN Peer Discovery

This project extends the UDP socket experiment from `02-udp-socket` from one machine to a local network. Each running instance periodically broadcasts a discovery message and listens for messages from other instances.

The goal is to let two laptops find each other when the program starts, without the user already knowing or entering the other laptop's IP address. Hard-coding or manually configuring addresses is inconvenient when addresses can change, while ordinary DNS lookup assumes that a name and DNS record have already been configured. For devices running this program on the same local network, UDP broadcast offers a simple way to announce their presence and learn each sender's IP address automatically. This is a small application-level discovery protocol, not a replacement for DNS: it is intended for peers on a reachable LAN and generally does not cross routers.

## What the experiment does

Each process creates a random 64-bit peer ID and broadcasts a message containing that ID to `255.255.255.255` on UDP port `9001`, immediately and then once per second. When it receives a datagram, it prints the sender's IP address, port, and message.

This is a peer-to-peer discovery stage rather than a client-server design: every instance can both announce itself and listen for others. The random IDs make accidental collisions unlikely, but they are not guaranteed unique or intended as secure identities. The current program learns a sender's IP address from the received UDP datagram; it does not establish a TCP connection yet.

The experiment appeared to work between:

- Windows: `10.163.198.251`
- Linux: `10.163.198.98`

These addresses share a prefix, but the subnet mask determines whether they are on the same subnet. A limited broadcast is normally confined to its local network segment; this test does not show that it reaches the whole campus network.

## TCP concepts

TCP is connection-oriented: the two endpoints establish a connection before exchanging application data. Once connected, TCP presents the application with a reliable, ordered byte stream, not a sequence of messages. A call to `send()` on one endpoint does not correspond to a matching call to `recv()` on the other. Data may be combined or split along the way, and `send()` or `recv()` may process fewer bytes than requested. The application must define its own message framing, for example by sending a length before each file or message, and loop as needed to send or receive the full amount.

TCP handles much of the delivery machinery behind that stream. It numbers bytes, acknowledges received data, retransmits data that appears to have been lost, and reorders data before delivering it to the application. It also uses flow control to avoid overwhelming the receiver and congestion control to adapt its sending rate to network conditions. These mechanisms are normally handled by the operating system's TCP implementation; application code uses the socket API, but still needs to handle connection errors, partial reads and writes, and its own message format.

## What I am learning

- **OS-specific code behind one interface:** `udpSocket.hpp` declares the `UdpSocket` API used by `main.cpp`. CMake selects either `udpSocketLinux.cpp` or `udpSocketWindows.cpp`, since each implements the same class using a different operating-system socket API.
- **Linux implementation:** I understand the Linux implementation, including its socket calls and error paths. The Windows implementation was AI-generated and has not been studied or verified to the same level.
- **Polling instead of waiting indefinitely:** Linux `poll()` waits for socket readiness up to a timeout. The program can therefore wait for an incoming datagram while still waking in time to send its next periodic broadcast. `poll()` reports readiness; `recvfrom()` then receives the datagram. This avoids calling a blocking receive and getting stuck there instead of continuing the loop. The Windows implementation provides the same behavior.
- **Readiness flags:** `POLLIN` asks for readable data, and `revents` reports which events actually occurred. A readiness notification is not the packet itself; the receive call obtains the data.
- **Addresses and byte order:** `sockaddr_in` holds an IPv4 address and port. Helpers such as `htons()` and `ntohs()` convert port values between host and network byte order. Binding to `INADDR_ANY` lets the socket receive on its available local interfaces.
- **What `bind()` does:** I initially thought `bind()` simply made a socket capable of receiving, but it assigns the socket a local address and port (and can restrict it to a particular local interface). A server normally binds to a known port so clients know where to send their first packet or connection request. A client usually does not need to choose its own port: the OS can assign an available ephemeral port when it sends or connects, and the remote endpoint can send replies back to that source endpoint. This discovery socket binds to UDP port `9001` because every instance needs to receive broadcasts sent to that known discovery port. The UDP source port is separate from any TCP listening port the program may advertise.
- **System-call errors:** Socket creation, configuration, binding, sending, waiting, and receiving are checked, with failures reported or raised as errors. The Linux implementation uses `perror()` for several failures so the OS can provide the associated error text.

## Next steps

Before acting on a received broadcast, validate that its message matches the discovery format and parse its peer ID. At present, the program prints any datagram delivered to the socket, including unrelated traffic on that port. A peer ID is useful for distinguishing peers, but it does not authenticate them.

Once discovery is validated, peers could use the sender's IP address to attempt a separate TCP connection. TCP provides a reliable, ordered byte stream, but it is not encrypted or authenticated by itself; a secure connection would also need TLS and an authentication policy. In a peer-to-peer design, one peer would need to listen while another initiates each connection, even though discovery itself is symmetric.

## Build and run

Build on Linux from this directory:

```sh
cmake -S . -B build
cmake --build build
./build/main
```

Build and run on Windows with CMake and Visual Studio Build Tools:

```powershell
cmake -S . -B build-windows
cmake --build build-windows --config Debug
.\build-windows\Debug\main.exe
```

The program broadcasts once per second until stopped with Ctrl+C. Run it only on a network where you are permitted to send broadcast traffic.