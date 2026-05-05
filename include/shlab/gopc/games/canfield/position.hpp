#pragma once

#include <compare>
#include <variant>

#include "shlab/gopc/games/canfield/column.hpp"

namespace shlab::gopc::games::canfield {

struct Foundation {
    auto operator<=>(const Foundation&) const noexcept = default;
};

struct Stock {
    int number{};

    auto operator<=>(const Stock&) const noexcept = default;
};

struct WastePile {
    int number{};

    auto operator<=>(const WastePile&) const noexcept = default;
};

struct Reserve {
    int number{};

    auto operator<=>(const Reserve&) const noexcept = default;
};

struct Tableau {
    Column column{};
    int number{};

    auto operator<=>(const Tableau&) const noexcept = default;
};

using Position = std::variant<Foundation, Stock, WastePile, Reserve, Tableau>;

}  // namespace shlab::gopc::games::canfield
