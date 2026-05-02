#include "shlab/gopc/playing_cards/card.hpp"

#include <cstdint>
#include <stdexcept>

namespace shlab::gopc::playing_cards {
namespace {

constexpr std::uint8_t kSuitMask = 0xF0;
constexpr std::uint8_t kRankMask = 0x0F;

constexpr std::uint8_t ToByte(const CardId id) noexcept {
    return static_cast<std::uint8_t>(id);
}

constexpr std::uint8_t ToByte(const Suit suit) noexcept {
    return static_cast<std::uint8_t>(suit);
}

constexpr std::uint8_t ToByte(const Rank rank) noexcept {
    return static_cast<std::uint8_t>(rank);
}

constexpr bool IsStandardSuit(const Suit suit) noexcept {
    switch (suit) {
    case Suit::Spades:
    case Suit::Hearts:
    case Suit::Diamonds:
    case Suit::Clubs:
        return true;
    case Suit::None:
        return false;
    }

    return false;
}

constexpr bool IsStandardRank(const Rank rank) noexcept {
    switch (rank) {
    case Rank::Ace:
    case Rank::Two:
    case Rank::Three:
    case Rank::Four:
    case Rank::Five:
    case Rank::Six:
    case Rank::Seven:
    case Rank::Eight:
    case Rank::Nine:
    case Rank::Ten:
    case Rank::Jack:
    case Rank::Queen:
    case Rank::King:
        return true;
    case Rank::None:
        return false;
    }

    return false;
}

constexpr std::optional<CardId> TryComposeCardId(const Suit suit, const Rank rank) noexcept {
    if (!IsStandardSuit(suit) || !IsStandardRank(rank)) {
        return std::nullopt;
    }

    return static_cast<CardId>(ToByte(suit) | ToByte(rank));
}

}  // namespace

Card::Card(const CardId id) noexcept
    : id_(id) {}

Card::Card(const Suit suit, const Rank rank)
    : id_(CardId::Joker) {
    const auto id = TryComposeCardId(suit, rank);
    if (!id.has_value()) {
        throw std::invalid_argument(
            "Card requires a standard suit and rank; use CardId for jokers.");
    }

    id_ = *id;
}

Card Card::from_id(const CardId id) noexcept {
    return Card{id};
}

Card Card::of(const Suit suit, const Rank rank) {
    return Card{suit, rank};
}

auto Card::try_of(const Suit suit, const Rank rank) noexcept -> std::optional<Card> {
    const auto id = TryComposeCardId(suit, rank);
    if (!id.has_value()) {
        return std::nullopt;
    }

    return Card{*id};
}

CardId Card::id() const noexcept {
    return id_;
}

Rank Card::rank() const noexcept {
    if (is_joker()) {
        return Rank::None;
    }

    return static_cast<Rank>(ToByte(id_) & kRankMask);
}

Suit Card::suit() const noexcept {
    if (is_joker()) {
        return Suit::None;
    }

    return static_cast<Suit>(ToByte(id_) & kSuitMask);
}

bool Card::is_joker() const noexcept {
    return id_ == CardId::Joker || id_ == CardId::ExtraJoker;
}

bool Card::is_extra_joker() const noexcept {
    return id_ == CardId::ExtraJoker;
}

bool Card::is_standard_card() const noexcept {
    return !is_joker();
}

}  // namespace shlab::gopc::playing_cards

namespace std {

std::size_t hash<shlab::gopc::playing_cards::Card>::operator()(
    const shlab::gopc::playing_cards::Card& card) const noexcept {
    return static_cast<std::size_t>(card.id());
}

}  // namespace std
