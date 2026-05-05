#include "shlab/gopc/games/four_leaf_clover.hpp"

#include <array>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include "shlab/gopc/playing_cards.hpp"

namespace {

using shlab::gopc::games::four_leaf_clover::Field;
using shlab::gopc::games::four_leaf_clover::FourLeafClover;
using shlab::gopc::games::four_leaf_clover::Position;
using shlab::gopc::games::four_leaf_clover::Removed;
using shlab::gopc::games::four_leaf_clover::Stock;
using shlab::gopc::playing_cards::Card;
using shlab::gopc::playing_cards::Rank;
using shlab::gopc::playing_cards::Suit;

int g_failures = 0;

void Check(const bool condition, const std::string_view message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        ++g_failures;
    }
}

template <typename T, typename U>
void CheckEqual(const T& actual, const U& expected, const std::string_view message) {
    Check(actual == expected, message);
}

std::vector<Card> FourLeafCloverDeck() {
    constexpr std::array suits{
        Suit::Spades,
        Suit::Hearts,
        Suit::Diamonds,
        Suit::Clubs,
    };

    constexpr std::array ranks{
        Rank::Ace,
        Rank::Two,
        Rank::Three,
        Rank::Four,
        Rank::Five,
        Rank::Six,
        Rank::Seven,
        Rank::Eight,
        Rank::Nine,
        Rank::Jack,
        Rank::Queen,
        Rank::King,
    };

    std::vector<Card> deck;
    deck.reserve(48);
    for (const auto suit : suits) {
        for (const auto rank : ranks) {
            deck.push_back(Card::of(suit, rank));
        }
    }

    return deck;
}

FourLeafClover::state_type AllRemovedState() {
    FourLeafClover::state_type positions;
    positions.reserve(48);

    for (const auto& card : FourLeafCloverDeck()) {
        positions.emplace(card, Removed{});
    }

    return positions;
}

FourLeafClover::state_type MakeState(const std::vector<std::pair<Card, Position>>& fixed_positions) {
    FourLeafClover::state_type positions;
    positions.reserve(48);

    std::unordered_set<Card> assigned;
    assigned.reserve(48);

    for (const auto& [card, position] : fixed_positions) {
        const auto [unused_it, inserted] = assigned.insert(card);
        (void)unused_it;
        if (!inserted) {
            throw std::invalid_argument("Duplicate fixed card.");
        }

        positions.emplace(card, position);
    }

    for (const auto& card : FourLeafCloverDeck()) {
        if (!assigned.contains(card)) {
            positions.emplace(card, Removed{});
        }
    }

    return positions;
}

void TestDealBuildsInitialState() {
    const auto deck = FourLeafCloverDeck();
    const auto game = FourLeafClover::deal(deck);

    CheckEqual(game.card_positions().size(), std::size_t{48}, "Deal keeps all cards");
    CheckEqual(game.stock_count(), std::size_t{32}, "Deal leaves 32 cards in stock");
    Check(!game.is_win(), "Initial deal is not a win");

    CheckEqual(std::get<Field>(game.position_of(deck[0])), Field{0}, "First card starts in the top-left field slot");
    CheckEqual(std::get<Field>(game.position_of(deck[15])), Field{15}, "Sixteenth card ends the 4x4 field");
    CheckEqual(std::get<Stock>(game.position_of(deck[16])), Stock{0}, "Seventeenth card becomes the first stock card");
    CheckEqual(std::get<Stock>(game.position_of(deck[47])), Stock{31}, "Last card becomes the last stock card");
}

void TestRemoveSameSuitSumFifteenRefillsInFieldOrder() {
    const auto six = Card::of(Suit::Hearts, Rank::Six);
    const auto nine = Card::of(Suit::Hearts, Rank::Nine);
    const auto refill_first = Card::of(Suit::Clubs, Rank::Ace);
    const auto refill_second = Card::of(Suit::Spades, Rank::Three);
    const auto stock_tail = Card::of(Suit::Diamonds, Rank::Four);
    std::vector<std::pair<Card, Position>> fixed_positions{
        {six, Field{5}},
        {nine, Field{2}},
        {refill_first, Stock{0}},
        {refill_second, Stock{1}},
        {stock_tail, Stock{2}},
    };

    std::unordered_set<Card> used{
        six,
        nine,
        refill_first,
        refill_second,
        stock_tail,
    };

    int field_slot = 0;
    for (const auto& card : FourLeafCloverDeck()) {
        if (used.contains(card)) {
            continue;
        }

        while (field_slot == 2 || field_slot == 5) {
            ++field_slot;
        }

        if (field_slot >= 16) {
            break;
        }

        fixed_positions.emplace_back(card, Field{field_slot});
        used.insert(card);
        ++field_slot;
    }

    const auto game = FourLeafClover{MakeState(fixed_positions)};
    const std::array selection{six, nine};

    Check(game.can_remove(selection), "Same-suit cards summing to 15 can be removed");

    const auto next = game.remove(selection);

    Check(std::holds_alternative<Removed>(next.position_of(six)), "Removed numeric card leaves the field");
    Check(std::holds_alternative<Removed>(next.position_of(nine)), "Second removed numeric card leaves the field");
    CheckEqual(std::get<Field>(next.position_of(refill_first)), Field{2}, "First stock card refills the first empty field slot");
    CheckEqual(std::get<Field>(next.position_of(refill_second)), Field{5}, "Second stock card refills the next empty field slot");
    CheckEqual(std::get<Stock>(next.position_of(stock_tail)), Stock{0}, "Remaining stock is renumbered after refill");
    CheckEqual(std::get<Field>(game.position_of(six)), Field{5}, "Original state stays unchanged after removal");
}

void TestRemoveThreeNumericCardsSummingFifteen() {
    const auto ace = Card::of(Suit::Clubs, Rank::Ace);
    const auto six = Card::of(Suit::Clubs, Rank::Six);
    const auto eight = Card::of(Suit::Clubs, Rank::Eight);
    const auto game = FourLeafClover{MakeState({
        {ace, Field{0}},
        {six, Field{1}},
        {eight, Field{2}},
    })};
    const std::array selection{ace, six, eight};

    Check(game.can_remove(selection), "Three same-suit numeric cards can be removed when their total is 15");
}

void TestRemoveJackQueenKingSet() {
    const auto jack = Card::of(Suit::Spades, Rank::Jack);
    const auto queen = Card::of(Suit::Spades, Rank::Queen);
    const auto king = Card::of(Suit::Spades, Rank::King);
    const auto game = FourLeafClover{MakeState({
        {jack, Field{0}},
        {queen, Field{7}},
        {king, Field{10}},
    })};
    const std::array selection{jack, queen, king};

    Check(game.can_remove(selection), "Same-suit JQK can be removed together");

    const auto next = game.remove(selection);
    Check(std::holds_alternative<Removed>(next.position_of(jack)), "Removed jack moves to Removed");
    Check(std::holds_alternative<Removed>(next.position_of(queen)), "Removed queen moves to Removed");
    Check(std::holds_alternative<Removed>(next.position_of(king)), "Removed king moves to Removed");
}

void TestIllegalRemovalsAndThrows() {
    const auto hearts_six = Card::of(Suit::Hearts, Rank::Six);
    const auto spades_nine = Card::of(Suit::Spades, Rank::Nine);
    const auto hearts_jack = Card::of(Suit::Hearts, Rank::Jack);
    const auto clubs_queen = Card::of(Suit::Clubs, Rank::Queen);
    const auto hearts_king = Card::of(Suit::Hearts, Rank::King);
    const auto stock_card = Card::of(Suit::Diamonds, Rank::Ace);

    const auto game = FourLeafClover{MakeState({
        {hearts_six, Field{0}},
        {spades_nine, Field{1}},
        {hearts_jack, Field{2}},
        {clubs_queen, Field{3}},
        {hearts_king, Field{4}},
        {stock_card, Stock{0}},
    })};

    const std::array mixed_sum{hearts_six, spades_nine};
    const std::array mixed_faces{hearts_jack, clubs_queen, hearts_king};
    const std::array includes_stock{hearts_six, stock_card};

    Check(!game.can_remove(mixed_sum), "Different suits cannot be removed for a sum of 15");
    Check(!game.can_remove(mixed_faces), "JQK removal requires matching suits");
    Check(!game.can_remove(includes_stock), "Only field cards can be removed");

    bool invalid_move = false;
    try {
        static_cast<void>(game.remove(mixed_sum));
    } catch (const std::logic_error&) {
        invalid_move = true;
    }
    Check(invalid_move, "Illegal removals throw");
}

void TestWinAndInvalidState() {
    const auto win = FourLeafClover{AllRemovedState()};
    Check(win.is_win(), "All removed cards produce a win");

    bool invalid_state = false;
    try {
        auto positions = AllRemovedState();
        positions.erase(Card::of(Suit::Clubs, Rank::King));
        positions.emplace(Card::of(Suit::Clubs, Rank::Ten), Removed{});
        static_cast<void>(FourLeafClover{std::move(positions)});
    } catch (const std::invalid_argument&) {
        invalid_state = true;
    }
    Check(invalid_state, "States containing tens are rejected");
}

}  // namespace

int main() {
    TestDealBuildsInitialState();
    TestRemoveSameSuitSumFifteenRefillsInFieldOrder();
    TestRemoveThreeNumericCardsSummingFifteen();
    TestRemoveJackQueenKingSet();
    TestIllegalRemovalsAndThrows();
    TestWinAndInvalidState();

    if (g_failures != 0) {
        std::cerr << g_failures << " test(s) failed.\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
