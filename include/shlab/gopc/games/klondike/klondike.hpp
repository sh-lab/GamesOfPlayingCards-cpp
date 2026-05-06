#pragma once

#include <span>

#include "shlab/gopc/games/klondike/column.hpp"
#include "shlab/gopc/games/klondike/position.hpp"
#include "shlab/gopc/playing_cards/card.hpp"
#include "shlab/gopc/utility/card_state.hpp"

namespace shlab::gopc::games::klondike {

class Klondike {
public:
    using card_type = shlab::gopc::playing_cards::Card;
    using state_type = shlab::gopc::utility::CardState<Position, 52>;

    explicit Klondike(state_type positions);

    [[nodiscard]] static Klondike deal(std::span<const card_type> deck);

    [[nodiscard]] const state_type& card_positions() const noexcept;
    [[nodiscard]] const Position& position_of(const card_type& card) const;
    [[nodiscard]] bool contains(const card_type& card) const noexcept;

    [[nodiscard]] std::size_t stock_count() const noexcept;
    [[nodiscard]] bool is_win() const noexcept;
    [[nodiscard]] bool is_pre_win() const noexcept;

    [[nodiscard]] bool can_open(const card_type& card) const;
    [[nodiscard]] Klondike open(const card_type& card) const;

    [[nodiscard]] bool can_draw(const card_type& card) const;
    [[nodiscard]] Klondike draw(const card_type& card) const;

    [[nodiscard]] bool can_move_to_foundation(const card_type& card) const;
    [[nodiscard]] Klondike move_to_foundation(const card_type& card) const;

    [[nodiscard]] bool can_redeal() const noexcept;
    [[nodiscard]] Klondike redeal() const;

    [[nodiscard]] bool can_move_to_tableau(const card_type& card, Column column) const;
    [[nodiscard]] Klondike move_to_tableau(const card_type& card, Column column) const;

    [[nodiscard]] bool operator==(const Klondike&) const = default;

private:
    state_type positions_;
};

}  // namespace shlab::gopc::games::klondike
