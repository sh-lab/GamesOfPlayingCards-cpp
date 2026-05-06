#include "shlab/gopc/games/beleaguered_castle.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

#include "shlab/gopc/playing_cards/rank.hpp"
#include "shlab/gopc/playing_cards/suit.hpp"

namespace shlab::gopc::games::beleaguered_castle {
namespace {

using Card = shlab::gopc::playing_cards::Card;
using Rank = shlab::gopc::playing_cards::Rank;
using Suit = shlab::gopc::playing_cards::Suit;

constexpr std::size_t kStandardDeckSize = 52;
constexpr int kTableauColumnCount = 8;
constexpr int kTableauDepth = 6;

[[noreturn]] void ThrowInvalidState(const char* message) {
    throw std::invalid_argument(message);
}

[[noreturn]] void ThrowInvalidMove(const char* message) {
    throw std::logic_error(message);
}

int ToInt(const Rank rank) noexcept {
    return static_cast<int>(rank);
}

int ToInt(const Column column) noexcept {
    return static_cast<int>(column);
}

const Position& LookupPosition(const BeleagueredCastle::state_type& positions, const Card& card) {
    const auto it = positions.find(card);
    if (it == positions.end()) {
        throw std::out_of_range("Card is not part of this Beleaguered Castle state.");
    }

    return it->second;
}

auto MutablePosition(BeleagueredCastle::state_type& positions, const Card& card) -> Position& {
    const auto it = positions.find(card);
    if (it == positions.end()) {
        throw std::out_of_range("Card is not part of this Beleaguered Castle state.");
    }

    return it->second;
}

std::optional<Card> PreviousRankCard(const Card& card) noexcept {
    if (card.rank() == Rank::Ace) {
        return std::nullopt;
    }

    return Card::try_of(card.suit(), static_cast<Rank>(ToInt(card.rank()) - 1));
}

bool IsTopTableauCard(const BeleagueredCastle::state_type& positions, const Card& card) {
    const auto* tableau = GetIf<Tableau>(&LookupPosition(positions, card));
    if (tableau == nullptr) {
        return false;
    }

    for (const auto& [other_card, position] : positions) {
        (void)other_card;
        if (const auto* other_tableau = GetIf<Tableau>(&position);
            other_tableau != nullptr
            && other_tableau->column == tableau->column
            && other_tableau->number > tableau->number) {
            return false;
        }
    }

    return true;
}

std::optional<Card> TableauTopCard(
    const BeleagueredCastle::state_type& positions,
    const Column column) {
    std::optional<std::pair<int, Card>> result;

    for (const auto& [card, position] : positions) {
        if (const auto* tableau = GetIf<Tableau>(&position);
            tableau != nullptr && tableau->column == column) {
            if (!result.has_value() || result->first < tableau->number) {
                result = std::pair{tableau->number, card};
            }
        }
    }

    if (!result.has_value()) {
        return std::nullopt;
    }

    return result->second;
}

std::size_t TableauCount(
    const BeleagueredCastle::state_type& positions,
    const Column column) noexcept {
    return std::count_if(
        positions.begin(),
        positions.end(),
        [column](const auto& pair) {
            const auto* tableau = GetIf<Tableau>(&pair.second);
            return tableau != nullptr && tableau->column == column;
        });
}

void ValidateCardSet(const BeleagueredCastle::state_type& positions) {
    if (positions.size() != kStandardDeckSize) {
        ThrowInvalidState("Beleaguered Castle state must contain exactly 52 standard cards.");
    }

    for (const auto& [card, position] : positions) {
        (void)position;
        if (!card.is_standard_card()) {
            ThrowInvalidState("Beleaguered Castle state cannot contain jokers.");
        }
    }
}

void ValidateTableau(const BeleagueredCastle::state_type& positions) {
    std::array<std::vector<int>, kTableauColumnCount> columns;

    for (const auto& [card, position] : positions) {
        (void)card;
        if (const auto* tableau = GetIf<Tableau>(&position); tableau != nullptr) {
            if (tableau->number < 0) {
                ThrowInvalidState("Tableau positions must use non-negative indices.");
            }

            const auto column_index = ToInt(tableau->column);
            if (column_index < 0 || column_index >= kTableauColumnCount) {
                ThrowInvalidState("Tableau column is out of range.");
            }

            columns[static_cast<std::size_t>(column_index)].push_back(tableau->number);
        }
    }

    for (auto& column : columns) {
        std::sort(column.begin(), column.end());
        for (std::size_t i = 0; i < column.size(); ++i) {
            if (column[i] != static_cast<int>(i)) {
                ThrowInvalidState("Tableau positions must be contiguous within each column.");
            }
        }
    }
}

std::size_t SuitIndex(const Suit suit) {
    switch (suit) {
    case Suit::Spades:
        return 0;
    case Suit::Hearts:
        return 1;
    case Suit::Diamonds:
        return 2;
    case Suit::Clubs:
        return 3;
    case Suit::None:
        break;
    }

    ThrowInvalidState("Foundation cards must use a standard suit.");
}

void ValidateFoundation(const BeleagueredCastle::state_type& positions) {
    std::array<std::vector<int>, 4> ranks_by_suit;

    for (const auto& [card, position] : positions) {
        if (HoldsAlternative<Foundation>(position)) {
            ranks_by_suit[SuitIndex(card.suit())].push_back(ToInt(card.rank()));
        }
    }

    for (auto& ranks : ranks_by_suit) {
        std::sort(ranks.begin(), ranks.end());
        for (std::size_t i = 0; i < ranks.size(); ++i) {
            if (ranks[i] != static_cast<int>(i) + 1) {
                ThrowInvalidState(
                    "Foundation cards must form a contiguous run from ace within each suit.");
            }
        }
    }
}

void ValidateState(const BeleagueredCastle::state_type& positions) {
    ValidateCardSet(positions);
    ValidateTableau(positions);
    ValidateFoundation(positions);
}

}  // namespace

BeleagueredCastle::BeleagueredCastle(state_type positions)
    : positions_(std::move(positions)) {
    ValidateState(positions_);
}

BeleagueredCastle BeleagueredCastle::deal(std::span<const card_type> deck) {
    if (deck.size() != kStandardDeckSize) {
        ThrowInvalidState("Beleaguered Castle deals require exactly 52 cards.");
    }

    state_type positions;
    positions.reserve(deck.size());

    for (const auto& card : deck) {
        if (!card.is_standard_card()) {
            ThrowInvalidState("Beleaguered Castle deals require standard cards only.");
        }
        positions.emplace(card, Tableau{Column::First, 0});
    }

    if (positions.size() != deck.size()) {
        ThrowInvalidState("Beleaguered Castle deals require 52 distinct cards.");
    }

    std::vector<Card> non_aces;
    non_aces.reserve(deck.size() - 4);
    for (const auto& card : deck) {
        if (card.rank() == Rank::Ace) {
            positions[card] = Foundation{};
        } else {
            non_aces.push_back(card);
        }
    }

    if (non_aces.size() != kStandardDeckSize - 4) {
        ThrowInvalidState("Beleaguered Castle deals require one ace of each suit.");
    }

    std::size_t deck_index = 0;
    for (int column_index = 0; column_index < kTableauColumnCount; ++column_index) {
        const auto column = static_cast<Column>(column_index);
        for (int number = 0; number < kTableauDepth; ++number) {
            positions[non_aces[deck_index++]] = Tableau{column, number};
        }
    }

    return BeleagueredCastle{std::move(positions)};
}

const BeleagueredCastle::state_type& BeleagueredCastle::card_positions() const noexcept {
    return positions_;
}

const Position& BeleagueredCastle::position_of(const card_type& card) const {
    return LookupPosition(positions_, card);
}

bool BeleagueredCastle::contains(const card_type& card) const noexcept {
    return positions_.contains(card);
}

bool BeleagueredCastle::is_win() const noexcept {
    return std::all_of(
        positions_.begin(),
        positions_.end(),
        [](const auto& pair) {
            return HoldsAlternative<Foundation>(pair.second);
        });
}

bool BeleagueredCastle::can_move_to_foundation(const card_type& card) const {
    if (!IsTopTableauCard(positions_, card)) {
        return false;
    }

    if (card.rank() == Rank::Ace) {
        return true;
    }

    const auto previous_card = PreviousRankCard(card);
    return previous_card.has_value()
        && HoldsAlternative<Foundation>(position_of(*previous_card));
}

BeleagueredCastle BeleagueredCastle::move_to_foundation(const card_type& card) const {
    if (!can_move_to_foundation(card)) {
        ThrowInvalidMove("Card cannot be moved to the foundation.");
    }

    auto next_positions = positions_;
    MutablePosition(next_positions, card) = Foundation{};
    return BeleagueredCastle{std::move(next_positions)};
}

bool BeleagueredCastle::can_move_to_tableau(const card_type& card, const Column column) const {
    if (!IsTopTableauCard(positions_, card)) {
        return false;
    }

    const auto* source_tableau = GetIf<Tableau>(&position_of(card));
    if (source_tableau == nullptr || source_tableau->column == column) {
        return false;
    }

    const auto destination_top = TableauTopCard(positions_, column);
    if (!destination_top.has_value()) {
        return true;
    }

    return ToInt(card.rank()) + 1 == ToInt(destination_top->rank());
}

BeleagueredCastle BeleagueredCastle::move_to_tableau(const card_type& card, const Column column) const {
    if (!can_move_to_tableau(card, column)) {
        ThrowInvalidMove("Card cannot be moved to the tableau.");
    }

    auto next_positions = positions_;
    const auto destination_count = static_cast<int>(TableauCount(next_positions, column));
    MutablePosition(next_positions, card) = Tableau{column, destination_count};
    return BeleagueredCastle{std::move(next_positions)};
}

}  // namespace shlab::gopc::games::beleaguered_castle
