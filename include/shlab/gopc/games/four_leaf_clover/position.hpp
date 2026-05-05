#pragma once

#include <compare>
#include <variant>

namespace shlab::gopc::games::four_leaf_clover {

struct Field {
    int number{};

    auto operator<=>(const Field&) const noexcept = default;
};

struct Stock {
    int number{};

    auto operator<=>(const Stock&) const noexcept = default;
};

struct Removed {
    auto operator<=>(const Removed&) const noexcept = default;
};

using Position = std::variant<Field, Stock, Removed>;

}  // namespace shlab::gopc::games::four_leaf_clover
