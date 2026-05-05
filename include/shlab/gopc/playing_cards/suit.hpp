#pragma once

#include <cstdint>

namespace shlab::gopc::playing_cards {

enum class Suit : std::uint8_t {
    None = 0x00,
    Spades = 0x10,
    Hearts = 0x20,
    Diamonds = 0x30,
    Clubs = 0x40,
};

}  // namespace shlab::gopc::playing_cards
