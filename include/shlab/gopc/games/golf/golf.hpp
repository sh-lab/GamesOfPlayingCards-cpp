#pragma once

#include <span>
#include <unordered_map>

#include "shlab/gopc/games/golf/lane.hpp"
#include "shlab/gopc/games/golf/position.hpp"
#include "shlab/gopc/playing_cards/card.hpp"

namespace shlab::gopc::games::golf {

class Golf {
public:
    using card_type = shlab::gopc::playing_cards::Card;
    using state_type = std::unordered_map<card_type, Position>;

    explicit Golf(state_type positions, bool can_loop);

    [[nodiscard]] static Golf deal(std::span<const card_type> deck, bool can_loop);

    [[nodiscard]] const state_type& card_positions() const noexcept;
    [[nodiscard]] const Position& position_of(const card_type& card) const;
    [[nodiscard]] bool contains(const card_type& card) const noexcept;
    [[nodiscard]] bool can_loop() const noexcept;

    [[nodiscard]] std::size_t deck_count() const noexcept;
    [[nodiscard]] bool is_win() const noexcept;
    [[nodiscard]] bool is_lose() const;

    [[nodiscard]] bool can_move_to_hand(const card_type& card) const;
    [[nodiscard]] Golf move_to_hand(const card_type& card) const;

    [[nodiscard]] bool operator==(const Golf&) const = default;

private:
    state_type positions_;
    bool can_loop_{};
};

}  // namespace shlab::gopc::games::golf
