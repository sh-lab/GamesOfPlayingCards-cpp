#include "shlab/gopc/games/klondike.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

#include "shlab/gopc/playing_cards/rank.hpp"
#include "shlab/gopc/playing_cards/suit.hpp"

namespace shlab::gopc::games::klondike {
namespace {

using Card = shlab::gopc::playing_cards::Card;
using Rank = shlab::gopc::playing_cards::Rank;
using Suit = shlab::gopc::playing_cards::Suit;

constexpr std::size_t kStandardDeckSize = 52;
constexpr int kTableauColumnCount = 7;

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

std::optional<Card> NextRankCard(const Card& card) noexcept {
    if (card.rank() == Rank::King) {
        return std::nullopt;
    }

    return Card::try_of(card.suit(), static_cast<Rank>(ToInt(card.rank()) + 1));
}

const Position& LookupPosition(
    const Klondike::state_type& positions,
    const Card& card) {
    const auto it = positions.find(card);
    if (it == positions.end()) {
        throw std::out_of_range("Card is not part of this Klondike state.");
    }

    return it->second;
}

auto MutablePosition(
    Klondike::state_type& positions,
    const Card& card) -> Position& {
    const auto it = positions.find(card);
    if (it == positions.end()) {
        throw std::out_of_range("Card is not part of this Klondike state.");
    }

    return it->second;
}

bool IsTopWasteCard(const Klondike::state_type& positions, const Card& card) {
    const auto* waste = std::get_if<WastePile>(&LookupPosition(positions, card));
    if (waste == nullptr) {
        return false;
    }

    for (const auto& [other_card, position] : positions) {
        (void)other_card;

        if (const auto* other_waste = std::get_if<WastePile>(&position);
            other_waste != nullptr && other_waste->number > waste->number) {
            return false;
        }
    }

    return true;
}

bool IsTopStockCard(const Klondike::state_type& positions, const Card& card) {
    const auto* stock = std::get_if<Stock>(&LookupPosition(positions, card));
    if (stock == nullptr) {
        return false;
    }

    for (const auto& [other_card, position] : positions) {
        (void)other_card;

        if (const auto* other_stock = std::get_if<Stock>(&position);
            other_stock != nullptr && other_stock->number > stock->number) {
            return false;
        }
    }

    return true;
}

bool IsTopTableauCard(const Klondike::state_type& positions, const Card& card) {
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

bool IsTopFoundationCard(const Klondike::state_type& positions, const Card& card) {
    if (!std::holds_alternative<Foundation>(LookupPosition(positions, card))) {
        return false;
    }

    const auto next_card = NextRankCard(card);
    return !next_card.has_value()
        || !std::holds_alternative<Foundation>(LookupPosition(positions, *next_card));
}

std::size_t TableauCount(
    const Klondike::state_type& positions,
    const Column column) noexcept {
    return std::count_if(
        positions.begin(),
        positions.end(),
        [column](const auto& pair) {
            const auto* tableau = std::get_if<Tableau>(&pair.second);
            return tableau != nullptr && tableau->column == column;
        });
}

std::vector<std::pair<int, Card>> OrderedTableauStack(
    const Klondike::state_type& positions,
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
    const Klondike::state_type& positions,
    const Card& card) {
    const auto* tableau = std::get_if<Tableau>(&LookupPosition(positions, card));
    if (tableau == nullptr || !tableau->open) {
        return false;
    }

    const auto stack = OrderedTableauStack(positions, card);
    if (stack.empty()) {
        return false;
    }

    auto previous = stack.front().second;
    for (std::size_t i = 1; i < stack.size(); ++i) {
        const auto current = stack[i].second;
        const auto* current_tableau = std::get_if<Tableau>(&LookupPosition(positions, current));
        if (current_tableau == nullptr || !current_tableau->open) {
            return false;
        }

        if (!IsAlternatingDescending(previous, current)) {
            return false;
        }

        previous = current;
    }

    return true;
}

std::optional<Card> TableauTopCard(
    const Klondike::state_type& positions,
    const Column column) {
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

void ValidateDistinctStandardCards(const Klondike::state_type& positions) {
    if (positions.size() != kStandardDeckSize) {
        ThrowInvalidState("Klondike state must contain exactly 52 standard cards.");
    }

    for (const auto& [card, position] : positions) {
        (void)position;

        if (!card.is_standard_card()) {
            ThrowInvalidState("Klondike state cannot contain jokers.");
        }
    }
}

void ValidateSequence(const std::vector<int>& numbers, const char* message) {
    for (std::size_t i = 0; i < numbers.size(); ++i) {
        if (numbers[i] != static_cast<int>(i)) {
            ThrowInvalidState(message);
        }
    }
}

void ValidateStockAndWaste(const Klondike::state_type& positions) {
    std::vector<int> stock_numbers;
    std::vector<int> waste_numbers;

    for (const auto& [card, position] : positions) {
        (void)card;

        if (const auto* stock = std::get_if<Stock>(&position); stock != nullptr) {
            if (stock->number < 0) {
                ThrowInvalidState("Stock positions must use non-negative indices.");
            }
            stock_numbers.push_back(stock->number);
        } else if (const auto* waste = std::get_if<WastePile>(&position); waste != nullptr) {
            if (waste->number < 0) {
                ThrowInvalidState("Waste pile positions must use non-negative indices.");
            }
            waste_numbers.push_back(waste->number);
        }
    }

    std::sort(stock_numbers.begin(), stock_numbers.end());
    std::sort(waste_numbers.begin(), waste_numbers.end());

    ValidateSequence(stock_numbers, "Stock positions must be contiguous from zero.");
    ValidateSequence(waste_numbers, "Waste pile positions must be contiguous from zero.");
}

void ValidateTableau(const Klondike::state_type& positions) {
    std::array<std::vector<Tableau>, kTableauColumnCount> columns;

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

void ValidateFoundation(const Klondike::state_type& positions) {
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

void ValidateState(const Klondike::state_type& positions) {
    ValidateDistinctStandardCards(positions);
    ValidateStockAndWaste(positions);
    ValidateTableau(positions);
    ValidateFoundation(positions);
}

bool CanMoveSourceToTableau(
    const Klondike::state_type& positions,
    const Card& card,
    const Column destination_column) {
    const auto& position = LookupPosition(positions, card);

    if (std::holds_alternative<Stock>(position)) {
        return false;
    }

    if (const auto* waste = std::get_if<WastePile>(&position); waste != nullptr) {
        return IsTopWasteCard(positions, card);
    }

    if (std::holds_alternative<Foundation>(position)) {
        return IsTopFoundationCard(positions, card);
    }

    if (const auto* tableau = std::get_if<Tableau>(&position); tableau != nullptr) {
        return tableau->column != destination_column
            && HasValidMovableTableauSequence(positions, card);
    }

    return false;
}

}  // namespace

Klondike::Klondike(state_type positions)
    : positions_(std::move(positions)) {
    ValidateState(positions_);
}

Klondike Klondike::deal(std::span<const card_type> deck) {
    if (deck.size() != kStandardDeckSize) {
        ThrowInvalidState("Klondike deals require exactly 52 cards.");
    }

    state_type positions;
    positions.reserve(deck.size());

    for (const auto& card : deck) {
        if (!card.is_standard_card()) {
            ThrowInvalidState("Klondike deals require standard cards only.");
        }
        positions.emplace(card, Stock{0});
    }

    if (positions.size() != deck.size()) {
        ThrowInvalidState("Klondike deals require 52 distinct cards.");
    }

    std::size_t deck_index = 0;

    for (int column_index = 0; column_index < kTableauColumnCount; ++column_index) {
        const auto column = static_cast<Column>(column_index);

        for (int number = 0; number < column_index; ++number) {
            positions[deck[deck_index++]] = Tableau{column, number, false};
        }

        positions[deck[deck_index++]] = Tableau{column, column_index, true};
    }

    int stock_number = 0;
    while (deck_index < deck.size()) {
        positions[deck[deck_index++]] = Stock{stock_number++};
    }

    return Klondike{std::move(positions)};
}

const Klondike::state_type& Klondike::card_positions() const noexcept {
    return positions_;
}

const Position& Klondike::position_of(const card_type& card) const {
    return LookupPosition(positions_, card);
}

bool Klondike::contains(const card_type& card) const noexcept {
    return positions_.contains(card);
}

std::size_t Klondike::stock_count() const noexcept {
    return std::count_if(
        positions_.begin(),
        positions_.end(),
        [](const auto& pair) {
            return std::holds_alternative<Stock>(pair.second);
        });
}

bool Klondike::is_win() const noexcept {
    return std::all_of(
        positions_.begin(),
        positions_.end(),
        [](const auto& pair) {
            return std::holds_alternative<Foundation>(pair.second);
        });
}

bool Klondike::is_pre_win() const noexcept {
    return std::all_of(
        positions_.begin(),
        positions_.end(),
        [](const auto& pair) {
            if (std::holds_alternative<Foundation>(pair.second)) {
                return true;
            }

            const auto* tableau = std::get_if<Tableau>(&pair.second);
            return tableau != nullptr && tableau->open;
        });
}

bool Klondike::can_open(const card_type& card) const {
    const auto* tableau = std::get_if<Tableau>(&position_of(card));
    return tableau != nullptr && !tableau->open && IsTopTableauCard(positions_, card);
}

Klondike Klondike::open(const card_type& card) const {
    if (!can_open(card)) {
        ThrowInvalidMove("Card cannot be opened.");
    }

    auto next_positions = positions_;
    const auto tableau = std::get<Tableau>(position_of(card));
    MutablePosition(next_positions, card) = Tableau{tableau.column, tableau.number, true};
    return Klondike{std::move(next_positions)};
}

bool Klondike::can_draw(const card_type& card) const {
    return IsTopStockCard(positions_, card);
}

Klondike Klondike::draw(const card_type& card) const {
    if (!can_draw(card)) {
        ThrowInvalidMove("Card cannot be drawn.");
    }

    auto next_positions = positions_;
    const auto waste_count = std::count_if(
        next_positions.begin(),
        next_positions.end(),
        [](const auto& pair) {
            return std::holds_alternative<WastePile>(pair.second);
        });

    MutablePosition(next_positions, card) = WastePile{static_cast<int>(waste_count)};
    return Klondike{std::move(next_positions)};
}

bool Klondike::can_move_to_foundation(const card_type& card) const {
    const auto& position = position_of(card);

    if (card.rank() != Rank::Ace) {
        const auto previous_card = PreviousRankCard(card);
        if (!previous_card.has_value()
            || !std::holds_alternative<Foundation>(position_of(*previous_card))) {
            return false;
        }
    }

    if (const auto* waste = std::get_if<WastePile>(&position); waste != nullptr) {
        return IsTopWasteCard(positions_, card);
    }

    if (const auto* tableau = std::get_if<Tableau>(&position); tableau != nullptr) {
        return tableau->open && IsTopTableauCard(positions_, card);
    }

    return false;
}

Klondike Klondike::move_to_foundation(const card_type& card) const {
    if (!can_move_to_foundation(card)) {
        ThrowInvalidMove("Card cannot be moved to the foundation.");
    }

    auto next_positions = positions_;
    MutablePosition(next_positions, card) = Foundation{};
    return Klondike{std::move(next_positions)};
}

bool Klondike::can_redeal() const noexcept {
    const auto has_stock = std::any_of(
        positions_.begin(),
        positions_.end(),
        [](const auto& pair) {
            return std::holds_alternative<Stock>(pair.second);
        });

    const auto has_waste = std::any_of(
        positions_.begin(),
        positions_.end(),
        [](const auto& pair) {
            return std::holds_alternative<WastePile>(pair.second);
        });

    return !has_stock && has_waste;
}

Klondike Klondike::redeal() const {
    if (!can_redeal()) {
        ThrowInvalidMove("Waste pile cannot be redealt.");
    }

    auto next_positions = positions_;
    std::vector<std::pair<int, Card>> waste_cards;
    waste_cards.reserve(next_positions.size());

    for (const auto& [card, position] : next_positions) {
        if (const auto* waste = std::get_if<WastePile>(&position); waste != nullptr) {
            waste_cards.emplace_back(waste->number, card);
        }
    }

    std::sort(
        waste_cards.begin(),
        waste_cards.end(),
        [](const auto& lhs, const auto& rhs) {
            return lhs.first > rhs.first;
        });

    int stock_number = 0;
    for (const auto& [waste_number, card] : waste_cards) {
        (void)waste_number;
        MutablePosition(next_positions, card) = Stock{stock_number++};
    }

    return Klondike{std::move(next_positions)};
}

bool Klondike::can_move_to_tableau(const card_type& card, const Column column) const {
    if (!CanMoveSourceToTableau(positions_, card, column)) {
        return false;
    }

    const auto destination_top = TableauTopCard(positions_, column);
    if (!destination_top.has_value()) {
        return card.rank() == Rank::King;
    }

    const auto* destination_top_tableau = std::get_if<Tableau>(&position_of(*destination_top));
    if (destination_top_tableau == nullptr || !destination_top_tableau->open) {
        return false;
    }

    return IsAlternatingDescending(*destination_top, card);
}

Klondike Klondike::move_to_tableau(const card_type& card, const Column column) const {
    if (!can_move_to_tableau(card, column)) {
        ThrowInvalidMove("Card cannot be moved to the tableau.");
    }

    auto next_positions = positions_;
    auto destination_count = static_cast<int>(TableauCount(next_positions, column));

    const auto& position = position_of(card);
    if (std::holds_alternative<Tableau>(position)) {
        const auto stack = OrderedTableauStack(next_positions, card);
        for (const auto& [unused_number, current_card] : stack) {
            (void)unused_number;
            MutablePosition(next_positions, current_card) = Tableau{column, destination_count++, true};
        }
    } else {
        MutablePosition(next_positions, card) = Tableau{column, destination_count, true};
    }

    return Klondike{std::move(next_positions)};
}

}  // namespace shlab::gopc::games::klondike
