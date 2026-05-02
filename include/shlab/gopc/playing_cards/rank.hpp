#pragma once

#include <cstdint>

namespace shlab::gopc::playing_cards {

enum class Rank : std::uint8_t {
    None = 0x00,
    Ace = 0x01,
    Two = 0x02,
    Three = 0x03,
    Four = 0x04,
    Five = 0x05,
    Six = 0x06,
    Seven = 0x07,
    Eight = 0x08,
    Nine = 0x09,
    Ten = 0x0A,
    Jack = 0x0B,
    Queen = 0x0C,
    King = 0x0D,
};

}  // namespace shlab::gopc::playing_cards
