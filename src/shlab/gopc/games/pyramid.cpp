#include "shlab/gopc/games/pyramid.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <utility>

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

[[noreturn]] void ThrowInvalidState(const char* message);
[[noreturn]] void ThrowInvalidMove(const char* message);

struct ContiguousNumbers {
    std::array<bool, kPyramidDeckSize> occupied{};
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
        if (HoldsAlternative<Hand>(position)) {
            return card;
        }
    }

    return std::nullopt;
}

bool IsTopDeckCard(const Pyramid::state_type& positions, const Card& card) {
    const auto* deck = GetIf<Deck>(&LookupPosition(positions, card));
    if (deck == nullptr) {
        return false;
    }

    for (const auto& [other_card, position] : positions) {
        (void)other_card;

        if (const auto* other_deck = GetIf<Deck>(&position);
            other_deck != nullptr && other_deck->number > deck->number) {
            return false;
        }
    }

    return true;
}

bool IsTopDiscardCard(const Pyramid::state_type& positions, const Card& card) {
    const auto* discard = GetIf<Discard>(&LookupPosition(positions, card));
    if (discard == nullptr) {
        return false;
    }

    for (const auto& [other_card, position] : positions) {
        (void)other_card;

        if (const auto* other_discard = GetIf<Discard>(&position);
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
    ContiguousNumbers deck_numbers;
    ContiguousNumbers discard_numbers;

    for (const auto& [card, position] : positions) {
        (void)card;

        if (const auto* deck = GetIf<Deck>(&position); deck != nullptr) {
            deck_numbers.add(deck->number, "Deck positions must use non-negative indices.");
        } else if (const auto* discard = GetIf<Discard>(&position); discard != nullptr) {
            discard_numbers.add(discard->number, "Discard positions must use non-negative indices.");
        }
    }

    deck_numbers.validate("Deck positions must be contiguous from zero.");
    discard_numbers.validate("Discard positions must be contiguous from zero.");
}

void ValidateField(const Pyramid::state_type& positions) {
    std::array<std::array<bool, kFieldRowCount>, kFieldRowCount> occupied{};

    for (const auto& [card, position] : positions) {
        (void)card;

        if (const auto* field = GetIf<Field>(&position); field != nullptr) {
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
        if (const auto* free_space = GetIf<FreeSpace>(&position); free_space != nullptr) {
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

        if (!HoldsAlternative<FreeSpace>(position) && !HoldsAlternative<Outside>(position)) {
            ThrowInvalidState("Jokers must be in FreeSpace or Outside.");
        }
    }
}

void ValidateHand(const Pyramid::state_type& positions) {
    bool has_hand = false;
    for (const auto& [card, position] : positions) {
        (void)card;
        if (!HoldsAlternative<Hand>(position)) {
            continue;
        }

        if (has_hand) {
            ThrowInvalidState("Pyramid state can contain at most one hand card.");
        }

        has_hand = true;
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

    std::array<bool, 256> seen{};
    std::array<std::optional<Card>, kStandardDeckSize> standard_cards{};
    std::size_t standard_card_count = 0;
    bool has_joker = false;
    bool has_extra_joker = false;

    for (const auto& card : deck) {
        const auto seen_index = static_cast<std::uint8_t>(card.id());
        if (seen[seen_index]) {
            ThrowInvalidState("Pyramid deals require 54 distinct cards.");
        }
        seen[seen_index] = true;

        if (card.is_standard_card()) {
            standard_cards[standard_card_count++] = card;
        } else if (card.id() == CardId::Joker) {
            has_joker = true;
        } else if (card.id() == CardId::ExtraJoker) {
            has_extra_joker = true;
        } else {
            ThrowInvalidState("Pyramid deals require supported cards only.");
        }
    }

    if (standard_card_count != kStandardDeckSize || !has_joker || !has_extra_joker) {
        ThrowInvalidState("Pyramid deals require 52 standard cards and two jokers.");
    }

    state_type positions;
    positions.reserve(deck.size());

    std::size_t card_index = 0;
    for (int row = 0; row < kFieldRowCount; ++row) {
        for (int number = 0; number <= row; ++number) {
            positions.emplace(*standard_cards[card_index++], Field{row, number});
        }
    }

    int deck_number = 0;
    while (card_index < standard_card_count) {
        positions.emplace(*standard_cards[card_index++], Deck{deck_number++});
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
            return HoldsAlternative<Deck>(pair.second);
        });
}

bool Pyramid::is_win() const noexcept {
    return std::none_of(
        positions_.begin(),
        positions_.end(),
        [](const auto& pair) {
            return HoldsAlternative<Field>(pair.second);
        });
}

bool Pyramid::can_select_for_remove(const card_type& card) const {
    const auto& position = position_of(card);

    if (HoldsAlternative<Outside>(position)) {
        return false;
    }

    if (HoldsAlternative<Hand>(position) || HoldsAlternative<FreeSpace>(position)) {
        return true;
    }

    if (HoldsAlternative<Discard>(position)) {
        return IsTopDiscardCard(positions_, card);
    }

    if (const auto* field = GetIf<Field>(&position); field != nullptr) {
        return !std::any_of(
            positions_.begin(),
            positions_.end(),
            [&](const auto& pair) {
                const auto* other_field = GetIf<Field>(&pair.second);
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
            return HoldsAlternative<Deck>(pair.second);
        });
}

Pyramid Pyramid::redeal() const {
    if (!can_redeal()) {
        ThrowInvalidMove("Discard pile cannot be redealt yet.");
    }

    auto next_positions = positions_;
    std::array<std::optional<Card>, kPyramidDeckSize> discard_cards{};
    std::size_t discard_card_count = 0;

    for (const auto& [card, position] : next_positions) {
        if (const auto* discard = GetIf<Discard>(&position); discard != nullptr) {
            const auto index = static_cast<std::size_t>(discard->number);
            discard_cards[index] = card;
            if (discard_card_count <= index) {
                discard_card_count = index + 1;
            }
        }
    }

    int deck_number = 0;
    for (std::size_t i = discard_card_count; i > 0; --i) {
        if (!discard_cards[i - 1].has_value()) {
            continue;
        }

        MutablePosition(next_positions, *discard_cards[i - 1]) = Deck{deck_number++};
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
                return HoldsAlternative<Discard>(pair.second);
            }))};
    }

    MutablePosition(next_positions, card) = Hand{};
    return Pyramid{std::move(next_positions)};
}

}  // namespace shlab::gopc::games::pyramid
