#include "shlab/gopc/playing_cards.hpp"
#include "shlab/gopc/utility/card_state.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {

using shlab::gopc::playing_cards::Card;
using shlab::gopc::playing_cards::CardId;
using shlab::gopc::playing_cards::Rank;
using shlab::gopc::playing_cards::Suit;
using shlab::gopc::utility::CardState;

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

void TestDirectIndexingAndLookup() {
    CardState<int, 54> state;

    const auto ace_of_spades = Card::of(Suit::Spades, Rank::Ace);
    const auto king_of_clubs = Card::of(Suit::Clubs, Rank::King);
    const auto joker = Card::from_id(CardId::Joker);
    const auto extra_joker = Card::from_id(CardId::ExtraJoker);

    Check(state.emplace(king_of_clubs, 40).second, "Insert King of Clubs");
    Check(state.emplace(ace_of_spades, 10).second, "Insert Ace of Spades");
    Check(state.emplace(joker, 50).second, "Insert Joker");
    Check(state.emplace(extra_joker, 60).second, "Insert Extra Joker");

    CheckEqual(state.size(), std::size_t{4}, "State size after direct inserts");
    Check(state.contains(ace_of_spades), "Contains Ace of Spades");
    Check(state.contains(joker), "Contains Joker");
    CheckEqual(state.at(king_of_clubs), 40, "Direct lookup finds King of Clubs");
    CheckEqual(state.find(extra_joker)->second, 60, "Find reaches Extra Joker slot");
}

void TestIterationSkipsUnoccupiedSlots() {
    CardState<int, 54> state;

    const auto ace_of_spades = Card::of(Suit::Spades, Rank::Ace);
    const auto king_of_clubs = Card::of(Suit::Clubs, Rank::King);
    const auto joker = Card::from_id(CardId::Joker);

    state.emplace(king_of_clubs, 40);
    state.emplace(joker, 50);
    state.emplace(ace_of_spades, 10);

    std::vector<Card> cards;
    std::vector<int> values;
    for (const auto& [card, value] : state) {
        cards.push_back(card);
        values.push_back(value);
    }

    CheckEqual(cards.size(), std::size_t{3}, "Iteration visits occupied slots only");
    CheckEqual(cards[0], ace_of_spades, "Iteration starts at first occupied direct index");
    CheckEqual(cards[1], king_of_clubs, "Iteration skips to later occupied standard slot");
    CheckEqual(cards[2], joker, "Iteration includes occupied joker slot");
    CheckEqual(values[0], 10, "Iteration keeps mapped value for Ace of Spades");
    CheckEqual(values[2], 50, "Iteration keeps mapped value for Joker");

    CheckEqual(state.erase(king_of_clubs), std::size_t{1}, "Erase removes occupied slot");
    cards.clear();
    for (const auto& [card, value] : state) {
        (void)value;
        cards.push_back(card);
    }

    CheckEqual(cards.size(), std::size_t{2}, "Iteration skips erased slot");
    CheckEqual(cards[0], ace_of_spades, "Iteration still begins with Ace of Spades");
    CheckEqual(cards[1], joker, "Iteration reaches joker after erased gap");
}

void TestOperatorBracketAndEquality() {
    CardState<int, 54> first;
    CardState<int, 54> second;

    const auto two_of_hearts = Card::of(Suit::Hearts, Rank::Two);
    const auto king_of_clubs = Card::of(Suit::Clubs, Rank::King);

    first[king_of_clubs] = 90;
    first[two_of_hearts] = 20;
    second[two_of_hearts] = 20;
    second[king_of_clubs] = 90;

    CheckEqual(first.size(), std::size_t{2}, "operator[] inserts exactly once per card");
    CheckEqual(first[two_of_hearts], 20, "operator[] returns existing mapped value");
    Check(first == second, "Equality ignores insertion order");

    second[king_of_clubs] = 91;
    Check(!(first == second), "Equality observes mapped value changes");
}

void TestUnsupportedCardsForStandardCapacity() {
    CardState<int, 52> state;
    const auto joker = Card::from_id(CardId::Joker);

    Check(!state.contains(joker), "52-card state does not contain joker");
    Check(state.find(joker) == state.end(), "Find returns end for unsupported joker");
    CheckEqual(state.erase(joker), std::size_t{0}, "Erase ignores unsupported joker");

    bool at_threw = false;
    try {
        static_cast<void>(state.at(joker));
    } catch (const std::out_of_range&) {
        at_threw = true;
    }
    Check(at_threw, "at throws for unsupported joker");

    bool emplace_threw = false;
    try {
        static_cast<void>(state.emplace(joker, 1));
    } catch (const std::out_of_range&) {
        emplace_threw = true;
    }
    Check(emplace_threw, "emplace throws for unsupported joker");

    bool bracket_threw = false;
    try {
        state[joker] = 2;
    } catch (const std::out_of_range&) {
        bracket_threw = true;
    }
    Check(bracket_threw, "operator[] throws for unsupported joker");
}

}  // namespace

int main() {
    TestDirectIndexingAndLookup();
    TestIterationSkipsUnoccupiedSlots();
    TestOperatorBracketAndEquality();
    TestUnsupportedCardsForStandardCapacity();

    if (g_failures != 0) {
        std::cerr << g_failures << " test(s) failed.\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
