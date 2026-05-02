#include "shlab/gopc/games/golf.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

#include "shlab/gopc/playing_cards/card_id.hpp"
#include "shlab/gopc/playing_cards/rank.hpp"

namespace shlab::gopc::games::golf {
namespace {

using Card = shlab::gopc::playing_cards::Card;
using CardId = shlab::gopc::playing_cards::CardId;
using Rank = shlab::gopc::playing_cards::Rank;

constexpr std::size_t kStandardDeckSize = 52;
constexpr std::size_t kSingleJokerDeckSize = 53;
constexpr std::size_t kDoubleJokerDeckSize = 54;
constexpr int kLaneCount = 7;
constexpr int kInitialFieldDepth = 5;
constexpr std::size_t kInitialFieldCards = static_cast<std::size_t>(kLaneCount * kInitialFieldDepth);

[[noreturn]] void ThrowInvalidState(const char* message) {
    throw std::invalid_argument(message);
}

[[noreturn]] void ThrowInvalidMove(const char* message) {
    throw std::logic_error(message);
}

int ToInt(const Rank rank) noexcept {
    return static_cast<int>(rank);
}

int ToInt(const Lane lane) noexcept {
    return static_cast<int>(lane);
}

bool IsAllowedCardCount(const std::size_t count) noexcept {
    return count == kStandardDeckSize
        || count == kSingleJokerDeckSize
        || count == kDoubleJokerDeckSize;
}

const Position& LookupPosition(const Golf::state_type& positions, const Card& card) {
    const auto it = positions.find(card);
    if (it == positions.end()) {
        throw std::out_of_range("Card is not part of this Golf state.");
    }

    return it->second;
}

auto MutablePosition(Golf::state_type& positions, const Card& card) -> Position& {
    const auto it = positions.find(card);
    if (it == positions.end()) {
        throw std::out_of_range("Card is not part of this Golf state.");
    }

    return it->second;
}

Card TopHandCard(const Golf::state_type& positions) {
    std::optional<std::pair<int, Card>> result;

    for (const auto& [card, position] : positions) {
        if (const auto* hand = std::get_if<Hand>(&position); hand != nullptr) {
            if (!result.has_value() || result->first < hand->number) {
                result = std::pair{hand->number, card};
            }
        }
    }

    if (!result.has_value()) {
        ThrowInvalidState("Golf state must contain at least one hand card.");
    }

    return result->second;
}

int TopHandNumber(const Golf::state_type& positions) {
    return std::get<Hand>(LookupPosition(positions, TopHandCard(positions))).number;
}

bool IsTopDeckCard(const Golf::state_type& positions, const Card& card) {
    const auto* deck = std::get_if<Deck>(&LookupPosition(positions, card));
    if (deck == nullptr) {
        return false;
    }

    for (const auto& [other_card, position] : positions) {
        (void)other_card;

        if (const auto* other_deck = std::get_if<Deck>(&position);
            other_deck != nullptr && other_deck->number > deck->number) {
            return false;
        }
    }

    return true;
}

bool IsTopFieldCard(const Golf::state_type& positions, const Card& card) {
    const auto* field = std::get_if<Field>(&LookupPosition(positions, card));
    if (field == nullptr) {
        return false;
    }

    for (const auto& [other_card, position] : positions) {
        (void)other_card;

        if (const auto* other_field = std::get_if<Field>(&position);
            other_field != nullptr
            && other_field->lane == field->lane
            && other_field->number > field->number) {
            return false;
        }
    }

    return true;
}

bool CanMoveByRank(const Card& card, const Card& top_hand_card, const bool can_loop) noexcept {
    if (card.is_joker() || top_hand_card.is_joker()) {
        return true;
    }

    if (std::abs(ToInt(card.rank()) - ToInt(top_hand_card.rank())) == 1) {
        return true;
    }

    if (!can_loop) {
        return false;
    }

    return (card.rank() == Rank::Ace && top_hand_card.rank() == Rank::King)
        || (card.rank() == Rank::King && top_hand_card.rank() == Rank::Ace);
}

void ValidateCardSet(const Golf::state_type& positions) {
    if (!IsAllowedCardCount(positions.size())) {
        ThrowInvalidState("Golf state must contain 52, 53, or 54 cards.");
    }

    bool has_joker = false;
    bool has_extra_joker = false;

    for (const auto& [card, position] : positions) {
        (void)position;

        if (card.is_standard_card()) {
            continue;
        }

        if (card.id() == CardId::Joker) {
            has_joker = true;
            continue;
        }

        if (card.id() == CardId::ExtraJoker) {
            has_extra_joker = true;
            continue;
        }

        ThrowInvalidState("Golf state contains an unsupported card.");
    }

    const auto expected_size = kStandardDeckSize
        + static_cast<std::size_t>(has_joker)
        + static_cast<std::size_t>(has_extra_joker);

    if (expected_size != positions.size()) {
        ThrowInvalidState("Golf state card count does not match its joker composition.");
    }

    if (has_extra_joker && !has_joker) {
        ThrowInvalidState("ExtraJoker requires Joker to be present as well.");
    }
}

void ValidateSequence(const std::vector<int>& numbers, const char* message) {
    for (std::size_t i = 0; i < numbers.size(); ++i) {
        if (numbers[i] != static_cast<int>(i)) {
            ThrowInvalidState(message);
        }
    }
}

void ValidateHandAndDeck(const Golf::state_type& positions) {
    std::vector<int> hand_numbers;
    std::vector<int> deck_numbers;

    for (const auto& [card, position] : positions) {
        (void)card;

        if (const auto* hand = std::get_if<Hand>(&position); hand != nullptr) {
            if (hand->number < 0) {
                ThrowInvalidState("Hand positions must use non-negative indices.");
            }
            hand_numbers.push_back(hand->number);
        } else if (const auto* deck = std::get_if<Deck>(&position); deck != nullptr) {
            if (deck->number < 0) {
                ThrowInvalidState("Deck positions must use non-negative indices.");
            }
            deck_numbers.push_back(deck->number);
        }
    }

    if (hand_numbers.empty()) {
        ThrowInvalidState("Golf state must contain at least one hand card.");
    }

    std::sort(hand_numbers.begin(), hand_numbers.end());
    std::sort(deck_numbers.begin(), deck_numbers.end());

    ValidateSequence(hand_numbers, "Hand positions must be contiguous from zero.");
    ValidateSequence(deck_numbers, "Deck positions must be contiguous from zero.");
}

void ValidateField(const Golf::state_type& positions) {
    std::array<std::vector<int>, kLaneCount> lanes;

    for (const auto& [card, position] : positions) {
        (void)card;

        if (const auto* field = std::get_if<Field>(&position); field != nullptr) {
            if (field->number < 0) {
                ThrowInvalidState("Field positions must use non-negative indices.");
            }

            const auto lane_index = ToInt(field->lane);
            if (lane_index < 0 || lane_index >= kLaneCount) {
                ThrowInvalidState("Field lane is out of range.");
            }

            lanes[static_cast<std::size_t>(lane_index)].push_back(field->number);
        }
    }

    for (auto& lane : lanes) {
        if (lane.size() > static_cast<std::size_t>(kInitialFieldDepth)) {
            ThrowInvalidState("Golf lanes cannot contain more than 5 field cards.");
        }

        std::sort(lane.begin(), lane.end());
        ValidateSequence(lane, "Field positions must be contiguous within each lane.");
    }
}

void ValidateState(const Golf::state_type& positions) {
    ValidateCardSet(positions);
    ValidateHandAndDeck(positions);
    ValidateField(positions);
}

}  // namespace

Golf::Golf(state_type positions, const bool can_loop)
    : positions_(std::move(positions)),
      can_loop_(can_loop) {
    ValidateState(positions_);
}

Golf Golf::deal(std::span<const card_type> deck, const bool can_loop) {
    if (!IsAllowedCardCount(deck.size())) {
        ThrowInvalidState("Golf deals require 52, 53, or 54 cards.");
    }

    if (deck.size() < 1 + kInitialFieldCards) {
        ThrowInvalidState("Golf deals require enough cards for hand and field.");
    }

    state_type positions;
    positions.reserve(deck.size());

    std::size_t deck_index = 0;

    if (!positions.emplace(deck[deck_index++], Hand{0}).second) {
        ThrowInvalidState("Golf deals require distinct cards.");
    }

    for (int lane_index = 0; lane_index < kLaneCount; ++lane_index) {
        const auto lane = static_cast<Lane>(lane_index);
        for (int number = 0; number < kInitialFieldDepth; ++number) {
            if (!positions.emplace(deck[deck_index++], Field{lane, number}).second) {
                ThrowInvalidState("Golf deals require distinct cards.");
            }
        }
    }

    int deck_number = 0;
    while (deck_index < deck.size()) {
        if (!positions.emplace(deck[deck_index++], Deck{deck_number++}).second) {
            ThrowInvalidState("Golf deals require distinct cards.");
        }
    }

    return Golf{std::move(positions), can_loop};
}

const Golf::state_type& Golf::card_positions() const noexcept {
    return positions_;
}

const Position& Golf::position_of(const card_type& card) const {
    return LookupPosition(positions_, card);
}

bool Golf::contains(const card_type& card) const noexcept {
    return positions_.contains(card);
}

bool Golf::can_loop() const noexcept {
    return can_loop_;
}

std::size_t Golf::deck_count() const noexcept {
    return std::count_if(
        positions_.begin(),
        positions_.end(),
        [](const auto& pair) {
            return std::holds_alternative<Deck>(pair.second);
        });
}

bool Golf::is_win() const noexcept {
    return std::none_of(
        positions_.begin(),
        positions_.end(),
        [](const auto& pair) {
            return std::holds_alternative<Field>(pair.second);
        });
}

bool Golf::is_lose() const {
    return !is_win()
        && std::none_of(
            positions_.begin(),
            positions_.end(),
            [this](const auto& pair) {
                return can_move_to_hand(pair.first);
            });
}

bool Golf::can_move_to_hand(const card_type& card) const {
    if (!contains(card)) {
        return false;
    }

    if (IsTopDeckCard(positions_, card)) {
        return true;
    }

    if (!IsTopFieldCard(positions_, card)) {
        return false;
    }

    return CanMoveByRank(card, TopHandCard(positions_), can_loop_);
}

Golf Golf::move_to_hand(const card_type& card) const {
    if (!can_move_to_hand(card)) {
        ThrowInvalidMove("Card cannot be moved to the hand.");
    }

    auto next_positions = positions_;
    MutablePosition(next_positions, card) = Hand{TopHandNumber(next_positions) + 1};
    return Golf{std::move(next_positions), can_loop_};
}

}  // namespace shlab::gopc::games::golf
