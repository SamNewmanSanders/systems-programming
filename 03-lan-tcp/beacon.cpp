#include "beacon.hpp"

#include <charconv>
#include <cstdio>
#include <random>
#include <system_error>


// Anything inside an anonymous namespace can only be accessed from 
// within that particular .cpp file.
namespace
{
const std::string beacon_prefix = "SAMNS_PEER ";
constexpr int id_digits = 16;
}

std::uint64_t generate_peer_id()
{
	// random_device provides an unpredictable seed; mt19937_64 uses it to
    // generate one pseudorandom 64-bit value. The generator is destroyed
    // when the function returns, so its state is not reused between calls.
    // Obviously fine here
	std::random_device random_device;
	std::mt19937_64 generator(random_device());
	return generator();
}

// Build the discovery beacon text for this peer, e.g. "SAMNS_PEER 3fa85f6457174562".
// AI was used to generate this function entirely.
std::string make_beacon(std::uint64_t peer_id)
{
	// %016llx: lowercase hex, padded with leading zeros to exactly 16 digits
	char id_text[id_digits + 1];
	std::snprintf(id_text, sizeof(id_text), "%016llx",
		static_cast<unsigned long long>(peer_id));

	return beacon_prefix + id_text;
}

// AI was also used to generate the parse_beacon function entirely
// Inverse of make_beacon: returns the peer ID if the message is a well-formed
// beacon ("SAMNS_PEER " + 16 hex digits), or nullopt for anything else.
std::optional<std::uint64_t> parse_beacon(const std::string& message)
{
	// A valid beacon is exactly the prefix followed by 16 digits
	if (message.size() != beacon_prefix.size() + id_digits ||
		message.compare(0, beacon_prefix.size(), beacon_prefix) != 0)
	{
		return std::nullopt;
	}

	const char* id_begin = message.data() + beacon_prefix.size();
	const char* id_end = id_begin + id_digits;

	// from_chars reports failure through its result instead of throwing (unlike stoull),
	// and says where parsing stopped, so a short or non-hex ID is rejected
	std::uint64_t peer_id = 0;
	const auto [parsed_end, error] = std::from_chars(id_begin, id_end, peer_id, 16);
	if (error != std::errc{} || parsed_end != id_end)
	{
		return std::nullopt;
	}

	return peer_id;
}
