#pragma once

#include <span>

#include "shlab/gopc/games/yukon/column.hpp"
#include "shlab/gopc/games/yukon/position.hpp"
#include "shlab/gopc/playing_cards/card.hpp"
#include "shlab/gopc/utility/card_state.hpp"

namespace shlab::gopc::games::yukon {

class Yukon {
public:
    using card_type = shlab::gopc::playing_cards::Card;
    using state_type = shlab::gopc::utility::CardState<Position, 52>;

    explicit Yukon(state_type positions);

    [[nodiscard]] static Yukon deal(std::span<const card_type> deck);

    [[nodiscard]] const state_type& card_positions() const noexcept;
    [[nodiscard]] const Position& position_of(const card_type& card) const;
    [[nodiscard]] bool contains(const card_type& card) const noexcept;

    [[nodiscard]] bool is_win() const noexcept;
    [[nodiscard]] bool is_moved_foundation_no_problem(const card_type& card) const;

    [[nodiscard]] bool can_open(const card_type& card) const;
    [[nodiscard]] Yukon open(const card_type& card) const;

    [[nodiscard]] bool can_move_to_foundation(const card_type& card) const;
    [[nodiscard]] Yukon move_to_foundation(const card_type& card) const;

    [[nodiscard]] bool can_move_to_tableau(const card_type& card, Column column) const;
    [[nodiscard]] Yukon move_to_tableau(const card_type& card, Column column) const;

    [[nodiscard]] bool operator==(const Yukon&) const = default;

private:
    state_type positions_;
};

}  // namespace shlab::gopc::games::yukon
