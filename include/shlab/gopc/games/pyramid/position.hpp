#pragma once

#include <compare>
#include <variant>

namespace shlab::gopc::games::pyramid {

struct Deck {
    int number{};

    auto operator<=>(const Deck&) const noexcept = default;
};

struct Discard {
    int number{};

    auto operator<=>(const Discard&) const noexcept = default;
};

struct Field {
    int row{};
    int number{};

    auto operator<=>(const Field&) const noexcept = default;
};

struct Hand {
    auto operator<=>(const Hand&) const noexcept = default;
};

struct FreeSpace {
    int number{};

    auto operator<=>(const FreeSpace&) const noexcept = default;
};

struct Outside {
    auto operator<=>(const Outside&) const noexcept = default;
};

using Position = std::variant<Deck, Discard, Field, Hand, FreeSpace, Outside>;

}  // namespace shlab::gopc::games::pyramid
