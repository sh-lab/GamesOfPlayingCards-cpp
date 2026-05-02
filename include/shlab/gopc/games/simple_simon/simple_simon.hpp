#pragma once

#include <span>
#include <unordered_map>

#include "shlab/gopc/games/simple_simon/column.hpp"
#include "shlab/gopc/games/simple_simon/position.hpp"
#include "shlab/gopc/playing_cards/card.hpp"

namespace shlab::gopc::games::simple_simon {

class SimpleSimon {
public:
    using card_type = shlab::gopc::playing_cards::Card;
    using state_type = std::unordered_map<card_type, Position>;

    explicit SimpleSimon(state_type positions);

    [[nodiscard]] static SimpleSimon deal(std::span<const card_type> deck);

    [[nodiscard]] const state_type& card_positions() const noexcept;
    [[nodiscard]] const Position& position_of(const card_type& card) const;
    [[nodiscard]] bool contains(const card_type& card) const noexcept;

    [[nodiscard]] bool is_win() const noexcept;

    [[nodiscard]] bool can_move_to_foundation(const card_type& card) const;
    [[nodiscard]] SimpleSimon move_to_foundation(const card_type& card) const;

    [[nodiscard]] bool can_move_to_tableau(const card_type& card, Column column) const;
    [[nodiscard]] SimpleSimon move_to_tableau(const card_type& card, Column column) const;

    [[nodiscard]] bool operator==(const SimpleSimon&) const = default;

private:
    state_type positions_;
};

}  // namespace shlab::gopc::games::simple_simon
