#include "shlab/gopc/games/calculation.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <unordered_set>
#include <utility>
#include <vector>

#include "shlab/gopc/playing_cards/rank.hpp"

namespace shlab::gopc::games::calculation {
namespace {

using Card = shlab::gopc::playing_cards::Card;
using Rank = shlab::gopc::playing_cards::Rank;

constexpr std::size_t kStandardDeckSize = 52;
constexpr int kFoundationColumnCount = 4;
constexpr int kTableauColumnCount = 4;
constexpr std::array kFoundationColumns{
    FoundationColumn::First,
    FoundationColumn::Second,
    FoundationColumn::Third,
    FoundationColumn::Fourth,
};
constexpr std::array kTableauColumns{
    TableauColumn::First,
    TableauColumn::Second,
    TableauColumn::Third,
    TableauColumn::Fourth,
};
constexpr std::array<std::array<Rank, 13>, kFoundationColumnCount> kFoundationSequences{{
    {Rank::Ace, Rank::Two, Rank::Three, Rank::Four, Rank::Five, Rank::Six, Rank::Seven,
     Rank::Eight, Rank::Nine, Rank::Ten, Rank::Jack, Rank::Queen, Rank::King},
    {Rank::Two, Rank::Four, Rank::Six, Rank::Eight, Rank::Ten, Rank::Queen, Rank::Ace,
     Rank::Three, Rank::Five, Rank::Seven, Rank::Nine, Rank::Jack, Rank::King},
    {Rank::Three, Rank::Six, Rank::Nine, Rank::Queen, Rank::Two, Rank::Five, Rank::Eight,
     Rank::Jack, Rank::Ace, Rank::Four, Rank::Seven, Rank::Ten, Rank::King},
    {Rank::Four, Rank::Eight, Rank::Queen, Rank::Three, Rank::Seven, Rank::Jack, Rank::Two,
     Rank::Six, Rank::Ten, Rank::Ace, Rank::Five, Rank::Nine, Rank::King},
}};

[[noreturn]] void ThrowInvalidState(const char* message) {
    throw std::invalid_argument(message);
}

[[noreturn]] void ThrowInvalidMove(const char* message) {
    throw std::logic_error(message);
}

int ToInt(const FoundationColumn column) noexcept {
    return static_cast<int>(column);
}

int ToInt(const TableauColumn column) noexcept {
    return static_cast<int>(column);
}

const Position& LookupPosition(const Calculation::state_type& positions, const Card& card) {
    const auto it = positions.find(card);
    if (it == positions.end()) {
        throw std::out_of_range("Card is not part of this Calculation state.");
    }

    return it->second;
}

auto MutablePosition(Calculation::state_type& positions, const Card& card) -> Position& {
    const auto it = positions.find(card);
    if (it == positions.end()) {
        throw std::out_of_range("Card is not part of this Calculation state.");
    }

    return it->second;
}

std::size_t FoundationCount(
    const Calculation::state_type& positions,
    const FoundationColumn column) noexcept {
    return std::count_if(
        positions.begin(),
        positions.end(),
        [column](const auto& pair) {
            const auto* foundation = std::get_if<Foundation>(&pair.second);
            return foundation != nullptr && foundation->column == column;
        });
}

std::optional<Rank> NextRank(
    const Calculation::state_type& positions,
    const FoundationColumn column) noexcept {
    const auto count = FoundationCount(positions, column);
    const auto& sequence = kFoundationSequences[static_cast<std::size_t>(ToInt(column))];
    if (count >= sequence.size()) {
        return std::nullopt;
    }

    return sequence[count];
}

bool HasWastePile(const Calculation::state_type& positions) noexcept {
    return std::any_of(
        positions.begin(),
        positions.end(),
        [](const auto& pair) {
            return std::holds_alternative<WastePile>(pair.second);
        });
}

bool IsTopStockCard(const Calculation::state_type& positions, const Card& card) {
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

bool IsTopTableauCard(const Calculation::state_type& positions, const Card& card) {
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

void ValidateCardSet(const Calculation::state_type& positions) {
    if (positions.size() != kStandardDeckSize) {
        ThrowInvalidState("Calculation state must contain exactly 52 standard cards.");
    }

    for (const auto& [card, position] : positions) {
        (void)position;

        if (!card.is_standard_card()) {
            ThrowInvalidState("Calculation state cannot contain jokers.");
        }
    }
}

void ValidateStock(const Calculation::state_type& positions) {
    std::vector<int> stock_numbers;

    for (const auto& [card, position] : positions) {
        (void)card;

        if (const auto* stock = std::get_if<Stock>(&position); stock != nullptr) {
            if (stock->number < 0) {
                ThrowInvalidState("Stock positions must use non-negative indices.");
            }
            stock_numbers.push_back(stock->number);
        }
    }

    std::sort(stock_numbers.begin(), stock_numbers.end());
    for (std::size_t i = 0; i < stock_numbers.size(); ++i) {
        if (stock_numbers[i] != static_cast<int>(i)) {
            ThrowInvalidState("Stock positions must be contiguous from zero.");
        }
    }
}

void ValidateWaste(const Calculation::state_type& positions) {
    const auto waste_count = std::count_if(
        positions.begin(),
        positions.end(),
        [](const auto& pair) {
            return std::holds_alternative<WastePile>(pair.second);
        });

    if (waste_count > 1) {
        ThrowInvalidState("Calculation state can contain at most one waste card.");
    }
}

void ValidateTableau(const Calculation::state_type& positions) {
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

void ValidateFoundation(const Calculation::state_type& positions) {
    std::array<std::vector<Card>, kFoundationColumnCount> foundations;

    for (const auto& [card, position] : positions) {
        if (const auto* foundation = std::get_if<Foundation>(&position); foundation != nullptr) {
            const auto column_index = ToInt(foundation->column);
            if (column_index < 0 || column_index >= kFoundationColumnCount) {
                ThrowInvalidState("Foundation column is out of range.");
            }

            foundations[static_cast<std::size_t>(column_index)].push_back(card);
        }
    }

    for (std::size_t i = 0; i < foundations.size(); ++i) {
        auto& cards = foundations[i];
        std::sort(
            cards.begin(),
            cards.end(),
            [&](const Card& lhs, const Card& rhs) {
                const auto& sequence = kFoundationSequences[i];
                const auto lhs_it = std::find(sequence.begin(), sequence.end(), lhs.rank());
                const auto rhs_it = std::find(sequence.begin(), sequence.end(), rhs.rank());
                return lhs_it < rhs_it;
            });

        const auto& sequence = kFoundationSequences[i];
        for (std::size_t index = 0; index < cards.size(); ++index) {
            if (cards[index].rank() != sequence[index]) {
                ThrowInvalidState(
                    "Foundation cards must follow the configured Calculation sequence.");
            }
        }
    }
}

void ValidateState(const Calculation::state_type& positions) {
    ValidateCardSet(positions);
    ValidateStock(positions);
    ValidateWaste(positions);
    ValidateTableau(positions);
    ValidateFoundation(positions);
}

}  // namespace

Calculation::Calculation(state_type positions)
    : positions_(std::move(positions)) {
    ValidateState(positions_);
}

Calculation Calculation::deal(std::span<const card_type> deck) {
    if (deck.size() != kStandardDeckSize) {
        ThrowInvalidState("Calculation deals require exactly 52 cards.");
    }

    state_type positions;
    positions.reserve(deck.size());

    std::unordered_set<Card> seen;
    seen.reserve(deck.size());

    std::vector<Card> cards;
    cards.reserve(deck.size());
    for (const auto& card : deck) {
        if (!card.is_standard_card()) {
            ThrowInvalidState("Calculation deals require standard cards only.");
        }
        if (!seen.insert(card).second) {
            ThrowInvalidState("Calculation deals require 52 distinct cards.");
        }
        cards.push_back(card);
    }

    const auto take_first_rank = [&](const Rank rank) {
        const auto it = std::find_if(
            cards.begin(),
            cards.end(),
            [rank](const Card& card) {
                return card.rank() == rank;
            });

        if (it == cards.end()) {
            ThrowInvalidState("Calculation deal requires the base foundation ranks.");
        }

        const auto card = *it;
        cards.erase(it);
        return card;
    };

    positions.emplace(take_first_rank(Rank::Ace), Foundation{FoundationColumn::First});
    positions.emplace(take_first_rank(Rank::Two), Foundation{FoundationColumn::Second});
    positions.emplace(take_first_rank(Rank::Three), Foundation{FoundationColumn::Third});
    positions.emplace(take_first_rank(Rank::Four), Foundation{FoundationColumn::Fourth});

    positions.emplace(cards.front(), WastePile{});
    cards.erase(cards.begin());

    for (std::size_t i = 0; i < cards.size(); ++i) {
        positions.emplace(cards[i], Stock{static_cast<int>(i)});
    }

    return Calculation{std::move(positions)};
}

const Calculation::state_type& Calculation::card_positions() const noexcept {
    return positions_;
}

const Position& Calculation::position_of(const card_type& card) const {
    return LookupPosition(positions_, card);
}

bool Calculation::contains(const card_type& card) const noexcept {
    return positions_.contains(card);
}

std::optional<Calculation::rank_type> Calculation::next_rank(const FoundationColumn column) const noexcept {
    return NextRank(positions_, column);
}

std::size_t Calculation::stock_count() const noexcept {
    return std::count_if(
        positions_.begin(),
        positions_.end(),
        [](const auto& pair) {
            return std::holds_alternative<Stock>(pair.second);
        });
}

bool Calculation::is_win() const noexcept {
    return std::all_of(
        positions_.begin(),
        positions_.end(),
        [](const auto& pair) {
            return std::holds_alternative<Foundation>(pair.second);
        });
}

bool Calculation::can_draw(const card_type& card) const {
    return !HasWastePile(positions_) && IsTopStockCard(positions_, card);
}

Calculation Calculation::draw(const card_type& card) const {
    if (!can_draw(card)) {
        ThrowInvalidMove("Card cannot be drawn from the stock.");
    }

    auto next_positions = positions_;
    MutablePosition(next_positions, card) = WastePile{};
    return Calculation{std::move(next_positions)};
}

bool Calculation::can_move_to_tableau(const card_type& card) const {
    return std::holds_alternative<WastePile>(position_of(card));
}

Calculation Calculation::move_to_tableau(const card_type& card, const TableauColumn column) const {
    if (!can_move_to_tableau(card)) {
        ThrowInvalidMove("Card cannot be moved to the tableau.");
    }

    auto next_positions = positions_;
    const auto destination_count = static_cast<int>(std::count_if(
        next_positions.begin(),
        next_positions.end(),
        [column](const auto& pair) {
            const auto* tableau = std::get_if<Tableau>(&pair.second);
            return tableau != nullptr && tableau->column == column;
        }));
    MutablePosition(next_positions, card) = Tableau{column, destination_count};
    return Calculation{std::move(next_positions)};
}

bool Calculation::can_move_to_foundation(const card_type& card, const FoundationColumn column) const {
    const auto& position = position_of(card);
    if (std::holds_alternative<WastePile>(position)) {
        const auto next = next_rank(column);
        return next.has_value() && card.rank() == *next;
    }

    if (std::holds_alternative<Tableau>(position) && IsTopTableauCard(positions_, card)) {
        const auto next = next_rank(column);
        return next.has_value() && card.rank() == *next;
    }

    return false;
}

Calculation Calculation::move_to_foundation(const card_type& card, const FoundationColumn column) const {
    if (!can_move_to_foundation(card, column)) {
        ThrowInvalidMove("Card cannot be moved to the foundation.");
    }

    auto next_positions = positions_;
    MutablePosition(next_positions, card) = Foundation{column};
    return Calculation{std::move(next_positions)};
}

}  // namespace shlab::gopc::games::calculation
