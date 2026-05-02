#pragma once

#include <compare>
#include <cstddef>
#include <functional>
#include <optional>

#include "shlab/gopc/playing_cards/card_id.hpp"
#include "shlab/gopc/playing_cards/rank.hpp"
#include "shlab/gopc/playing_cards/suit.hpp"

namespace shlab::gopc::playing_cards {

class Card {
public:
    explicit Card(CardId id) noexcept;
    Card(Suit suit, Rank rank);

    static Card from_id(CardId id) noexcept;
    static Card of(Suit suit, Rank rank);
    static std::optional<Card> try_of(Suit suit, Rank rank) noexcept;

    [[nodiscard]] CardId id() const noexcept;
    [[nodiscard]] Rank rank() const noexcept;
    [[nodiscard]] Suit suit() const noexcept;
    [[nodiscard]] bool is_joker() const noexcept;
    [[nodiscard]] bool is_extra_joker() const noexcept;
    [[nodiscard]] bool is_standard_card() const noexcept;

    auto operator<=>(const Card&) const noexcept = default;

private:
    CardId id_;
};

}  // namespace shlab::gopc::playing_cards

namespace std {

template <>
struct hash<shlab::gopc::playing_cards::Card> {
    std::size_t operator()(const shlab::gopc::playing_cards::Card& card) const noexcept;
};

}  // namespace std
