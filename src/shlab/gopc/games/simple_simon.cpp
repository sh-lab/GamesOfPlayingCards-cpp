#include "shlab/gopc/games/simple_simon.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

#include "shlab/gopc/playing_cards/rank.hpp"
#include "shlab/gopc/playing_cards/suit.hpp"

namespace shlab::gopc::games::simple_simon {
namespace {

using Card = shlab::gopc::playing_cards::Card;
using Rank = shlab::gopc::playing_cards::Rank;
using Suit = shlab::gopc::playing_cards::Suit;

constexpr std::size_t kStandardDeckSize = 52;
constexpr int kTableauColumnCount = 10;
constexpr std::array<int, kTableauColumnCount> kInitialColumnSizes{8, 8, 8, 7, 6, 5, 4, 3, 2, 1};

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

const Position& LookupPosition(const SimpleSimon::state_type& positions, const Card& card) {
    const auto it = positions.find(card);
    if (it == positions.end()) {
        throw std::out_of_range("Card is not part of this Simple Simon state.");
    }

    return it->second;
}

auto MutablePosition(SimpleSimon::state_type& positions, const Card& card) -> Position& {
    const auto it = positions.find(card);
    if (it == positions.end()) {
        throw std::out_of_range("Card is not part of this Simple Simon state.");
    }

    return it->second;
}

bool IsSameSuitDescending(const Card& lower_card, const Card& upper_card) noexcept {
    return lower_card.suit() == upper_card.suit()
        && ToInt(upper_card.rank()) + 1 == ToInt(lower_card.rank());
}

std::size_t TableauCount(const SimpleSimon::state_type& positions, const Column column) noexcept {
    return std::count_if(
        positions.begin(),
        positions.end(),
        [column](const auto& pair) {
            const auto* tableau = GetIf<Tableau>(&pair.second);
            return tableau != nullptr && tableau->column == column;
        });
}

std::optional<Card> TableauTopCard(const SimpleSimon::state_type& positions, const Column column) {
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
    const SimpleSimon::state_type& positions,
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

bool IsMovableTail(const std::vector<std::pair<int, Card>>& stack) noexcept {
    if (stack.empty()) {
        return false;
    }

    for (std::size_t i = 1; i < stack.size(); ++i) {
        if (!IsSameSuitDescending(stack[i - 1].second, stack[i].second)) {
            return false;
        }
    }

    return true;
}

bool IsCompleteSuitRun(const std::vector<std::pair<int, Card>>& stack) noexcept {
    return stack.size() == 13
        && stack.front().second.rank() == Rank::King
        && stack.back().second.rank() == Rank::Ace
        && IsMovableTail(stack);
}

void ValidateCardSet(const SimpleSimon::state_type& positions) {
    if (positions.size() != kStandardDeckSize) {
        ThrowInvalidState("Simple Simon state must contain exactly 52 standard cards.");
    }

    for (const auto& [card, position] : positions) {
        (void)position;
        if (!card.is_standard_card()) {
            ThrowInvalidState("Simple Simon state cannot contain jokers.");
        }
    }
}

void ValidateTableau(const SimpleSimon::state_type& positions) {
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

void ValidateFoundation(const SimpleSimon::state_type& positions) {
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

void ValidateState(const SimpleSimon::state_type& positions) {
    ValidateCardSet(positions);
    ValidateTableau(positions);
    ValidateFoundation(positions);
}

}  // namespace

SimpleSimon::SimpleSimon(state_type positions)
    : positions_(std::move(positions)) {
    ValidateState(positions_);
}

SimpleSimon SimpleSimon::deal(std::span<const card_type> deck) {
    if (deck.size() != kStandardDeckSize) {
        ThrowInvalidState("Simple Simon deals require exactly 52 cards.");
    }

    state_type positions;
    positions.reserve(deck.size());

    for (const auto& card : deck) {
        if (!card.is_standard_card()) {
            ThrowInvalidState("Simple Simon deals require standard cards only.");
        }
        positions.emplace(card, Foundation{});
    }

    if (positions.size() != deck.size()) {
        ThrowInvalidState("Simple Simon deals require 52 distinct cards.");
    }

    std::size_t deck_index = 0;
    for (int column_index = 0; column_index < kTableauColumnCount; ++column_index) {
        const auto column = static_cast<Column>(column_index);
        for (int number = 0; number < kInitialColumnSizes[static_cast<std::size_t>(column_index)]; ++number) {
            positions[deck[deck_index++]] = Tableau{column, number};
        }
    }

    return SimpleSimon{std::move(positions)};
}

const SimpleSimon::state_type& SimpleSimon::card_positions() const noexcept {
    return positions_;
}

const Position& SimpleSimon::position_of(const card_type& card) const {
    return LookupPosition(positions_, card);
}

bool SimpleSimon::contains(const card_type& card) const noexcept {
    return positions_.contains(card);
}

bool SimpleSimon::is_win() const noexcept {
    return std::all_of(
        positions_.begin(),
        positions_.end(),
        [](const auto& pair) {
            return HoldsAlternative<Foundation>(pair.second);
        });
}

bool SimpleSimon::can_move_to_foundation(const card_type& card) const {
    return IsCompleteSuitRun(OrderedTableauTail(positions_, card));
}

SimpleSimon SimpleSimon::move_to_foundation(const card_type& card) const {
    const auto stack = OrderedTableauTail(positions_, card);
    if (!IsCompleteSuitRun(stack)) {
        ThrowInvalidMove("Card cannot be moved to the foundation.");
    }

    auto next_positions = positions_;
    for (const auto& [unused_number, current_card] : stack) {
        (void)unused_number;
        MutablePosition(next_positions, current_card) = Foundation{};
    }
    return SimpleSimon{std::move(next_positions)};
}

bool SimpleSimon::can_move_to_tableau(const card_type& card, const Column column) const {
    const auto* tableau = GetIf<Tableau>(&position_of(card));
    if (tableau == nullptr || tableau->column == column) {
        return false;
    }

    const auto stack = OrderedTableauTail(positions_, card);
    if (!IsMovableTail(stack)) {
        return false;
    }

    const auto destination_top = TableauTopCard(positions_, column);
    if (!destination_top.has_value()) {
        return true;
    }

    return ToInt(card.rank()) + 1 == ToInt(destination_top->rank());
}

SimpleSimon SimpleSimon::move_to_tableau(const card_type& card, const Column column) const {
    const auto stack = OrderedTableauTail(positions_, card);
    if (!can_move_to_tableau(card, column)) {
        ThrowInvalidMove("Card cannot be moved to the tableau.");
    }

    auto next_positions = positions_;
    auto destination_count = static_cast<int>(TableauCount(next_positions, column));
    for (const auto& [unused_number, current_card] : stack) {
        (void)unused_number;
        MutablePosition(next_positions, current_card) = Tableau{column, destination_count++};
    }

    return SimpleSimon{std::move(next_positions)};
}

}  // namespace shlab::gopc::games::simple_simon
