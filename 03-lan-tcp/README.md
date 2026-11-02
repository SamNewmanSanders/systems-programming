# 03 - LAN Peer-to-Peer Messaging

A rewrite of the earlier attempt (kept in `03-lan-tcp-old` for reference). Each laptop running the program finds the others on the LAN with UDP broadcast, then connects to them over TCP to exchange messages. Linux only for now.

## Design

- **One thread, one event loop.** Instead of a thread per socket, a single `poll()` call waits on every file descriptor at once (the UDP socket now; the TCP listener, connections and stdin later). `poll()` reports which descriptors are ready, and only those are read.
- **Discovery never stops.** Every instance broadcasts a beacon containing its random 64-bit peer ID once per second, forever, and keeps a table of peers it has heard from. A lost beacon does not matter because the next one replaces it, and late joiners are found automatically.
- **Full mesh, no fixed roles.** Every peer listens for TCP connections *and* dials out. For each pair of peers, the lower ID dials the higher ID, so each pair connects exactly once. Client and server describe a single connection, not a laptop; once connected, both sides are equal.

## What I am learning

- **Blocking vs non-blocking sockets:** by default, `recvfrom()` sleeps until a datagram arrives. A socket created with `SOCK_NONBLOCK` returns immediately instead, with `-1` and `errno` set to `EAGAIN` (or `EWOULDBLOCK`, the same value on Linux) when nothing is waiting. `SOCK_DGRAM | SOCK_NONBLOCK` is a bitwise OR: `SOCK_DGRAM` picks the socket type (UDP) and `SOCK_NONBLOCK` changes how operations behave. The same mode can be set after creation with `fcntl(fd, F_SETFL, O_NONBLOCK)`.
- **`poll()` readiness is not a guarantee:** `poll()` reporting `POLLIN` means data was available when it checked, not that the next `recvfrom()` will succeed. For example, Linux can report a UDP socket readable and then discard the datagram for a bad checksum when it is actually read (see the BUGS section of `man 2 select`). With a blocking socket that `recvfrom()` would sleep and freeze the whole event loop, so descriptors in an event loop are always non-blocking: `poll()` decides when to wait, and receives never wait.
- **Level-triggered `poll()`:** `poll()` keeps reporting a descriptor as readable while data remains, so reading one datagram per wakeup is correct; any leftover data makes the next `poll()` return immediately. Draining every waiting datagram in a loop is an optimisation (and is required with edge-triggered `epoll`).

## TODO

### Milestone 1: discovery on an event loop
- [ ] `UdpSocket`: create with `SOCK_NONBLOCK`, set `SO_REUSEADDR` before `bind()`, add an `fd()` accessor for `poll()`
- [ ] `UdpSocket`: `try_receive()` returns `std::nullopt` on `EAGAIN`/`EWOULDBLOCK` rather than treating it as an error
- [ ] `UdpSocket`: delete the copy constructor and copy assignment so two objects never close the same descriptor
- [ ] Beacon: `makeBeacon(id)` and `parseBeacon(message)`, parsing with `std::from_chars` (no exceptions on bad input)
- [ ] Event loop: `poll()` on the UDP socket with a timeout until the next beacon, clamped to zero or more (a negative timeout waits forever)
- [ ] Peer table: print each new peer once, ignore our own beacons
- [ ] Test: two instances on one laptop, then two laptops; check CPU stays near 0% while idle
- [ ] Optional: drop peers not heard from for 5 seconds

### Milestone 2: TCP mesh
- [ ] Non-blocking TCP listener added to the `poll()` set
- [ ] Lower ID dials higher ID on discovering a new peer; send a HELLO with our ID first
- [ ] Track connections; remove them when `recv()` returns 0

### Milestone 3: messaging
- [ ] Add stdin to the `poll()` set and send typed lines to every connection
- [ ] Message framing over the TCP byte stream (newline delimiter or length prefix)

### Later
- [ ] Non-blocking `connect()` (`EINPROGRESS`, wait for `POLLOUT`, check `SO_ERROR`)
- [ ] Binary beacon format with explicit byte order
- [ ] Delete `03-lan-tcp-old` once this version works

## Build and run

`build.sh` configures the first time, then builds. It works from any directory:

```sh
03-lan-tcp/build.sh        # build
03-lan-tcp/build.sh run    # build, then run
```

The equivalent manual commands, from this directory:

```sh
cmake -S . -B build        # configure: first time only
cmake --build build        # build: after every change
./build/main
```

Build separately on each laptop: a binary built against Arch's newer C/C++ libraries will usually not run on an older Ubuntu.
