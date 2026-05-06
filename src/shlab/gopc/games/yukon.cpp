#include "shlab/gopc/games/yukon.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

#include "shlab/gopc/playing_cards/rank.hpp"
#include "shlab/gopc/playing_cards/suit.hpp"

namespace shlab::gopc::games::yukon {
namespace {

using Card = shlab::gopc::playing_cards::Card;
using Rank = shlab::gopc::playing_cards::Rank;
using Suit = shlab::gopc::playing_cards::Suit;

constexpr std::size_t kStandardDeckSize = 52;
constexpr int kTableauColumnCount = 7;
constexpr std::array kColumns{
    Column::First,
    Column::Second,
    Column::Third,
    Column::Fourth,
    Column::Fifth,
    Column::Sixth,
    Column::Seventh,
};

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

bool IsRed(const Suit suit) noexcept {
    return suit == Suit::Hearts || suit == Suit::Diamonds;
}

bool IsAlternatingDescending(const Card& lower_card, const Card& upper_card) noexcept {
    return IsRed(lower_card.suit()) != IsRed(upper_card.suit())
        && ToInt(upper_card.rank()) + 1 == ToInt(lower_card.rank());
}

std::optional<Card> PreviousRankCard(const Card& card) noexcept {
    if (card.rank() == Rank::Ace) {
        return std::nullopt;
    }

    return Card::try_of(card.suit(), static_cast<Rank>(ToInt(card.rank()) - 1));
}

const Position& LookupPosition(const Yukon::state_type& positions, const Card& card) {
    const auto it = positions.find(card);
    if (it == positions.end()) {
        throw std::out_of_range("Card is not part of this Yukon state.");
    }

    return it->second;
}

auto MutablePosition(Yukon::state_type& positions, const Card& card) -> Position& {
    const auto it = positions.find(card);
    if (it == positions.end()) {
        throw std::out_of_range("Card is not part of this Yukon state.");
    }

    return it->second;
}

bool IsTopTableauCard(const Yukon::state_type& positions, const Card& card) {
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

bool IsTopFoundationCard(const Yukon::state_type& positions, const Card& card) {
    if (!HoldsAlternative<Foundation>(LookupPosition(positions, card))) {
        return false;
    }

    if (card.rank() == Rank::King) {
        return true;
    }

    const auto next_card = Card::try_of(card.suit(), static_cast<Rank>(ToInt(card.rank()) + 1));
    return !next_card.has_value()
        || !HoldsAlternative<Foundation>(LookupPosition(positions, *next_card));
}

std::size_t TableauCount(const Yukon::state_type& positions, const Column column) noexcept {
    return std::count_if(
        positions.begin(),
        positions.end(),
        [column](const auto& pair) {
            const auto* tableau = GetIf<Tableau>(&pair.second);
            return tableau != nullptr && tableau->column == column;
        });
}

std::optional<Card> TableauTopCard(const Yukon::state_type& positions, const Column column) {
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

std::vector<std::pair<int, Card>> OrderedTableauTail(
    const Yukon::state_type& positions,
    const Card& card) {
    const auto* tableau = GetIf<Tableau>(&LookupPosition(positions, card));
    if (tableau == nullptr) {
        return {};
    }

    std::vector<std::pair<int, Card>> stack;
    for (const auto& [candidate_card, position] : positions) {
        if (const auto* candidate = GetIf<Tableau>(&position);
            candidate != nullptr
            && candidate->column == tableau->column
            && candidate->number >= tableau->number) {
            stack.emplace_back(candidate->number, candidate_card);
        }
    }

    std::sort(
        stack.begin(),
        stack.end(),
        [](const auto& lhs, const auto& rhs) {
            return lhs.first < rhs.first;
        });

    return stack;
}

void ValidateCardSet(const Yukon::state_type& positions) {
    if (positions.size() != kStandardDeckSize) {
        ThrowInvalidState("Yukon state must contain exactly 52 standard cards.");
    }

    for (const auto& [card, position] : positions) {
        (void)position;

        if (!card.is_standard_card()) {
            ThrowInvalidState("Yukon state cannot contain jokers.");
        }
    }
}

void ValidateTableau(const Yukon::state_type& positions) {
    std::array<std::vector<Tableau>, kTableauColumnCount> columns;

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

            columns[static_cast<std::size_t>(column_index)].push_back(*tableau);
        }
    }

    for (auto& column : columns) {
        std::sort(
            column.begin(),
            column.end(),
            [](const Tableau& lhs, const Tableau& rhs) {
                return lhs.number < rhs.number;
            });

        bool seen_open = false;
        for (std::size_t i = 0; i < column.size(); ++i) {
            if (column[i].number != static_cast<int>(i)) {
                ThrowInvalidState("Tableau positions must be contiguous within each column.");
            }

            if (seen_open && !column[i].open) {
                ThrowInvalidState("Closed tableau cards cannot appear above open cards.");
            }

            seen_open = seen_open || column[i].open;
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

void ValidateFoundation(const Yukon::state_type& positions) {
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

void ValidateState(const Yukon::state_type& positions) {
    ValidateCardSet(positions);
    ValidateTableau(positions);
    ValidateFoundation(positions);
}

}  // namespace

Yukon::Yukon(state_type positions)
    : positions_(std::move(positions)) {
    ValidateState(positions_);
}

Yukon Yukon::deal(std::span<const card_type> deck) {
    if (deck.size() != kStandardDeckSize) {
        ThrowInvalidState("Yukon deals require exactly 52 cards.");
    }

    state_type positions;
    positions.reserve(deck.size());

    for (const auto& card : deck) {
        if (!card.is_standard_card()) {
            ThrowInvalidState("Yukon deals require standard cards only.");
        }
        positions.emplace(card, Foundation{});
    }

    if (positions.size() != deck.size()) {
        ThrowInvalidState("Yukon deals require 52 distinct cards.");
    }

    std::size_t deck_index = 0;
    for (int column_index = 0; column_index < kTableauColumnCount; ++column_index) {
        const auto column = static_cast<Column>(column_index);

        for (int number = 0; number < column_index; ++number) {
            positions[deck[deck_index++]] = Tableau{column, number, false};
        }

        positions[deck[deck_index++]] = Tableau{column, column_index, true};

        if (column != Column::First) {
            positions[deck[deck_index++]] = Tableau{column, column_index + 1, true};
            positions[deck[deck_index++]] = Tableau{column, column_index + 2, true};
            positions[deck[deck_index++]] = Tableau{column, column_index + 3, true};
            positions[deck[deck_index++]] = Tableau{column, column_index + 4, true};
        }
    }

    return Yukon{std::move(positions)};
}

const Yukon::state_type& Yukon::card_positions() const noexcept {
    return positions_;
}

const Position& Yukon::position_of(const card_type& card) const {
    return LookupPosition(positions_, card);
}

bool Yukon::contains(const card_type& card) const noexcept {
    return positions_.contains(card);
}

bool Yukon::is_win() const noexcept {
    return std::all_of(
        positions_.begin(),
        positions_.end(),
        [](const auto& pair) {
            return HoldsAlternative<Foundation>(pair.second);
        });
}

bool Yukon::is_moved_foundation_no_problem(const card_type& card) const {
    if (!can_move_to_foundation(card)) {
        return false;
    }

    if (card.rank() == Rank::Ace) {
        return true;
    }

    const auto previous_rank = static_cast<Rank>(ToInt(card.rank()) - 1);
    return std::all_of(
        positions_.begin(),
        positions_.end(),
        [previous_rank](const auto& pair) {
            return pair.first.rank() != previous_rank
                || HoldsAlternative<Foundation>(pair.second);
        });
}

bool Yukon::can_open(const card_type& card) const {
    const auto* tableau = GetIf<Tableau>(&position_of(card));
    return tableau != nullptr && !tableau->open && IsTopTableauCard(positions_, card);
}

Yukon Yukon::open(const card_type& card) const {
    if (!can_open(card)) {
        ThrowInvalidMove("Card cannot be opened.");
    }

    auto next_positions = positions_;
    const auto tableau = Get<Tableau>(position_of(card));
    MutablePosition(next_positions, card) = Tableau{tableau.column, tableau.number, true};
    return Yukon{std::move(next_positions)};
}

bool Yukon::can_move_to_foundation(const card_type& card) const {
    const auto* tableau = GetIf<Tableau>(&position_of(card));
    if (tableau == nullptr || !tableau->open || !IsTopTableauCard(positions_, card)) {
        return false;
    }

    if (card.rank() == Rank::Ace) {
        return true;
    }

    const auto previous_card = PreviousRankCard(card);
    return previous_card.has_value() && HoldsAlternative<Foundation>(position_of(*previous_card));
}

Yukon Yukon::move_to_foundation(const card_type& card) const {
    if (!can_move_to_foundation(card)) {
        ThrowInvalidMove("Card cannot be moved to the foundation.");
    }

    auto next_positions = positions_;
    MutablePosition(next_positions, card) = Foundation{};
    return Yukon{std::move(next_positions)};
}

bool Yukon::can_move_to_tableau(const card_type& card, const Column column) const {
    const auto* tableau = GetIf<Tableau>(&position_of(card));
    if (tableau == nullptr || !tableau->open || tableau->column == column) {
        return false;
    }

    const auto destination_top = TableauTopCard(positions_, column);
    if (!destination_top.has_value()) {
        return card.rank() == Rank::King;
    }

    const auto* destination_top_tableau = GetIf<Tableau>(&position_of(*destination_top));
    if (destination_top_tableau == nullptr || !destination_top_tableau->open) {
        return false;
    }

    return IsAlternatingDescending(*destination_top, card);
}

Yukon Yukon::move_to_tableau(const card_type& card, const Column column) const {
    if (!can_move_to_tableau(card, column)) {
        ThrowInvalidMove("Card cannot be moved to the tableau.");
    }

    auto next_positions = positions_;
    auto destination_count = static_cast<int>(TableauCount(next_positions, column));

    const auto stack = OrderedTableauTail(next_positions, card);
    for (const auto& [unused_number, current_card] : stack) {
        (void)unused_number;
        MutablePosition(next_positions, current_card) = Tableau{column, destination_count++, true};
    }

    return Yukon{std::move(next_positions)};
}

}  // namespace shlab::gopc::games::yukon
