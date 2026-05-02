#pragma once

#include <compare>
#include <variant>

#include "shlab/gopc/games/beleaguered_castle/column.hpp"

namespace shlab::gopc::games::beleaguered_castle {

struct Foundation {
    auto operator<=>(const Foundation&) const noexcept = default;
};

struct Tableau {
    Column column{};
    int number{};

    auto operator<=>(const Tableau&) const noexcept = default;
};

using Position = std::variant<Foundation, Tableau>;

}  // namespace shlab::gopc::games::beleaguered_castle
