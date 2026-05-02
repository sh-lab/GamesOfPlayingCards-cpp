#pragma once

#include <compare>
#include <variant>

#include "shlab/gopc/games/calculation/foundation_column.hpp"
#include "shlab/gopc/games/calculation/tableau_column.hpp"

namespace shlab::gopc::games::calculation {

struct Foundation {
    FoundationColumn column{};

    auto operator<=>(const Foundation&) const noexcept = default;
};

struct Stock {
    int number{};

    auto operator<=>(const Stock&) const noexcept = default;
};

struct WastePile {
    auto operator<=>(const WastePile&) const noexcept = default;
};

struct Tableau {
    TableauColumn column{};
    int number{};

    auto operator<=>(const Tableau&) const noexcept = default;
};

using Position = std::variant<Foundation, Stock, WastePile, Tableau>;

}  // namespace shlab::gopc::games::calculation
