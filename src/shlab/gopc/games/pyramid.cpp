#include "shlab/gopc/games/pyramid.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <unordered_set>
#include <utility>
#include <vector>

#include "shlab/gopc/playing_cards/card_id.hpp"
#include "shlab/gopc/playing_cards/rank.hpp"

namespace shlab::gopc::games::pyramid {
namespace {

using Card = shlab::gopc::playing_cards::Card;
using CardId = shlab::gopc::playing_cards::CardId;
using Rank = shlab::gopc::playing_cards::Rank;

constexpr std::size_t kStandardDeckSize = 52;
constexpr std::size_t kPyramidDeckSize = 54;
constexpr int kFieldRowCount = 7;
constexpr int kFreeSpaceCount = 2;

[[noreturn]] void ThrowInvalidState(const char* message) {
    throw std::invalid_argument(message);
}

[[noreturn]] void ThrowInvalidMove(const char* message) {
    throw std::logic_error(message);
}

int ToInt(const Rank rank) noexcept {
    return static_cast<int>(rank);
}

const Position& LookupPosition(const Pyramid::state_type& positions, const Card& card) {
    const auto it = positions.find(card);
    if (it == positions.end()) {
        throw std::out_of_range("Card is not part of this Pyramid state.");
    }

    return it->second;
}

auto MutablePosition(Pyramid::state_type& positions, const Card& card) -> Position& {
    const auto it = positions.find(card);
    if (it == positions.end()) {
        throw std::out_of_range("Card is not part of this Pyramid state.");
    }

    return it->second;
}

std::optional<Card> HandCard(const Pyramid::state_type& positions) {
    for (const auto& [card, position] : positions) {
        if (std::holds_alternative<Hand>(position)) {
            return card;
        }
    }

    return std::nullopt;
}

bool IsTopDeckCard(const Pyramid::state_type& positions, const Card& card) {
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

bool IsTopDiscardCard(const Pyramid::state_type& positions, const Card& card) {
    const auto* discard = std::get_if<Discard>(&LookupPosition(positions, card));
    if (discard == nullptr) {
        return false;
    }

    for (const auto& [other_card, position] : positions) {
        (void)other_card;

        if (const auto* other_discard = std::get_if<Discard>(&position);
            other_discard != nullptr && other_discard->number > discard->number) {
            return false;
        }
    }

    return true;
}

void ValidateCardSet(const Pyramid::state_type& positions) {
    if (positions.size() != kPyramidDeckSize) {
        ThrowInvalidState("Pyramid state must contain exactly 54 cards.");
    }

    std::size_t standard_card_count = 0;
    bool has_joker = false;
    bool has_extra_joker = false;

    for (const auto& [card, position] : positions) {
        (void)position;

        if (card.is_standard_card()) {
            ++standard_card_count;
        } else if (card.id() == CardId::Joker) {
            has_joker = true;
        } else if (card.id() == CardId::ExtraJoker) {
            has_extra_joker = true;
        } else {
            ThrowInvalidState("Pyramid state contains an unsupported card.");
        }
    }

    if (standard_card_count != kStandardDeckSize || !has_joker || !has_extra_joker) {
        ThrowInvalidState("Pyramid state must contain 52 standard cards and two jokers.");
    }
}

void ValidateDeckAndDiscard(const Pyramid::state_type& positions) {
    std::vector<int> deck_numbers;
    std::vector<int> discard_numbers;

    for (const auto& [card, position] : positions) {
        (void)card;

        if (const auto* deck = std::get_if<Deck>(&position); deck != nullptr) {
            if (deck->number < 0) {
                ThrowInvalidState("Deck positions must use non-negative indices.");
            }
            deck_numbers.push_back(deck->number);
        } else if (const auto* discard = std::get_if<Discard>(&position); discard != nullptr) {
            if (discard->number < 0) {
                ThrowInvalidState("Discard positions must use non-negative indices.");
            }
            discard_numbers.push_back(discard->number);
        }
    }

    std::sort(deck_numbers.begin(), deck_numbers.end());
    std::sort(discard_numbers.begin(), discard_numbers.end());

    for (std::size_t i = 0; i < deck_numbers.size(); ++i) {
        if (deck_numbers[i] != static_cast<int>(i)) {
            ThrowInvalidState("Deck positions must be contiguous from zero.");
        }
    }

    for (std::size_t i = 0; i < discard_numbers.size(); ++i) {
        if (discard_numbers[i] != static_cast<int>(i)) {
            ThrowInvalidState("Discard positions must be contiguous from zero.");
        }
    }
}

void ValidateField(const Pyramid::state_type& positions) {
    std::array<std::array<bool, kFieldRowCount>, kFieldRowCount> occupied{};

    for (const auto& [card, position] : positions) {
        (void)card;

        if (const auto* field = std::get_if<Field>(&position); field != nullptr) {
            if (field->row < 0 || field->row >= kFieldRowCount) {
                ThrowInvalidState("Field rows must be in the range [0, 6].");
            }

            if (field->number < 0 || field->number > field->row) {
                ThrowInvalidState("Field numbers must be within the row bounds.");
            }

            auto& slot = occupied[static_cast<std::size_t>(field->row)][static_cast<std::size_t>(field->number)];
            if (slot) {
                ThrowInvalidState("Field positions must be unique.");
            }
            slot = true;
        }
    }
}

void ValidateFreeSpaceAndJokers(const Pyramid::state_type& positions) {
    std::array<bool, kFreeSpaceCount> occupied_free_spaces{};

    for (const auto& [card, position] : positions) {
        if (const auto* free_space = std::get_if<FreeSpace>(&position); free_space != nullptr) {
            if (free_space->number < 0 || free_space->number >= kFreeSpaceCount) {
                ThrowInvalidState("FreeSpace positions must be in the range [0, 1].");
            }

            if (card.is_standard_card()) {
                ThrowInvalidState("Only jokers can be placed in FreeSpace.");
            }

            auto& slot = occupied_free_spaces[static_cast<std::size_t>(free_space->number)];
            if (slot) {
                ThrowInvalidState("FreeSpace positions must be unique.");
            }
            slot = true;
        }

        if (!card.is_joker() && !card.is_extra_joker()) {
            continue;
        }

        if (!std::holds_alternative<FreeSpace>(position) && !std::holds_alternative<Outside>(position)) {
            ThrowInvalidState("Jokers must be in FreeSpace or Outside.");
        }
    }
}

void ValidateHand(const Pyramid::state_type& positions) {
    const auto hand_count = std::count_if(
        positions.begin(),
        positions.end(),
        [](const auto& pair) {
            return std::holds_alternative<Hand>(pair.second);
        });

    if (hand_count > 1) {
        ThrowInvalidState("Pyramid state can contain at most one hand card.");
    }
}

void ValidateState(const Pyramid::state_type& positions) {
    ValidateCardSet(positions);
    ValidateDeckAndDiscard(positions);
    ValidateField(positions);
    ValidateFreeSpaceAndJokers(positions);
    ValidateHand(positions);
}

}  // namespace

Pyramid::Pyramid(state_type positions)
    : positions_(std::move(positions)) {
    ValidateState(positions_);
}

Pyramid Pyramid::deal(std::span<const card_type> deck) {
    if (deck.size() != kPyramidDeckSize) {
        ThrowInvalidState("Pyramid deals require exactly 54 cards.");
    }

    std::unordered_set<Card> seen;
    seen.reserve(deck.size());

    std::vector<Card> standard_cards;
    standard_cards.reserve(kStandardDeckSize);
    bool has_joker = false;
    bool has_extra_joker = false;

    for (const auto& card : deck) {
        if (!seen.insert(card).second) {
            ThrowInvalidState("Pyramid deals require 54 distinct cards.");
        }

        if (card.is_standard_card()) {
            standard_cards.push_back(card);
        } else if (card.id() == CardId::Joker) {
            has_joker = true;
        } else if (card.id() == CardId::ExtraJoker) {
            has_extra_joker = true;
        } else {
            ThrowInvalidState("Pyramid deals require supported cards only.");
        }
    }

    if (standard_cards.size() != kStandardDeckSize || !has_joker || !has_extra_joker) {
        ThrowInvalidState("Pyramid deals require 52 standard cards and two jokers.");
    }

    state_type positions;
    positions.reserve(deck.size());

    std::size_t card_index = 0;
    for (int row = 0; row < kFieldRowCount; ++row) {
        for (int number = 0; number <= row; ++number) {
            positions.emplace(standard_cards[card_index++], Field{row, number});
        }
    }

    int deck_number = 0;
    while (card_index < standard_cards.size()) {
        positions.emplace(standard_cards[card_index++], Deck{deck_number++});
    }

    positions.emplace(Card::from_id(CardId::Joker), FreeSpace{0});
    positions.emplace(Card::from_id(CardId::ExtraJoker), FreeSpace{1});

    return Pyramid{std::move(positions)};
}

const Pyramid::state_type& Pyramid::card_positions() const noexcept {
    return positions_;
}

const Position& Pyramid::position_of(const card_type& card) const {
    return LookupPosition(positions_, card);
}

bool Pyramid::contains(const card_type& card) const noexcept {
    return positions_.contains(card);
}

std::size_t Pyramid::deck_count() const noexcept {
    return std::count_if(
        positions_.begin(),
        positions_.end(),
        [](const auto& pair) {
            return std::holds_alternative<Deck>(pair.second);
        });
}

bool Pyramid::is_win() const noexcept {
    return std::none_of(
        positions_.begin(),
        positions_.end(),
        [](const auto& pair) {
            return std::holds_alternative<Field>(pair.second);
        });
}

bool Pyramid::can_select_for_remove(const card_type& card) const {
    const auto& position = position_of(card);

    if (std::holds_alternative<Outside>(position)) {
        return false;
    }

    if (std::holds_alternative<Hand>(position) || std::holds_alternative<FreeSpace>(position)) {
        return true;
    }

    if (std::holds_alternative<Discard>(position)) {
        return IsTopDiscardCard(positions_, card);
    }

    if (const auto* field = std::get_if<Field>(&position); field != nullptr) {
        return !std::any_of(
            positions_.begin(),
            positions_.end(),
            [&](const auto& pair) {
                const auto* other_field = std::get_if<Field>(&pair.second);
                return other_field != nullptr
                    && other_field->row == field->row + 1
                    && (other_field->number == field->number
                        || other_field->number == field->number + 1);
            });
    }

    return false;
}

bool Pyramid::can_remove(const card_type& card) const {
    return card.rank() == Rank::King && can_select_for_remove(card);
}

Pyramid Pyramid::remove(const card_type& card) const {
    if (!can_remove(card)) {
        ThrowInvalidMove("Card cannot be removed by itself.");
    }

    auto next_positions = positions_;
    MutablePosition(next_positions, card) = Outside{};
    return Pyramid{std::move(next_positions)};
}

bool Pyramid::can_remove(const card_type& first_card, const card_type& second_card) const {
    if (first_card == second_card) {
        return false;
    }

    if (!can_select_for_remove(first_card) || !can_select_for_remove(second_card)) {
        return false;
    }

    if (first_card.is_joker() || first_card.is_extra_joker()
        || second_card.is_joker() || second_card.is_extra_joker()) {
        return true;
    }

    return ToInt(first_card.rank()) + ToInt(second_card.rank()) == 13;
}

Pyramid Pyramid::remove(const card_type& first_card, const card_type& second_card) const {
    if (!can_remove(first_card, second_card)) {
        ThrowInvalidMove("Cards cannot be removed as a pair.");
    }

    auto next_positions = positions_;
    MutablePosition(next_positions, first_card) = Outside{};
    MutablePosition(next_positions, second_card) = Outside{};
    return Pyramid{std::move(next_positions)};
}

bool Pyramid::can_redeal() const noexcept {
    return !std::any_of(
        positions_.begin(),
        positions_.end(),
        [](const auto& pair) {
            return std::holds_alternative<Deck>(pair.second);
        });
}

Pyramid Pyramid::redeal() const {
    if (!can_redeal()) {
        ThrowInvalidMove("Discard pile cannot be redealt yet.");
    }

    auto next_positions = positions_;
    std::vector<std::pair<int, Card>> discard_cards;
    discard_cards.reserve(next_positions.size());

    for (const auto& [card, position] : next_positions) {
        if (const auto* discard = std::get_if<Discard>(&position); discard != nullptr) {
            discard_cards.emplace_back(discard->number, card);
        }
    }

    std::sort(
        discard_cards.begin(),
        discard_cards.end(),
        [](const auto& lhs, const auto& rhs) {
            return lhs.first > rhs.first;
        });

    int deck_number = 0;
    for (const auto& [discard_number, card] : discard_cards) {
        (void)discard_number;
        MutablePosition(next_positions, card) = Deck{deck_number++};
    }

    return Pyramid{std::move(next_positions)};
}

bool Pyramid::can_draw(const card_type& card) const {
    return IsTopDeckCard(positions_, card);
}

Pyramid Pyramid::draw(const card_type& card) const {
    if (!can_draw(card)) {
        ThrowInvalidMove("Card cannot be drawn from the deck.");
    }

    auto next_positions = positions_;
    const auto hand_card = HandCard(next_positions);
    if (hand_card.has_value()) {
        MutablePosition(next_positions, *hand_card) = Discard{static_cast<int>(std::count_if(
            next_positions.begin(),
            next_positions.end(),
            [](const auto& pair) {
                return std::holds_alternative<Discard>(pair.second);
            }))};
    }

    MutablePosition(next_positions, card) = Hand{};
    return Pyramid{std::move(next_positions)};
}

}  // namespace shlab::gopc::games::pyramid
