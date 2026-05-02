#pragma once

#include <compare>
#include <variant>

#include "shlab/gopc/games/freecell/column.hpp"

namespace shlab::gopc::games::freecell {

struct Foundation {
    auto operator<=>(const Foundation&) const noexcept = default;
};

struct Cell {
    int number{};

    auto operator<=>(const Cell&) const noexcept = default;
};

struct Tableau {
    Column column{};
    int number{};

    auto operator<=>(const Tableau&) const noexcept = default;
};

using Position = std::variant<Cell, Foundation, Tableau>;

}  // namespace shlab::gopc::games::freecell
