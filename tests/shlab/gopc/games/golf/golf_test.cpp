#include "shlab/gopc/games/golf.hpp"

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

using shlab::gopc::games::golf::Deck;
using shlab::gopc::games::golf::Field;
using shlab::gopc::games::golf::Golf;
using shlab::gopc::games::golf::Hand;
using shlab::gopc::games::golf::Lane;
using shlab::gopc::games::golf::Position;
using shlab::gopc::playing_cards::Card;
using shlab::gopc::playing_cards::CardId;
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

std::vector<Card> StandardDeck() {
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
        Rank::Ten,
        Rank::Jack,
        Rank::Queen,
        Rank::King,
    };

    std::vector<Card> deck;
    deck.reserve(52);
    for (const auto suit : suits) {
        for (const auto rank : ranks) {
            deck.push_back(Card::of(suit, rank));
        }
    }

    return deck;
}

std::vector<Card> GolfDeck(const bool include_joker = false, const bool include_extra_joker = false) {
    auto deck = StandardDeck();
    if (include_joker) {
        deck.push_back(Card::from_id(CardId::Joker));
    }
    if (include_extra_joker) {
        deck.push_back(Card::from_id(CardId::ExtraJoker));
    }
    return deck;
}

Golf::state_type MakeState(
    const std::vector<std::pair<Card, Position>>& fixed_positions,
    const bool can_loop,
    const bool include_joker = false,
    const bool include_extra_joker = false) {
    (void)can_loop;

    Golf::state_type positions;
    const auto deck = GolfDeck(include_joker, include_extra_joker);
    positions.reserve(deck.size());

    std::unordered_set<Card> assigned;
    assigned.reserve(deck.size());
    std::vector<int> fixed_deck_numbers;

    for (const auto& [card, position] : fixed_positions) {
        const auto [unused_it, inserted] = assigned.insert(card);
        (void)unused_it;
        if (!inserted) {
            throw std::invalid_argument("Duplicate fixed card.");
        }
        if (const auto* fixed_deck = std::get_if<Deck>(&position); fixed_deck != nullptr) {
            fixed_deck_numbers.push_back(fixed_deck->number);
        }
        positions.emplace(card, position);
    }

    std::sort(fixed_deck_numbers.begin(), fixed_deck_numbers.end());
    for (std::size_t i = 0; i < fixed_deck_numbers.size(); ++i) {
        if (fixed_deck_numbers[i] != static_cast<int>(i)) {
            throw std::invalid_argument("Fixed deck numbers must be contiguous from zero.");
        }
    }

    int deck_number = static_cast<int>(fixed_deck_numbers.size());
    for (const auto& card : deck) {
        if (!assigned.contains(card)) {
            positions.emplace(card, Deck{deck_number++});
        }
    }

    return positions;
}

void TestDealBuildsInitialState() {
    const auto deck = GolfDeck(true, true);
    const auto golf = Golf::deal(deck, true);

    Check(golf.can_loop(), "Loop rule is stored");
    CheckEqual(golf.card_positions().size(), std::size_t{54}, "Deal keeps all cards");
    CheckEqual(golf.deck_count(), std::size_t{18}, "Double joker deal leaves 18 deck cards");
    Check(!golf.is_win(), "Initial deal is not a win");
    Check(!golf.is_lose(), "Initial deal is not a lose");

    CheckEqual(std::get<Hand>(golf.position_of(deck[0])), Hand{0}, "First card starts in hand");
    CheckEqual(std::get<Field>(golf.position_of(deck[1])), Field{Lane::First, 0}, "First lane starts after hand");
    CheckEqual(std::get<Field>(golf.position_of(deck[35])), Field{Lane::Seventh, 4}, "Fields consume 35 cards");
    CheckEqual(std::get<Deck>(golf.position_of(deck[53])), Deck{17}, "Last card becomes top deck");
}

void TestMoveFromDeckReturnsNewState() {
    const auto deck = GolfDeck(true, true);
    const auto top_deck = deck.back();
    const auto golf = Golf::deal(deck, false);

    Check(golf.can_move_to_hand(top_deck), "Top deck card is always movable");

    const auto next = golf.move_to_hand(top_deck);

    CheckEqual(std::get<Deck>(golf.position_of(top_deck)), Deck{17}, "Original state is unchanged");
    CheckEqual(std::get<Hand>(next.position_of(top_deck)), Hand{1}, "Moved deck card becomes the new top hand");
    CheckEqual(next.deck_count(), std::size_t{17}, "Deck count decreases after drawing from deck");
}

void TestFieldTopRuleAndRankMove() {
    const auto hand_card = Card::of(Suit::Clubs, Rank::King);
    const auto blocked = Card::of(Suit::Clubs, Rank::Queen);
    const auto movable = Card::of(Suit::Diamonds, Rank::Queen);

    const auto golf = Golf{MakeState({
        {hand_card, Hand{0}},
        {blocked, Field{Lane::First, 0}},
        {movable, Field{Lane::First, 1}},
    }, false), false};

    Check(!golf.can_move_to_hand(blocked), "Only the top field card can move");
    Check(golf.can_move_to_hand(movable), "Adjacent rank field top can move");

    const auto next = golf.move_to_hand(movable);
    CheckEqual(std::get<Hand>(next.position_of(movable)), Hand{1}, "Field card moves onto hand");
}

void TestJokerRule() {
    const auto hand_card = Card::of(Suit::Spades, Rank::Seven);
    const auto joker = Card::from_id(CardId::Joker);
    const auto golf = Golf{MakeState({
        {hand_card, Hand{0}},
        {joker, Field{Lane::Second, 0}},
    }, false, true), false};

    Check(golf.can_move_to_hand(joker), "Joker field card can always move to hand");

    const auto ace = Card::of(Suit::Hearts, Rank::Ace);
    const auto next = Golf{MakeState({
        {joker, Hand{0}},
        {ace, Field{Lane::Third, 0}},
    }, false, true), false};

    Check(next.can_move_to_hand(ace), "Any card can move onto a joker hand");
}

void TestLoopRule() {
    const auto king = Card::of(Suit::Clubs, Rank::King);
    const auto ace = Card::of(Suit::Spades, Rank::Ace);

    const auto without_loop = Golf{MakeState({
        {king, Hand{0}},
        {ace, Field{Lane::First, 0}},
    }, false), false};

    const auto with_loop = Golf{MakeState({
        {king, Hand{0}},
        {ace, Field{Lane::First, 0}},
    }, true), true};

    Check(!without_loop.can_move_to_hand(ace), "Ace-King is blocked when loop is disabled");
    Check(with_loop.can_move_to_hand(ace), "Ace-King is allowed when loop is enabled");
}

void TestWinAndLoseQueries() {
    const auto hand_card = Card::of(Suit::Clubs, Rank::King);
    const auto dead_field = Card::of(Suit::Spades, Rank::Five);

    Golf::state_type lose_positions;
    lose_positions.reserve(52);
    lose_positions.emplace(hand_card, Hand{0});
    lose_positions.emplace(dead_field, Field{Lane::First, 0});

    int hand_number = 1;
    for (const auto& card : GolfDeck()) {
        if (card != hand_card && card != dead_field) {
            lose_positions.emplace(card, Hand{hand_number++});
        }
    }

    const auto lose = Golf{std::move(lose_positions), false};

    Check(!lose.is_win(), "Remaining field prevents win");
    Check(lose.is_lose(), "No deck and no legal field move is lose");

    Golf::state_type win_positions;
    win_positions.reserve(52);
    int win_hand_number = 0;
    for (const auto& card : GolfDeck()) {
        win_positions.emplace(card, Hand{win_hand_number++});
    }
    const auto win = Golf{std::move(win_positions), false};

    Check(win.is_win(), "No field cards means win");
    Check(!win.is_lose(), "Win state is not lose");
}

void TestInvalidStatesAndMoves() {
    bool invalid_extra_only = false;
    try {
        auto positions = MakeState({
            {Card::of(Suit::Clubs, Rank::King), Hand{0}},
        }, false, false, true);
        static_cast<void>(Golf{std::move(positions), false});
    } catch (const std::invalid_argument&) {
        invalid_extra_only = true;
    }
    Check(invalid_extra_only, "ExtraJoker alone is rejected");

    bool invalid_no_hand = false;
    try {
        auto positions = MakeState({
            {Card::of(Suit::Clubs, Rank::King), Deck{0}},
        }, false);
        static_cast<void>(Golf{std::move(positions), false});
    } catch (const std::invalid_argument&) {
        invalid_no_hand = true;
    }
    Check(invalid_no_hand, "States without a hand card are rejected");

    const auto golf = Golf{MakeState({
        {Card::of(Suit::Clubs, Rank::King), Hand{0}},
        {Card::of(Suit::Spades, Rank::Five), Field{Lane::First, 0}},
    }, false), false};

    bool invalid_move = false;
    try {
        static_cast<void>(golf.move_to_hand(Card::of(Suit::Spades, Rank::Five)));
    } catch (const std::logic_error&) {
        invalid_move = true;
    }
    Check(invalid_move, "Illegal moves throw");
}

}  // namespace

int main() {
    TestDealBuildsInitialState();
    TestMoveFromDeckReturnsNewState();
    TestFieldTopRuleAndRankMove();
    TestJokerRule();
    TestLoopRule();
    TestWinAndLoseQueries();
    TestInvalidStatesAndMoves();

    if (g_failures != 0) {
        std::cerr << g_failures << " test(s) failed.\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
