#pragma once

#include <compare>
#include <variant>

#include "shlab/gopc/games/klondike/column.hpp"

namespace shlab::gopc::games::klondike {

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

struct Tableau {
    Column column{};
    int number{};
    bool open{};

    auto operator<=>(const Tableau&) const noexcept = default;
};

using Position = std::variant<Foundation, Stock, WastePile, Tableau>;

}  // namespace shlab::gopc::games::klondike
