#pragma once

#include <optional>
#include <span>

#include "shlab/gopc/games/calculation/foundation_column.hpp"
#include "shlab/gopc/games/calculation/position.hpp"
#include "shlab/gopc/games/calculation/tableau_column.hpp"
#include "shlab/gopc/playing_cards/card.hpp"
#include "shlab/gopc/playing_cards/rank.hpp"
#include "shlab/gopc/utility/card_state.hpp"

namespace shlab::gopc::games::calculation {

class Calculation {
public:
    using card_type = shlab::gopc::playing_cards::Card;
    using rank_type = shlab::gopc::playing_cards::Rank;
    using state_type = shlab::gopc::utility::CardState<Position, 52>;

    explicit Calculation(state_type positions);

    [[nodiscard]] static Calculation deal(std::span<const card_type> deck);

    [[nodiscard]] const state_type& card_positions() const noexcept;
    [[nodiscard]] const Position& position_of(const card_type& card) const;
    [[nodiscard]] bool contains(const card_type& card) const noexcept;

    [[nodiscard]] std::optional<rank_type> next_rank(FoundationColumn column) const noexcept;
    [[nodiscard]] std::size_t stock_count() const noexcept;
    [[nodiscard]] bool is_win() const noexcept;

    [[nodiscard]] bool can_draw(const card_type& card) const;
    [[nodiscard]] Calculation draw(const card_type& card) const;

    [[nodiscard]] bool can_move_to_tableau(const card_type& card) const;
    [[nodiscard]] Calculation move_to_tableau(const card_type& card, TableauColumn column) const;

    [[nodiscard]] bool can_move_to_foundation(const card_type& card, FoundationColumn column) const;
    [[nodiscard]] Calculation move_to_foundation(const card_type& card, FoundationColumn column) const;

    [[nodiscard]] bool operator==(const Calculation&) const = default;

private:
    state_type positions_;
};

}  // namespace shlab::gopc::games::calculation
