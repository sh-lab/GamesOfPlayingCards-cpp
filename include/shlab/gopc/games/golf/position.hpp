#pragma once

#include <compare>
#include <variant>

#include "shlab/gopc/games/golf/lane.hpp"

namespace shlab::gopc::games::golf {

struct Hand {
    int number{};

    auto operator<=>(const Hand&) const noexcept = default;
};

struct Deck {
    int number{};

    auto operator<=>(const Deck&) const noexcept = default;
};

struct Field {
    Lane lane{};
    int number{};

    auto operator<=>(const Field&) const noexcept = default;
};

using Position = std::variant<Hand, Deck, Field>;

}  // namespace shlab::gopc::games::golf
