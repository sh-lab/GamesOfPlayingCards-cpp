#pragma once

#include <compare>
#include <variant>

#include "shlab/gopc/games/yukon/column.hpp"

namespace shlab::gopc::games::yukon {

struct Foundation {
    auto operator<=>(const Foundation&) const noexcept = default;
};

struct Tableau {
    Column column{};
    int number{};
    bool open{};

    auto operator<=>(const Tableau&) const noexcept = default;
};

using Position = std::variant<Foundation, Tableau>;

}  // namespace shlab::gopc::games::yukon
