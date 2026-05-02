#pragma once

#include <span>
#include <unordered_map>

#include "shlab/gopc/games/canfield/column.hpp"
#include "shlab/gopc/games/canfield/position.hpp"
#include "shlab/gopc/playing_cards/card.hpp"
#include "shlab/gopc/playing_cards/rank.hpp"

namespace shlab::gopc::games::canfield {

class Canfield {
public:
    using card_type = shlab::gopc::playing_cards::Card;
    using rank_type = shlab::gopc::playing_cards::Rank;
    using state_type = std::unordered_map<card_type, Position>;

    Canfield(state_type positions, rank_type foundation_base_rank);

    [[nodiscard]] static Canfield deal(std::span<const card_type> deck);

    [[nodiscard]] const state_type& card_positions() const noexcept;
    [[nodiscard]] const Position& position_of(const card_type& card) const;
    [[nodiscard]] bool contains(const card_type& card) const noexcept;

    [[nodiscard]] rank_type foundation_base_rank() const noexcept;
    [[nodiscard]] std::size_t stock_count() const noexcept;
    [[nodiscard]] std::size_t waste_count() const noexcept;
    [[nodiscard]] std::size_t reserve_count() const noexcept;
    [[nodiscard]] bool is_win() const noexcept;

    [[nodiscard]] bool can_draw() const noexcept;
    [[nodiscard]] Canfield draw() const;

    [[nodiscard]] bool can_redeal() const noexcept;
    [[nodiscard]] Canfield redeal() const;

    [[nodiscard]] bool can_move_to_foundation(const card_type& card) const;
    [[nodiscard]] Canfield move_to_foundation(const card_type& card) const;

    [[nodiscard]] bool can_move_to_tableau(const card_type& card, Column column) const;
    [[nodiscard]] Canfield move_to_tableau(const card_type& card, Column column) const;

    [[nodiscard]] bool operator==(const Canfield&) const = default;

private:
    state_type positions_;
    rank_type foundation_base_rank_{};
};

}  // namespace shlab::gopc::games::canfield
