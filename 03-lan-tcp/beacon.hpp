#pragma once

#include <cstdint>
#include <optional>
#include <string>

// Discovery beacon, sent as text: "SAMNS_PEER <id>"
//   SAMNS_PEER  marks the datagram as ours, so other traffic on the port is ignored
//   <id>        peer ID as exactly 16 lowercase hex digits; fixed width makes it
//               easy to parse, and it compares as a plain number once parsed
// Example: "SAMNS_PEER 3fa85f6457174562"

// Random 64-bit ID, generated once at startup. Collisions are possible but very unlikely.
std::uint64_t generate_peer_id();

std::string make_beacon(std::uint64_t peer_id);

// Returns the sender's peer ID, or std::nullopt for anything that is not a well-formed
// beacon. Never throws, because any device on the LAN can send arbitrary bytes to the
// discovery port.
std::optional<std::uint64_t> parse_beacon(const std::string& message);
