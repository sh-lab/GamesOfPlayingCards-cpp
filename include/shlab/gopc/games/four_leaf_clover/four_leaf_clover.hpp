#pragma once

#include <cstddef>
#include <span>

#include "shlab/gopc/games/four_leaf_clover/position.hpp"
#include "shlab/gopc/playing_cards/card.hpp"
#include "shlab/gopc/utility/card_state.hpp"

namespace shlab::gopc::games::four_leaf_clover {

class FourLeafClover {
public:
    using card_type = shlab::gopc::playing_cards::Card;
    using state_type = shlab::gopc::utility::CardState<Position, 52>;

    explicit FourLeafClover(state_type positions);

    [[nodiscard]] static FourLeafClover deal(std::span<const card_type> deck);

    [[nodiscard]] const state_type& card_positions() const noexcept;
    [[nodiscard]] const Position& position_of(const card_type& card) const;
    [[nodiscard]] bool contains(const card_type& card) const noexcept;

    [[nodiscard]] std::size_t stock_count() const noexcept;
    [[nodiscard]] bool is_win() const noexcept;

    [[nodiscard]] bool can_remove(std::span<const card_type> cards) const;
    [[nodiscard]] FourLeafClover remove(std::span<const card_type> cards) const;

    [[nodiscard]] bool operator==(const FourLeafClover&) const = default;

private:
    state_type positions_;
};

}  // namespace shlab::gopc::games::four_leaf_clover
