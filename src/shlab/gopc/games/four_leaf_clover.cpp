#include "shlab/gopc/games/four_leaf_clover.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <unordered_set>
#include <utility>
#include <vector>

#include "shlab/gopc/playing_cards/rank.hpp"

namespace shlab::gopc::games::four_leaf_clover {
namespace {

using Card = shlab::gopc::playing_cards::Card;
using Rank = shlab::gopc::playing_cards::Rank;

constexpr std::size_t kDeckSize = 48;
constexpr int kFieldSlotCount = 16;

[[noreturn]] void ThrowInvalidState(const char* message) {
    throw std::invalid_argument(message);
}

[[noreturn]] void ThrowInvalidMove(const char* message) {
    throw std::logic_error(message);
}

const Position& LookupPosition(const FourLeafClover::state_type& positions, const Card& card) {
    const auto it = positions.find(card);
    if (it == positions.end()) {
        throw std::out_of_range("Card is not part of this FourLeafClover state.");
    }

    return it->second;
}

auto MutablePosition(FourLeafClover::state_type& positions, const Card& card) -> Position& {
    const auto it = positions.find(card);
    if (it == positions.end()) {
        throw std::out_of_range("Card is not part of this FourLeafClover state.");
    }

    return it->second;
}

bool IsSupportedCard(const Card& card) noexcept {
    return card.is_standard_card() && card.rank() != Rank::Ten;
}

std::optional<int> NumericValue(const Card& card) noexcept {
    switch (card.rank()) {
    case Rank::Ace:
        return 1;
    case Rank::Two:
        return 2;
    case Rank::Three:
        return 3;
    case Rank::Four:
        return 4;
    case Rank::Five:
        return 5;
    case Rank::Six:
        return 6;
    case Rank::Seven:
        return 7;
    case Rank::Eight:
        return 8;
    case Rank::Nine:
        return 9;
    default:
        return std::nullopt;
    }
}

bool IsJackQueenKingSet(std::span<const Card> cards) noexcept {
    if (cards.size() != 3) {
        return false;
    }

    bool has_jack = false;
    bool has_queen = false;
    bool has_king = false;

    for (const auto& card : cards) {
        switch (card.rank()) {
        case Rank::Jack:
            has_jack = true;
            break;
        case Rank::Queen:
            has_queen = true;
            break;
        case Rank::King:
            has_king = true;
            break;
        default:
            return false;
        }
    }

    return has_jack && has_queen && has_king;
}

bool IsFieldCard(const FourLeafClover::state_type& positions, const Card& card) {
    return std::holds_alternative<Field>(LookupPosition(positions, card));
}

void ValidateCardSet(const FourLeafClover::state_type& positions) {
    if (positions.size() != kDeckSize) {
        ThrowInvalidState("Four Leaf Clover state must contain exactly 48 cards.");
    }

    for (const auto& [card, position] : positions) {
        (void)position;
        if (!IsSupportedCard(card)) {
            ThrowInvalidState("Four Leaf Clover state must contain only standard cards except tens.");
        }
    }
}

void ValidateField(const FourLeafClover::state_type& positions) {
    std::array<bool, kFieldSlotCount> occupied{};

    for (const auto& [card, position] : positions) {
        (void)card;

        if (const auto* field = std::get_if<Field>(&position); field != nullptr) {
            if (field->number < 0 || field->number >= kFieldSlotCount) {
                ThrowInvalidState("Field positions must be in the range [0, 15].");
            }

            auto& slot = occupied[static_cast<std::size_t>(field->number)];
            if (slot) {
                ThrowInvalidState("Field positions must be unique.");
            }
            slot = true;
        }
    }
}

void ValidateStock(const FourLeafClover::state_type& positions) {
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

void ValidateState(const FourLeafClover::state_type& positions) {
    ValidateCardSet(positions);
    ValidateField(positions);
    ValidateStock(positions);
}

std::vector<std::pair<int, Card>> OrderedStock(const FourLeafClover::state_type& positions) {
    std::vector<std::pair<int, Card>> stock_cards;
    stock_cards.reserve(positions.size());

    for (const auto& [card, position] : positions) {
        if (const auto* stock = std::get_if<Stock>(&position); stock != nullptr) {
            stock_cards.emplace_back(stock->number, card);
        }
    }

    std::sort(
        stock_cards.begin(),
        stock_cards.end(),
        [](const auto& lhs, const auto& rhs) {
            return lhs.first < rhs.first;
        });

    return stock_cards;
}

std::vector<int> EmptyFieldSlots(const FourLeafClover::state_type& positions) {
    std::array<bool, kFieldSlotCount> occupied{};

    for (const auto& [card, position] : positions) {
        (void)card;

        if (const auto* field = std::get_if<Field>(&position); field != nullptr) {
            occupied[static_cast<std::size_t>(field->number)] = true;
        }
    }

    std::vector<int> result;
    result.reserve(kFieldSlotCount);
    for (int slot = 0; slot < kFieldSlotCount; ++slot) {
        if (!occupied[static_cast<std::size_t>(slot)]) {
            result.push_back(slot);
        }
    }

    return result;
}

void RefillField(FourLeafClover::state_type& positions) {
    const auto empty_slots = EmptyFieldSlots(positions);
    const auto stock_cards = OrderedStock(positions);
    const auto refill_count = std::min(empty_slots.size(), stock_cards.size());

    for (std::size_t i = 0; i < refill_count; ++i) {
        MutablePosition(positions, stock_cards[i].second) = Field{empty_slots[i]};
    }

    for (std::size_t i = refill_count; i < stock_cards.size(); ++i) {
        MutablePosition(positions, stock_cards[i].second) = Stock{static_cast<int>(i - refill_count)};
    }
}

}  // namespace

FourLeafClover::FourLeafClover(state_type positions)
    : positions_(std::move(positions)) {
    ValidateState(positions_);
}

FourLeafClover FourLeafClover::deal(std::span<const card_type> deck) {
    if (deck.size() != kDeckSize) {
        ThrowInvalidState("Four Leaf Clover deals require exactly 48 cards.");
    }

    std::unordered_set<Card> seen;
    seen.reserve(deck.size());

    state_type positions;
    positions.reserve(deck.size());

    for (std::size_t i = 0; i < deck.size(); ++i) {
        const auto& card = deck[i];
        if (!IsSupportedCard(card)) {
            ThrowInvalidState("Four Leaf Clover deals require standard cards without tens.");
        }

        const auto [unused_it, inserted] = seen.insert(card);
        (void)unused_it;
        if (!inserted) {
            ThrowInvalidState("Four Leaf Clover deals require unique cards.");
        }

        if (i < static_cast<std::size_t>(kFieldSlotCount)) {
            positions.emplace(card, Field{static_cast<int>(i)});
        } else {
            positions.emplace(card, Stock{static_cast<int>(i - kFieldSlotCount)});
        }
    }

    return FourLeafClover{std::move(positions)};
}

const FourLeafClover::state_type& FourLeafClover::card_positions() const noexcept {
    return positions_;
}

const Position& FourLeafClover::position_of(const card_type& card) const {
    return LookupPosition(positions_, card);
}

bool FourLeafClover::contains(const card_type& card) const noexcept {
    return positions_.contains(card);
}

std::size_t FourLeafClover::stock_count() const noexcept {
    return std::count_if(
        positions_.begin(),
        positions_.end(),
        [](const auto& pair) {
            return std::holds_alternative<Stock>(pair.second);
        });
}

bool FourLeafClover::is_win() const noexcept {
    return std::all_of(
        positions_.begin(),
        positions_.end(),
        [](const auto& pair) {
            return std::holds_alternative<Removed>(pair.second);
        });
}

bool FourLeafClover::can_remove(std::span<const card_type> cards) const {
    if (cards.empty()) {
        return false;
    }

    std::unordered_set<Card> selected_cards;
    selected_cards.reserve(cards.size());

    const auto first_suit = cards.front().suit();
    int numeric_total = 0;
    bool all_numeric = true;

    for (const auto& card : cards) {
        if (!contains(card)) {
            return false;
        }

        const auto [unused_it, inserted] = selected_cards.insert(card);
        (void)unused_it;
        if (!inserted) {
            return false;
        }

        if (!IsFieldCard(positions_, card) || card.suit() != first_suit) {
            return false;
        }

        const auto value = NumericValue(card);
        if (!value.has_value()) {
            all_numeric = false;
            continue;
        }

        numeric_total += *value;
    }

    if (IsJackQueenKingSet(cards)) {
        return true;
    }

    return all_numeric && cards.size() >= 2 && numeric_total == 15;
}

FourLeafClover FourLeafClover::remove(std::span<const card_type> cards) const {
    if (!can_remove(cards)) {
        ThrowInvalidMove("Cards cannot be removed from this Four Leaf Clover state.");
    }

    auto next_positions = positions_;
    for (const auto& card : cards) {
        MutablePosition(next_positions, card) = Removed{};
    }

    RefillField(next_positions);
    return FourLeafClover{std::move(next_positions)};
}

}  // namespace shlab::gopc::games::four_leaf_clover
