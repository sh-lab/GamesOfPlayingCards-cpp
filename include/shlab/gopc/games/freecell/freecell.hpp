#pragma once

#include <span>

#include "shlab/gopc/games/freecell/column.hpp"
#include "shlab/gopc/games/freecell/position.hpp"
#include "shlab/gopc/playing_cards/card.hpp"
#include "shlab/gopc/utility/card_state.hpp"

namespace shlab::gopc::games::freecell {

class FreeCell {
public:
    using card_type = shlab::gopc::playing_cards::Card;
    using state_type = shlab::gopc::utility::CardState<Position, 52>;

    explicit FreeCell(state_type positions);

    [[nodiscard]] static FreeCell deal(std::span<const card_type> deck);

    [[nodiscard]] const state_type& card_positions() const noexcept;
    [[nodiscard]] const Position& position_of(const card_type& card) const;
    [[nodiscard]] bool contains(const card_type& card) const noexcept;

    [[nodiscard]] bool is_win() const noexcept;

    [[nodiscard]] bool can_move_to_cell(const card_type& card) const;
    [[nodiscard]] FreeCell move_to_cell(const card_type& card) const;

    [[nodiscard]] bool can_move_to_foundation(const card_type& card) const;
    [[nodiscard]] FreeCell move_to_foundation(const card_type& card) const;

    [[nodiscard]] bool can_move_to_tableau(const card_type& card, Column column) const;
    [[nodiscard]] FreeCell move_to_tableau(const card_type& card, Column column) const;

    [[nodiscard]] bool operator==(const FreeCell&) const = default;

private:
    state_type positions_;
};

}  // namespace shlab::gopc::games::freecell
