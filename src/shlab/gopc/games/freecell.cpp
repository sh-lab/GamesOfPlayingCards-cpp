#include "shlab/gopc/games/freecell.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

#include "shlab/gopc/playing_cards/rank.hpp"
#include "shlab/gopc/playing_cards/suit.hpp"

namespace shlab::gopc::games::freecell {
namespace {

using Card = shlab::gopc::playing_cards::Card;
using Rank = shlab::gopc::playing_cards::Rank;
using Suit = shlab::gopc::playing_cards::Suit;

constexpr std::size_t kStandardDeckSize = 52;
constexpr int kTableauColumnCount = 8;
constexpr int kCellCount = 4;
constexpr std::array kColumns{
    Column::First,
    Column::Second,
    Column::Third,
    Column::Fourth,
    Column::Fifth,
    Column::Sixth,
    Column::Seventh,
    Column::Eighth,
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

bool CanStackOnTableau(const Card& moving_card, const Card& destination_card) noexcept {
    return IsRed(moving_card.suit()) != IsRed(destination_card.suit())
        && ToInt(moving_card.rank()) + 1 == ToInt(destination_card.rank());
}

std::optional<Card> PreviousRankCard(const Card& card) noexcept {
    if (card.rank() == Rank::Ace) {
        return std::nullopt;
    }

    return Card::try_of(card.suit(), static_cast<Rank>(ToInt(card.rank()) - 1));
}

const Position& LookupPosition(const FreeCell::state_type& positions, const Card& card) {
    const auto it = positions.find(card);
    if (it == positions.end()) {
        throw std::out_of_range("Card is not part of this FreeCell state.");
    }

    return it->second;
}

auto MutablePosition(FreeCell::state_type& positions, const Card& card) -> Position& {
    const auto it = positions.find(card);
    if (it == positions.end()) {
        throw std::out_of_range("Card is not part of this FreeCell state.");
    }

    return it->second;
}

bool IsTopTableauCard(const FreeCell::state_type& positions, const Card& card) {
    const auto* tableau = std::get_if<Tableau>(&LookupPosition(positions, card));
    if (tableau == nullptr) {
        return false;
    }

    for (const auto& [other_card, position] : positions) {
        (void)other_card;

        if (const auto* other_tableau = std::get_if<Tableau>(&position);
            other_tableau != nullptr
            && other_tableau->column == tableau->column
            && other_tableau->number > tableau->number) {
            return false;
        }
    }

    return true;
}

std::size_t TableauCount(const FreeCell::state_type& positions, const Column column) noexcept {
    return std::count_if(
        positions.begin(),
        positions.end(),
        [column](const auto& pair) {
            const auto* tableau = std::get_if<Tableau>(&pair.second);
            return tableau != nullptr && tableau->column == column;
        });
}

std::optional<Card> TableauTopCard(const FreeCell::state_type& positions, const Column column) {
    std::optional<std::pair<int, Card>> result;

    for (const auto& [card, position] : positions) {
        if (const auto* tableau = std::get_if<Tableau>(&position);
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

std::vector<std::pair<int, Card>> OrderedTableauStack(
    const FreeCell::state_type& positions,
    const Card& card) {
    const auto* tableau = std::get_if<Tableau>(&LookupPosition(positions, card));
    if (tableau == nullptr) {
        return {};
    }

    std::vector<std::pair<int, Card>> stack;
    for (const auto& [candidate_card, position] : positions) {
        if (const auto* candidate = std::get_if<Tableau>(&position);
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

bool HasValidMovableTableauSequence(
    const FreeCell::state_type& positions,
    const Card& card) {
    const auto stack = OrderedTableauStack(positions, card);
    if (stack.empty()) {
        return false;
    }

    auto target_card = stack.front().second;
    for (std::size_t i = 1; i < stack.size(); ++i) {
        const auto current_card = stack[i].second;
        if (!CanStackOnTableau(current_card, target_card)) {
            return false;
        }
        target_card = current_card;
    }

    return true;
}

std::size_t OccupiedCellCount(const FreeCell::state_type& positions) noexcept {
    return std::count_if(
        positions.begin(),
        positions.end(),
        [](const auto& pair) {
            return std::holds_alternative<Cell>(pair.second);
        });
}

std::optional<int> FirstEmptyCellNumber(const FreeCell::state_type& positions) {
    for (int cell_number = 0; cell_number < kCellCount; ++cell_number) {
        const auto occupied = std::any_of(
            positions.begin(),
            positions.end(),
            [cell_number](const auto& pair) {
                const auto* cell = std::get_if<Cell>(&pair.second);
                return cell != nullptr && cell->number == cell_number;
            });

        if (!occupied) {
            return cell_number;
        }
    }

    return std::nullopt;
}

bool CanMoveTableauToTableau(
    const FreeCell::state_type& positions,
    const Card& card,
    const Column destination_column) {
    const auto* tableau = std::get_if<Tableau>(&LookupPosition(positions, card));
    if (tableau == nullptr || tableau->column == destination_column) {
        return false;
    }

    const auto stack = OrderedTableauStack(positions, card);
    if (stack.empty()) {
        return false;
    }

    if (!HasValidMovableTableauSequence(positions, card)) {
        return false;
    }

    const auto empty_tableau_count = std::count_if(
        kColumns.begin(),
        kColumns.end(),
        [&](const Column column) {
            return column != destination_column
                && !std::any_of(
                    positions.begin(),
                    positions.end(),
                    [column](const auto& pair) {
                        const auto* current_tableau = std::get_if<Tableau>(&pair.second);
                        return current_tableau != nullptr && current_tableau->column == column;
                    });
        });

    auto movable_card_capacity = static_cast<std::size_t>(kCellCount - OccupiedCellCount(positions) + 1);
    for (int i = 0; i < empty_tableau_count; ++i) {
        movable_card_capacity *= 2;
    }

    return stack.size() <= movable_card_capacity;
}

void ValidateCardSet(const FreeCell::state_type& positions) {
    if (positions.size() != kStandardDeckSize) {
        ThrowInvalidState("FreeCell state must contain exactly 52 standard cards.");
    }

    for (const auto& [card, position] : positions) {
        (void)position;

        if (!card.is_standard_card()) {
            ThrowInvalidState("FreeCell state cannot contain jokers.");
        }
    }
}

void ValidateTableau(const FreeCell::state_type& positions) {
    std::array<std::vector<int>, kTableauColumnCount> columns;

    for (const auto& [card, position] : positions) {
        (void)card;

        if (const auto* tableau = std::get_if<Tableau>(&position); tableau != nullptr) {
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

void ValidateCells(const FreeCell::state_type& positions) {
    std::array<bool, kCellCount> occupied_cells{};

    for (const auto& [card, position] : positions) {
        (void)card;

        if (const auto* cell = std::get_if<Cell>(&position); cell != nullptr) {
            if (cell->number < 0 || cell->number >= kCellCount) {
                ThrowInvalidState("Cell positions must be in the range [0, 3].");
            }

            auto& occupied = occupied_cells[static_cast<std::size_t>(cell->number)];
            if (occupied) {
                ThrowInvalidState("Cell positions must be unique.");
            }

            occupied = true;
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

void ValidateFoundation(const FreeCell::state_type& positions) {
    std::array<std::vector<int>, 4> ranks_by_suit;

    for (const auto& [card, position] : positions) {
        if (std::holds_alternative<Foundation>(position)) {
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

void ValidateState(const FreeCell::state_type& positions) {
    ValidateCardSet(positions);
    ValidateTableau(positions);
    ValidateCells(positions);
    ValidateFoundation(positions);
}

}  // namespace

FreeCell::FreeCell(state_type positions)
    : positions_(std::move(positions)) {
    ValidateState(positions_);
}

FreeCell FreeCell::deal(std::span<const card_type> deck) {
    if (deck.size() != kStandardDeckSize) {
        ThrowInvalidState("FreeCell deals require exactly 52 cards.");
    }

    state_type positions;
    positions.reserve(deck.size());

    for (const auto& card : deck) {
        if (!card.is_standard_card()) {
            ThrowInvalidState("FreeCell deals require standard cards only.");
        }
        positions.emplace(card, Foundation{});
    }

    if (positions.size() != deck.size()) {
        ThrowInvalidState("FreeCell deals require 52 distinct cards.");
    }

    std::size_t deck_index = 0;
    for (int column_index = 0; column_index < kTableauColumnCount; ++column_index) {
        const auto column = static_cast<Column>(column_index);
        const auto card_count = column_index < 4 ? 7 : 6;

        for (int number = 0; number < card_count; ++number) {
            positions[deck[deck_index++]] = Tableau{column, number};
        }
    }

    return FreeCell{std::move(positions)};
}

const FreeCell::state_type& FreeCell::card_positions() const noexcept {
    return positions_;
}

const Position& FreeCell::position_of(const card_type& card) const {
    return LookupPosition(positions_, card);
}

bool FreeCell::contains(const card_type& card) const noexcept {
    return positions_.contains(card);
}

bool FreeCell::is_win() const noexcept {
    return std::all_of(
        positions_.begin(),
        positions_.end(),
        [](const auto& pair) {
            return std::holds_alternative<Foundation>(pair.second);
        });
}

bool FreeCell::can_move_to_cell(const card_type& card) const {
    return std::holds_alternative<Tableau>(position_of(card))
        && IsTopTableauCard(positions_, card)
        && FirstEmptyCellNumber(positions_).has_value();
}

FreeCell FreeCell::move_to_cell(const card_type& card) const {
    if (!can_move_to_cell(card)) {
        ThrowInvalidMove("Card cannot be moved to a cell.");
    }

    auto next_positions = positions_;
    MutablePosition(next_positions, card) = Cell{*FirstEmptyCellNumber(next_positions)};
    return FreeCell{std::move(next_positions)};
}

bool FreeCell::can_move_to_foundation(const card_type& card) const {
    const auto& position = position_of(card);

    if (card.rank() != Rank::Ace) {
        const auto previous_card = PreviousRankCard(card);
        if (!previous_card.has_value()
            || !std::holds_alternative<Foundation>(position_of(*previous_card))) {
            return false;
        }
    }

    if (std::holds_alternative<Cell>(position)) {
        return true;
    }

    if (std::holds_alternative<Tableau>(position)) {
        return IsTopTableauCard(positions_, card);
    }

    return false;
}

FreeCell FreeCell::move_to_foundation(const card_type& card) const {
    if (!can_move_to_foundation(card)) {
        ThrowInvalidMove("Card cannot be moved to the foundation.");
    }

    auto next_positions = positions_;
    MutablePosition(next_positions, card) = Foundation{};
    return FreeCell{std::move(next_positions)};
}

bool FreeCell::can_move_to_tableau(const card_type& card, const Column column) const {
    const auto& position = position_of(card);
    if (!std::holds_alternative<Cell>(position) && !CanMoveTableauToTableau(positions_, card, column)) {
        return false;
    }

    const auto destination_top = TableauTopCard(positions_, column);
    if (!destination_top.has_value()) {
        return true;
    }

    return CanStackOnTableau(card, *destination_top);
}

FreeCell FreeCell::move_to_tableau(const card_type& card, const Column column) const {
    if (!can_move_to_tableau(card, column)) {
        ThrowInvalidMove("Card cannot be moved to the tableau.");
    }

    auto next_positions = positions_;
    auto destination_count = static_cast<int>(TableauCount(next_positions, column));

    if (std::holds_alternative<Tableau>(position_of(card))) {
        const auto stack = OrderedTableauStack(next_positions, card);
        for (const auto& [unused_number, current_card] : stack) {
            (void)unused_number;
            MutablePosition(next_positions, current_card) = Tableau{column, destination_count++};
        }
    } else {
        MutablePosition(next_positions, card) = Tableau{column, destination_count};
    }

    return FreeCell{std::move(next_positions)};
}

}  // namespace shlab::gopc::games::freecell
