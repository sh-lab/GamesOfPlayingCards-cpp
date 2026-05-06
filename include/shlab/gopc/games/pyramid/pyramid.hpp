#pragma once

#include <span>

#include "shlab/gopc/games/pyramid/position.hpp"
#include "shlab/gopc/playing_cards/card.hpp"
#include "shlab/gopc/utility/card_state.hpp"

namespace shlab::gopc::games::pyramid {

class Pyramid {
public:
    using card_type = shlab::gopc::playing_cards::Card;
    using state_type = shlab::gopc::utility::CardState<Position, 54>;

    explicit Pyramid(state_type positions);

    [[nodiscard]] static Pyramid deal(std::span<const card_type> deck);

    [[nodiscard]] const state_type& card_positions() const noexcept;
    [[nodiscard]] const Position& position_of(const card_type& card) const;
    [[nodiscard]] bool contains(const card_type& card) const noexcept;

    [[nodiscard]] std::size_t deck_count() const noexcept;
    [[nodiscard]] bool is_win() const noexcept;

    [[nodiscard]] bool can_select_for_remove(const card_type& card) const;
    [[nodiscard]] bool can_remove(const card_type& card) const;
    [[nodiscard]] Pyramid remove(const card_type& card) const;
    [[nodiscard]] bool can_remove(const card_type& first_card, const card_type& second_card) const;
    [[nodiscard]] Pyramid remove(const card_type& first_card, const card_type& second_card) const;

    [[nodiscard]] bool can_redeal() const noexcept;
    [[nodiscard]] Pyramid redeal() const;

    [[nodiscard]] bool can_draw(const card_type& card) const;
    [[nodiscard]] Pyramid draw(const card_type& card) const;

    [[nodiscard]] bool operator==(const Pyramid&) const = default;

private:
    state_type positions_;
};

}  // namespace shlab::gopc::games::pyramid
