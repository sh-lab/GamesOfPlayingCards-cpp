#include "shlab/gopc/games/canfield.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <utility>

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

[[noreturn]] void ThrowInvalidState(const char* message);
[[noreturn]] void ThrowInvalidMove(const char* message);

struct OrderedCardList {
    std::array<std::optional<Card>, kStandardDeckSize> cards{};
    std::size_t size{};

    [[nodiscard]] bool empty() const noexcept {
        return size == 0;
    }
};

struct DrawCards {
    std::array<std::optional<Card>, kDrawCount> cards{};
    std::array<int, kDrawCount> numbers{{
        -1,
        -1,
        -1,
    }};
    std::size_t size{};
};

struct ContiguousNumbers {
    std::array<bool, kStandardDeckSize> occupied{};
    std::size_t count{};

    void add(const int number, const char* negative_message) {
        if (number < 0) {
            ThrowInvalidState(negative_message);
        }

        const auto index = static_cast<std::size_t>(number);
        if (index >= occupied.size()) {
            ThrowInvalidState(negative_message);
        }

        occupied[index] = true;
        ++count;
    }

    void validate(const char* message) const {
        for (std::size_t i = 0; i < count; ++i) {
            if (!occupied[i]) {
                ThrowInvalidState(message);
            }
        }
    }
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
        if (const auto* reserve = GetIf<Reserve>(&position); reserve != nullptr) {
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
        if (const auto* waste = GetIf<WastePile>(&position); waste != nullptr) {
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

DrawCards TopStockCards(
    const Canfield::state_type& positions,
    const int draw_count) {
    DrawCards stock_cards;
    for (const auto& [card, position] : positions) {
        if (const auto* stock = GetIf<Stock>(&position); stock != nullptr) {
            auto insert_index = stock_cards.size;
            while (insert_index > 0 && stock_cards.numbers[insert_index - 1] < stock->number) {
                if (insert_index < static_cast<std::size_t>(draw_count)) {
                    stock_cards.numbers[insert_index] = stock_cards.numbers[insert_index - 1];
                    stock_cards.cards[insert_index] = stock_cards.cards[insert_index - 1];
                }
                --insert_index;
            }

            if (insert_index >= static_cast<std::size_t>(draw_count)) {
                continue;
            }

            stock_cards.numbers[insert_index] = stock->number;
            stock_cards.cards[insert_index] = card;
            if (stock_cards.size < static_cast<std::size_t>(draw_count)) {
                ++stock_cards.size;
            }
        }
    }

    return stock_cards;
}

std::optional<Card> TableauTopCard(const Canfield::state_type& positions, const Column column) {
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

std::size_t TableauCount(const Canfield::state_type& positions, const Column column) noexcept {
    return std::count_if(
        positions.begin(),
        positions.end(),
        [column](const auto& pair) {
            const auto* tableau = GetIf<Tableau>(&pair.second);
            return tableau != nullptr && tableau->column == column;
        });
}

OrderedCardList OrderedTableauColumn(
    const Canfield::state_type& positions,
    const Column column) {
    OrderedCardList stack;
    for (const auto& [card, position] : positions) {
        if (const auto* tableau = GetIf<Tableau>(&position);
            tableau != nullptr && tableau->column == column) {
            const auto index = static_cast<std::size_t>(tableau->number);
            stack.cards[index] = card;
            if (stack.size <= index) {
                stack.size = index + 1;
            }
        }
    }

    return stack;
}

bool HasValidWholeTableauSequence(
    const Canfield::state_type& positions,
    const Column column) {
    const auto stack = OrderedTableauColumn(positions, column);
    if (stack.empty()) {
        return false;
    }

    auto previous = *stack.cards[0];
    for (std::size_t i = 1; i < stack.size; ++i) {
        const auto current = *stack.cards[i];
        if (!CanBuildOnTableau(current, previous)) {
            return false;
        }
        previous = current;
    }

    return true;
}

OrderedCardList MovableTableauStack(
    const Canfield::state_type& positions,
    const Card& card) {
    const auto* tableau = GetIf<Tableau>(&LookupPosition(positions, card));
    if (tableau == nullptr) {
        return {};
    }

    const auto top = TableauTopCard(positions, tableau->column);
    if (top.has_value() && *top == card) {
        OrderedCardList stack;
        stack.cards[0] = card;
        stack.size = 1;
        return stack;
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
        if (card.suit() == suit && HoldsAlternative<Foundation>(position)) {
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

void ValidateStockWasteReserve(const Canfield::state_type& positions) {
    ContiguousNumbers stock_numbers;
    ContiguousNumbers waste_numbers;
    ContiguousNumbers reserve_numbers;

    for (const auto& [card, position] : positions) {
        (void)card;

        if (const auto* stock = GetIf<Stock>(&position); stock != nullptr) {
            stock_numbers.add(stock->number, "Stock positions must use non-negative indices.");
        } else if (const auto* waste = GetIf<WastePile>(&position); waste != nullptr) {
            waste_numbers.add(waste->number, "Waste positions must use non-negative indices.");
        } else if (const auto* reserve = GetIf<Reserve>(&position); reserve != nullptr) {
            reserve_numbers.add(reserve->number, "Reserve positions must use non-negative indices.");
        }
    }

    stock_numbers.validate("Stock positions must be contiguous from zero.");
    waste_numbers.validate("Waste positions must be contiguous from zero.");
    reserve_numbers.validate("Reserve positions must be contiguous from zero.");
}

void ValidateTableau(
    const Canfield::state_type& positions,
    const bool reserve_must_fill_empty) {
    std::array<ContiguousNumbers, kTableauColumnCount> columns;

    for (const auto& [card, position] : positions) {
        (void)card;

        if (const auto* tableau = GetIf<Tableau>(&position); tableau != nullptr) {
            const auto column_index = ToInt(tableau->column);
            if (column_index < 0 || column_index >= kTableauColumnCount) {
                ThrowInvalidState("Tableau column is out of range.");
            }

            columns[static_cast<std::size_t>(column_index)].add(
                tableau->number,
                "Tableau positions must use non-negative indices.");
        }
    }

    for (auto& column : columns) {
        column.validate("Tableau positions must be contiguous within each column.");

        if (reserve_must_fill_empty && column.count == 0) {
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
    std::array<std::array<bool, 13>, 4> ranks_by_suit{};
    std::array<std::size_t, 4> suit_counts{};

    for (const auto& [card, position] : positions) {
        if (HoldsAlternative<Foundation>(position)) {
            auto rank_index = 0;
            while (AdvanceRank(foundation_base_rank, rank_index) != card.rank()) {
                ++rank_index;
            }

            ranks_by_suit[SuitIndex(card.suit())][static_cast<std::size_t>(rank_index)] = true;
            ++suit_counts[SuitIndex(card.suit())];
        }
    }

    for (std::size_t suit_index = 0; suit_index < ranks_by_suit.size(); ++suit_index) {
        for (std::size_t i = 0; i < suit_counts[suit_index]; ++i) {
            if (!ranks_by_suit[suit_index][i]) {
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
    std::array<bool, 256> seen{};

    for (const auto& card : deck) {
        if (!card.is_standard_card()) {
            ThrowInvalidState("Canfield deals require standard cards only.");
        }

        const auto index = static_cast<std::uint8_t>(card.id());
        if (seen[index]) {
            ThrowInvalidState("Canfield deals require 52 distinct cards.");
        }
        seen[index] = true;
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
            return HoldsAlternative<Stock>(pair.second);
        });
}

std::size_t Canfield::waste_count() const noexcept {
    return std::count_if(
        positions_.begin(),
        positions_.end(),
        [](const auto& pair) {
            return HoldsAlternative<WastePile>(pair.second);
        });
}

std::size_t Canfield::reserve_count() const noexcept {
    return std::count_if(
        positions_.begin(),
        positions_.end(),
        [](const auto& pair) {
            return HoldsAlternative<Reserve>(pair.second);
        });
}

bool Canfield::is_win() const noexcept {
    return std::all_of(
        positions_.begin(),
        positions_.end(),
        [](const auto& pair) {
            return HoldsAlternative<Foundation>(pair.second);
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

    for (std::size_t i = 0; i < cards_to_draw.size; ++i) {
        MutablePosition(next_positions, *cards_to_draw.cards[i]) = WastePile{next_waste_number++};
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
    std::array<std::optional<Card>, kStandardDeckSize> waste_cards{};
    std::size_t waste_card_count = 0;

    for (const auto& [card, position] : next_positions) {
        if (const auto* waste = GetIf<WastePile>(&position); waste != nullptr) {
            const auto index = static_cast<std::size_t>(waste->number);
            waste_cards[index] = card;
            if (waste_card_count <= index) {
                waste_card_count = index + 1;
            }
        }
    }

    int stock_number = 0;
    for (std::size_t i = waste_card_count; i > 0; --i) {
        if (!waste_cards[i - 1].has_value()) {
            continue;
        }

        MutablePosition(next_positions, *waste_cards[i - 1]) = Stock{stock_number++};
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

    const auto* tableau = GetIf<Tableau>(&position_of(card));
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

    if (HoldsAlternative<Tableau>(source_position)) {
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

    if (const auto* source_tableau = GetIf<Tableau>(&position_of(card));
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
        for (std::size_t i = 0; i < stack.size; ++i) {
            MutablePosition(next_positions, *stack.cards[i]) = Tableau{column, destination_count++};
        }
        AutoFillEmptyTableauFromReserve(next_positions);
        return Canfield{std::move(next_positions), foundation_base_rank_};
    }

    return Canfield{std::move(next_positions), foundation_base_rank_};
}

}  // namespace shlab::gopc::games::canfield
