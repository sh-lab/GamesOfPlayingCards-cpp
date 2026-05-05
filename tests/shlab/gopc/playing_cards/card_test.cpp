#include "shlab/gopc/playing_cards.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <unordered_set>

namespace {

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

void TestStandardCardConstruction() {
    using namespace shlab::gopc::playing_cards;

    const auto ace_of_spades = Card::of(Suit::Spades, Rank::Ace);

    CheckEqual(ace_of_spades.id(), CardId::AceOfSpades, "Ace of Spades id");
    CheckEqual(ace_of_spades.suit(), Suit::Spades, "Ace of Spades suit");
    CheckEqual(ace_of_spades.rank(), Rank::Ace, "Ace of Spades rank");
    Check(!ace_of_spades.is_joker(), "Ace of Spades is not joker");
    Check(ace_of_spades.is_standard_card(), "Ace of Spades is standard card");
}

void TestIdBasedConstruction() {
    using namespace shlab::gopc::playing_cards;

    const auto queen_of_diamonds = Card::from_id(CardId::QueenOfDiamonds);

    CheckEqual(queen_of_diamonds.suit(), Suit::Diamonds, "Queen of Diamonds suit");
    CheckEqual(queen_of_diamonds.rank(), Rank::Queen, "Queen of Diamonds rank");
    CheckEqual(queen_of_diamonds, Card::of(Suit::Diamonds, Rank::Queen), "Factory equivalence");
}

void TestJokers() {
    using namespace shlab::gopc::playing_cards;

    const auto joker = Card::from_id(CardId::Joker);
    const auto extra_joker = Card::from_id(CardId::ExtraJoker);

    Check(joker.is_joker(), "Joker flag");
    Check(extra_joker.is_joker(), "ExtraJoker flag");
    Check(!joker.is_extra_joker(), "Joker is not extra joker");
    Check(extra_joker.is_extra_joker(), "ExtraJoker flag is distinct");
    CheckEqual(joker.suit(), Suit::None, "Joker suit");
    CheckEqual(joker.rank(), Rank::None, "Joker rank");
    Check(joker != extra_joker, "Two joker ids stay distinct");
}

void TestInvalidSuitRankPairs() {
    using namespace shlab::gopc::playing_cards;

    Check(!Card::try_of(Suit::None, Rank::Ace).has_value(), "Suit::None rejected");
    Check(!Card::try_of(Suit::Spades, Rank::None).has_value(), "Rank::None rejected");

    bool threw = false;
    try {
        static_cast<void>(Card::of(Suit::None, Rank::None));
    } catch (const std::invalid_argument&) {
        threw = true;
    }

    Check(threw, "Invalid Card::of throws");
}

void TestOrderingAndHashing() {
    using namespace shlab::gopc::playing_cards;

    const auto ace_of_spades = Card::of(Suit::Spades, Rank::Ace);
    const auto two_of_spades = Card::of(Suit::Spades, Rank::Two);
    const auto joker = Card::from_id(CardId::Joker);

    Check(ace_of_spades < two_of_spades, "Ordering follows CardId");
    Check(two_of_spades < joker, "Standard cards sort before jokers");

    std::unordered_set<Card> cards;
    cards.insert(ace_of_spades);
    cards.insert(Card::from_id(CardId::AceOfSpades));
    cards.insert(joker);
    cards.insert(Card::from_id(CardId::ExtraJoker));

    CheckEqual(cards.size(), std::size_t{3}, "Hash uses value semantics");
}

}  // namespace

int main() {
    TestStandardCardConstruction();
    TestIdBasedConstruction();
    TestJokers();
    TestInvalidSuitRankPairs();
    TestOrderingAndHashing();

    if (g_failures != 0) {
        std::cerr << g_failures << " test(s) failed.\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
