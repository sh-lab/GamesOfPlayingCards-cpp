#include "shlab/gopc/games/canfield.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <unordered_set>
#include <utility>
#include <vector>

#include "shlab/gopc/playing_cards/rank.hpp"
#include "shlab/gopc/playing_cards/suit.hpp"

namespace shlab::gopc::games::canfield {
namespace {

using Card = shlab::gopc::playing_cards::Card;
using Rank = shlab::gopc::playing_cards::Rank;
using Suit = shlab::gopc::playing_cards::Suit;

constexpr std::size_t kStandardDeckSize = 52;
constexpr int kTableauColumnCount = 4;
constexpr int kReserveInitialCount = 13;
constexpr int kDrawCount = 3;
constexpr std::array kColumns{
    Column::First,
    Column::Second,
    Column::Third,
    Column::Fourth,
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

Rank AdvanceRank(const Rank rank, const int steps) noexcept {
    const auto zero_based = ToInt(rank) - 1;
    return static_cast<Rank>((zero_based + steps) % 13 + 1);
}

Rank PreviousRankWrap(const Rank rank) noexcept {
    return AdvanceRank(rank, 12);
}

bool CanBuildOnTableau(const Card& moving_card, const Card& destination_card) noexcept {
    return IsRed(moving_card.suit()) != IsRed(destination_card.suit())
        && AdvanceRank(moving_card.rank(), 1) == destination_card.rank();
}

const Position& LookupPosition(const Canfield::state_type& positions, const Card& card) {
    const auto it = positions.find(card);
    if (it == positions.end()) {
        throw std::out_of_range("Card is not part of this Canfield state.");
    }

    return it->second;
}

auto MutablePosition(Canfield::state_type& positions, const Card& card) -> Position& {
    const auto it = positions.find(card);
    if (it == positions.end()) {
        throw std::out_of_range("Card is not part of this Canfield state.");
    }

    return it->second;
}

std::optional<Card> TopReserveCard(const Canfield::state_type& positions) {
    std::optional<std::pair<int, Card>> result;
    for (const auto& [card, position] : positions) {
        if (const auto* reserve = std::get_if<Reserve>(&position); reserve != nullptr) {
            if (!result.has_value() || result->first < reserve->number) {
                result = std::pair{reserve->number, card};
            }
        }
    }

    if (!result.has_value()) {
        return std::nullopt;
    }

    return result->second;
}

std::optional<Card> TopWasteCard(const Canfield::state_type& positions) {
    std::optional<std::pair<int, Card>> result;
    for (const auto& [card, position] : positions) {
        if (const auto* waste = std::get_if<WastePile>(&position); waste != nullptr) {
            if (!result.has_value() || result->first < waste->number) {
                result = std::pair{waste->number, card};
            }
        }
    }

    if (!result.has_value()) {
        return std::nullopt;
    }

    return result->second;
}

std::vector<std::pair<int, Card>> TopStockCards(
    const Canfield::state_type& positions,
    const int draw_count) {
    std::vector<std::pair<int, Card>> stock_cards;
    for (const auto& [card, position] : positions) {
        if (const auto* stock = std::get_if<Stock>(&position); stock != nullptr) {
            stock_cards.emplace_back(stock->number, card);
        }
    }

    std::sort(
        stock_cards.begin(),
        stock_cards.end(),
        [](const auto& lhs, const auto& rhs) {
            return lhs.first > rhs.first;
        });

    if (stock_cards.size() > static_cast<std::size_t>(draw_count)) {
        stock_cards.erase(stock_cards.begin() + draw_count, stock_cards.end());
    }

    return stock_cards;
}

std::optional<Card> TableauTopCard(const Canfield::state_type& positions, const Column column) {
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

std::size_t TableauCount(const Canfield::state_type& positions, const Column column) noexcept {
    return std::count_if(
        positions.begin(),
        positions.end(),
        [column](const auto& pair) {
            const auto* tableau = std::get_if<Tableau>(&pair.second);
            return tableau != nullptr && tableau->column == column;
        });
}

std::vector<std::pair<int, Card>> OrderedTableauColumn(
    const Canfield::state_type& positions,
    const Column column) {
    std::vector<std::pair<int, Card>> stack;
    for (const auto& [card, position] : positions) {
        if (const auto* tableau = std::get_if<Tableau>(&position);
            tableau != nullptr && tableau->column == column) {
            stack.emplace_back(tableau->number, card);
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

bool HasValidWholeTableauSequence(
    const Canfield::state_type& positions,
    const Column column) {
    const auto stack = OrderedTableauColumn(positions, column);
    if (stack.empty()) {
        return false;
    }

    auto previous = stack.front().second;
    for (std::size_t i = 1; i < stack.size(); ++i) {
        const auto current = stack[i].second;
        if (!CanBuildOnTableau(current, previous)) {
            return false;
        }
        previous = current;
    }

    return true;
}

std::vector<std::pair<int, Card>> MovableTableauStack(
    const Canfield::state_type& positions,
    const Card& card) {
    const auto* tableau = std::get_if<Tableau>(&LookupPosition(positions, card));
    if (tableau == nullptr) {
        return {};
    }

    const auto top = TableauTopCard(positions, tableau->column);
    if (top.has_value() && *top == card) {
        return {{tableau->number, card}};
    }

    if (tableau->number == 0 && HasValidWholeTableauSequence(positions, tableau->column)) {
        return OrderedTableauColumn(positions, tableau->column);
    }

    return {};
}

std::optional<Rank> NextFoundationRankForSuit(
    const Canfield::state_type& positions,
    const Rank foundation_base_rank,
    const Suit suit) {
    std::size_t count = 0;
    for (const auto& [card, position] : positions) {
        if (card.suit() == suit && std::holds_alternative<Foundation>(position)) {
            ++count;
        }
    }

    if (count >= 13) {
        return std::nullopt;
    }

    return AdvanceRank(foundation_base_rank, static_cast<int>(count));
}

void AutoFillEmptyTableauFromReserve(
    Canfield::state_type& positions) {
    while (true) {
        const auto reserve_top = TopReserveCard(positions);
        if (!reserve_top.has_value()) {
            return;
        }

        const auto empty_column = std::find_if(
            kColumns.begin(),
            kColumns.end(),
            [&](const Column column) {
                return TableauCount(positions, column) == 0;
            });

        if (empty_column == kColumns.end()) {
            return;
        }

        MutablePosition(positions, *reserve_top) = Tableau{*empty_column, 0};
    }
}

void ValidateCardSet(const Canfield::state_type& positions) {
    if (positions.size() != kStandardDeckSize) {
        ThrowInvalidState("Canfield state must contain exactly 52 standard cards.");
    }

    for (const auto& [card, position] : positions) {
        (void)position;
        if (!card.is_standard_card()) {
            ThrowInvalidState("Canfield state cannot contain jokers.");
        }
    }
}

void ValidateNumberedPile(
    const std::vector<int>& numbers,
    const char* message) {
    for (std::size_t i = 0; i < numbers.size(); ++i) {
        if (numbers[i] != static_cast<int>(i)) {
            ThrowInvalidState(message);
        }
    }
}

void ValidateStockWasteReserve(const Canfield::state_type& positions) {
    std::vector<int> stock_numbers;
    std::vector<int> waste_numbers;
    std::vector<int> reserve_numbers;

    for (const auto& [card, position] : positions) {
        (void)card;

        if (const auto* stock = std::get_if<Stock>(&position); stock != nullptr) {
            if (stock->number < 0) {
                ThrowInvalidState("Stock positions must use non-negative indices.");
            }
            stock_numbers.push_back(stock->number);
        } else if (const auto* waste = std::get_if<WastePile>(&position); waste != nullptr) {
            if (waste->number < 0) {
                ThrowInvalidState("Waste positions must use non-negative indices.");
            }
            waste_numbers.push_back(waste->number);
        } else if (const auto* reserve = std::get_if<Reserve>(&position); reserve != nullptr) {
            if (reserve->number < 0) {
                ThrowInvalidState("Reserve positions must use non-negative indices.");
            }
            reserve_numbers.push_back(reserve->number);
        }
    }

    std::sort(stock_numbers.begin(), stock_numbers.end());
    std::sort(waste_numbers.begin(), waste_numbers.end());
    std::sort(reserve_numbers.begin(), reserve_numbers.end());

    ValidateNumberedPile(stock_numbers, "Stock positions must be contiguous from zero.");
    ValidateNumberedPile(waste_numbers, "Waste positions must be contiguous from zero.");
    ValidateNumberedPile(reserve_numbers, "Reserve positions must be contiguous from zero.");
}

void ValidateTableau(
    const Canfield::state_type& positions,
    const bool reserve_must_fill_empty) {
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
        ValidateNumberedPile(column, "Tableau positions must be contiguous within each column.");

        if (reserve_must_fill_empty && column.empty()) {
            ThrowInvalidState("Empty tableau columns must be filled from reserve while reserve remains.");
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

void ValidateFoundation(
    const Canfield::state_type& positions,
    const Rank foundation_base_rank) {
    std::array<std::vector<Rank>, 4> ranks_by_suit;

    for (const auto& [card, position] : positions) {
        if (std::holds_alternative<Foundation>(position)) {
            ranks_by_suit[SuitIndex(card.suit())].push_back(card.rank());
        }
    }

    for (auto& ranks : ranks_by_suit) {
        std::sort(
            ranks.begin(),
            ranks.end(),
            [&](const Rank lhs, const Rank rhs) {
                auto lhs_index = 0;
                auto rhs_index = 0;
                while (AdvanceRank(foundation_base_rank, lhs_index) != lhs) {
                    ++lhs_index;
                }
                while (AdvanceRank(foundation_base_rank, rhs_index) != rhs) {
                    ++rhs_index;
                }
                return lhs_index < rhs_index;
            });

        for (std::size_t i = 0; i < ranks.size(); ++i) {
            if (ranks[i] != AdvanceRank(foundation_base_rank, static_cast<int>(i))) {
                ThrowInvalidState("Foundation cards must follow the Canfield base-rank sequence.");
            }
        }
    }
}

void ValidateState(
    const Canfield::state_type& positions,
    const Rank foundation_base_rank) {
    if (foundation_base_rank == Rank::None) {
        ThrowInvalidState("Canfield requires a valid foundation base rank.");
    }

    ValidateCardSet(positions);
    ValidateStockWasteReserve(positions);
    ValidateTableau(positions, TopReserveCard(positions).has_value());
    ValidateFoundation(positions, foundation_base_rank);
}

}  // namespace

Canfield::Canfield(state_type positions, const rank_type foundation_base_rank)
    : positions_(std::move(positions))
    , foundation_base_rank_(foundation_base_rank) {
    ValidateState(positions_, foundation_base_rank_);
}

Canfield Canfield::deal(std::span<const card_type> deck) {
    if (deck.size() != kStandardDeckSize) {
        ThrowInvalidState("Canfield deals require exactly 52 cards.");
    }

    state_type positions;
    positions.reserve(deck.size());
    std::unordered_set<Card> seen;
    seen.reserve(deck.size());

    for (const auto& card : deck) {
        if (!card.is_standard_card()) {
            ThrowInvalidState("Canfield deals require standard cards only.");
        }
        if (!seen.insert(card).second) {
            ThrowInvalidState("Canfield deals require 52 distinct cards.");
        }
    }

    std::size_t deck_index = 0;
    for (int i = 0; i < kReserveInitialCount; ++i) {
        positions.emplace(deck[deck_index++], Reserve{i});
    }

    const auto foundation_base_card = deck[deck_index++];
    positions.emplace(foundation_base_card, Foundation{});

    for (int column_index = 0; column_index < kTableauColumnCount; ++column_index) {
        positions.emplace(deck[deck_index++], Tableau{static_cast<Column>(column_index), 0});
    }

    int stock_number = 0;
    while (deck_index < deck.size()) {
        positions.emplace(deck[deck_index++], Stock{stock_number++});
    }

    return Canfield{std::move(positions), foundation_base_card.rank()};
}

const Canfield::state_type& Canfield::card_positions() const noexcept {
    return positions_;
}

const Position& Canfield::position_of(const card_type& card) const {
    return LookupPosition(positions_, card);
}

bool Canfield::contains(const card_type& card) const noexcept {
    return positions_.contains(card);
}

Canfield::rank_type Canfield::foundation_base_rank() const noexcept {
    return foundation_base_rank_;
}

std::size_t Canfield::stock_count() const noexcept {
    return std::count_if(
        positions_.begin(),
        positions_.end(),
        [](const auto& pair) {
            return std::holds_alternative<Stock>(pair.second);
        });
}

std::size_t Canfield::waste_count() const noexcept {
    return std::count_if(
        positions_.begin(),
        positions_.end(),
        [](const auto& pair) {
            return std::holds_alternative<WastePile>(pair.second);
        });
}

std::size_t Canfield::reserve_count() const noexcept {
    return std::count_if(
        positions_.begin(),
        positions_.end(),
        [](const auto& pair) {
            return std::holds_alternative<Reserve>(pair.second);
        });
}

bool Canfield::is_win() const noexcept {
    return std::all_of(
        positions_.begin(),
        positions_.end(),
        [](const auto& pair) {
            return std::holds_alternative<Foundation>(pair.second);
        });
}

bool Canfield::can_draw() const noexcept {
    return stock_count() != 0;
}

Canfield Canfield::draw() const {
    if (!can_draw()) {
        ThrowInvalidMove("Stock cannot be drawn.");
    }

    auto next_positions = positions_;
    const auto cards_to_draw = TopStockCards(next_positions, kDrawCount);
    auto next_waste_number = static_cast<int>(waste_count());

    for (const auto& [unused_stock_number, card] : cards_to_draw) {
        (void)unused_stock_number;
        MutablePosition(next_positions, card) = WastePile{next_waste_number++};
    }

    return Canfield{std::move(next_positions), foundation_base_rank_};
}

bool Canfield::can_redeal() const noexcept {
    return stock_count() == 0 && waste_count() != 0;
}

Canfield Canfield::redeal() const {
    if (!can_redeal()) {
        ThrowInvalidMove("Waste cannot be redealt.");
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
    for (const auto& [unused_waste_number, card] : waste_cards) {
        (void)unused_waste_number;
        MutablePosition(next_positions, card) = Stock{stock_number++};
    }

    return Canfield{std::move(next_positions), foundation_base_rank_};
}

bool Canfield::can_move_to_foundation(const card_type& card) const {
    const auto reserve_top = TopReserveCard(positions_);
    const auto waste_top = TopWasteCard(positions_);

    if (reserve_top.has_value() && *reserve_top == card) {
        const auto next_rank = NextFoundationRankForSuit(positions_, foundation_base_rank_, card.suit());
        return next_rank.has_value() && *next_rank == card.rank();
    }

    if (waste_top.has_value() && *waste_top == card) {
        const auto next_rank = NextFoundationRankForSuit(positions_, foundation_base_rank_, card.suit());
        return next_rank.has_value() && *next_rank == card.rank();
    }

    const auto* tableau = std::get_if<Tableau>(&position_of(card));
    if (tableau != nullptr) {
        const auto top = TableauTopCard(positions_, tableau->column);
        const auto next_rank = NextFoundationRankForSuit(positions_, foundation_base_rank_, card.suit());
        return top.has_value() && *top == card && next_rank.has_value() && *next_rank == card.rank();
    }

    return false;
}

Canfield Canfield::move_to_foundation(const card_type& card) const {
    if (!can_move_to_foundation(card)) {
        ThrowInvalidMove("Card cannot be moved to the foundation.");
    }

    auto next_positions = positions_;
    const auto source_position = position_of(card);
    MutablePosition(next_positions, card) = Foundation{};

    if (std::holds_alternative<Tableau>(source_position)) {
        AutoFillEmptyTableauFromReserve(next_positions);
    }

    return Canfield{std::move(next_positions), foundation_base_rank_};
}

bool Canfield::can_move_to_tableau(const card_type& card, const Column column) const {
    const auto reserve_top = TopReserveCard(positions_);
    const auto waste_top = TopWasteCard(positions_);

    auto is_source_allowed = false;
    if (reserve_top.has_value() && *reserve_top == card) {
        is_source_allowed = true;
    } else if (waste_top.has_value() && *waste_top == card) {
        is_source_allowed = true;
    } else {
        is_source_allowed = !MovableTableauStack(positions_, card).empty();
    }

    if (!is_source_allowed) {
        return false;
    }

    if (const auto* source_tableau = std::get_if<Tableau>(&position_of(card));
        source_tableau != nullptr && source_tableau->column == column) {
        return false;
    }

    const auto destination_top = TableauTopCard(positions_, column);
    if (!destination_top.has_value()) {
        return !reserve_top.has_value() || *reserve_top == card;
    }

    return CanBuildOnTableau(card, *destination_top);
}

Canfield Canfield::move_to_tableau(const card_type& card, const Column column) const {
    if (!can_move_to_tableau(card, column)) {
        ThrowInvalidMove("Card cannot be moved to the tableau.");
    }

    auto next_positions = positions_;
    auto destination_count = static_cast<int>(TableauCount(next_positions, column));

    if (const auto reserve_top = TopReserveCard(next_positions);
        reserve_top.has_value() && *reserve_top == card) {
        MutablePosition(next_positions, card) = Tableau{column, destination_count};
    } else if (const auto waste_top = TopWasteCard(next_positions);
        waste_top.has_value() && *waste_top == card) {
        MutablePosition(next_positions, card) = Tableau{column, destination_count};
    } else {
        const auto stack = MovableTableauStack(next_positions, card);
        for (const auto& [unused_number, current_card] : stack) {
            (void)unused_number;
            MutablePosition(next_positions, current_card) = Tableau{column, destination_count++};
        }
        AutoFillEmptyTableauFromReserve(next_positions);
        return Canfield{std::move(next_positions), foundation_base_rank_};
    }

    return Canfield{std::move(next_positions), foundation_base_rank_};
}

}  // namespace shlab::gopc::games::canfield
