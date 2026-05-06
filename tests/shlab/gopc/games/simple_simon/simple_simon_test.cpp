#include "shlab/gopc/games/simple_simon.hpp"

#include <algorithm>
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

using shlab::gopc::games::simple_simon::Column;
using shlab::gopc::games::simple_simon::Foundation;
using shlab::gopc::games::simple_simon::Position;
using shlab::gopc::games::simple_simon::SimpleSimon;
using shlab::gopc::games::simple_simon::Tableau;
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

SimpleSimon::state_type AllFoundationState() {
    SimpleSimon::state_type positions;
    positions.reserve(52);

    for (const auto& card : StandardDeck()) {
        positions.emplace(card, Foundation{});
    }

    return positions;
}

SimpleSimon::state_type MakeState(
    const std::vector<std::pair<Card, Position>>& fixed_positions,
    const Column parking_column = Column::Tenth) {
    SimpleSimon::state_type positions;
    positions.reserve(52);

    std::unordered_set<Card> assigned;
    assigned.reserve(52);
    std::array<std::vector<int>, 10> tableau_numbers;

    for (const auto& [card, position] : fixed_positions) {
        const auto [unused_it, inserted] = assigned.insert(card);
        (void)unused_it;
        if (!inserted) {
            throw std::invalid_argument("Duplicate fixed card.");
        }

        if (const auto* tableau = GetIf<Tableau>(&position); tableau != nullptr) {
            tableau_numbers[static_cast<std::size_t>(tableau->column)].push_back(tableau->number);
        }

        positions.emplace(card, position);
    }

    auto& parking_numbers = tableau_numbers[static_cast<std::size_t>(parking_column)];
    std::sort(parking_numbers.begin(), parking_numbers.end());
    for (std::size_t i = 0; i < parking_numbers.size(); ++i) {
        if (parking_numbers[i] != static_cast<int>(i)) {
            throw std::invalid_argument("Parking column must be contiguous in fixed positions.");
        }
    }

    int parking_number = static_cast<int>(parking_numbers.size());
    for (const auto& card : StandardDeck()) {
        if (!assigned.contains(card)) {
            positions.emplace(card, Tableau{parking_column, parking_number++});
        }
    }

    return positions;
}

void TestDealBuildsInitialState() {
    const auto deck = StandardDeck();
    const auto simple_simon = SimpleSimon::deal(deck);

    CheckEqual(simple_simon.card_positions().size(), std::size_t{52}, "Deal keeps all cards");
    Check(!simple_simon.is_win(), "Initial deal is not a win");

    CheckEqual(
        Get<Tableau>(simple_simon.position_of(deck[0])),
        Tableau{Column::First, 0},
        "First card starts the first tableau column");
    CheckEqual(
        Get<Tableau>(simple_simon.position_of(deck[7])),
        Tableau{Column::First, 7},
        "First tableau column has eight cards");
    CheckEqual(
        Get<Tableau>(simple_simon.position_of(deck[8])),
        Tableau{Column::Second, 0},
        "Second tableau column follows the first");
    CheckEqual(
        Get<Tableau>(simple_simon.position_of(deck[24])),
        Tableau{Column::Fourth, 0},
        "Fourth tableau column starts after the three eight-card columns");
    CheckEqual(
        Get<Tableau>(simple_simon.position_of(deck[51])),
        Tableau{Column::Tenth, 0},
        "Last card becomes the only card in the tenth tableau column");
    Check(
        !HoldsAlternative<Foundation>(simple_simon.position_of(deck[0])),
        "Initial deal has no foundation cards");
}

void TestMoveCompleteSuitRunToFoundationReturnsNewState() {
    const auto king = Card::of(Suit::Spades, Rank::King);
    const auto queen = Card::of(Suit::Spades, Rank::Queen);
    const auto jack = Card::of(Suit::Spades, Rank::Jack);
    const auto ten = Card::of(Suit::Spades, Rank::Ten);
    const auto nine = Card::of(Suit::Spades, Rank::Nine);
    const auto eight = Card::of(Suit::Spades, Rank::Eight);
    const auto seven = Card::of(Suit::Spades, Rank::Seven);
    const auto six = Card::of(Suit::Spades, Rank::Six);
    const auto five = Card::of(Suit::Spades, Rank::Five);
    const auto four = Card::of(Suit::Spades, Rank::Four);
    const auto three = Card::of(Suit::Spades, Rank::Three);
    const auto two = Card::of(Suit::Spades, Rank::Two);
    const auto ace = Card::of(Suit::Spades, Rank::Ace);

    const auto simple_simon = SimpleSimon{MakeState({
        {king, Tableau{Column::First, 0}},
        {queen, Tableau{Column::First, 1}},
        {jack, Tableau{Column::First, 2}},
        {ten, Tableau{Column::First, 3}},
        {nine, Tableau{Column::First, 4}},
        {eight, Tableau{Column::First, 5}},
        {seven, Tableau{Column::First, 6}},
        {six, Tableau{Column::First, 7}},
        {five, Tableau{Column::First, 8}},
        {four, Tableau{Column::First, 9}},
        {three, Tableau{Column::First, 10}},
        {two, Tableau{Column::First, 11}},
        {ace, Tableau{Column::First, 12}},
    })};

    Check(simple_simon.can_move_to_foundation(king), "Complete same-suit run can move to foundation");
    Check(!simple_simon.can_move_to_foundation(queen), "Only the full run starting at king can move to foundation");

    const auto next = simple_simon.move_to_foundation(king);

    CheckEqual(
        Get<Tableau>(simple_simon.position_of(king)),
        Tableau{Column::First, 0},
        "Original state is unchanged after foundation move");
    Check(HoldsAlternative<Foundation>(next.position_of(king)), "King moves to foundation");
    Check(HoldsAlternative<Foundation>(next.position_of(ace)), "Ace moves to foundation with the run");
}

void TestMoveSameSuitTailToTableauReturnsNewState() {
    const auto destination = Card::of(Suit::Hearts, Rank::Nine);
    const auto eight = Card::of(Suit::Clubs, Rank::Eight);
    const auto seven = Card::of(Suit::Clubs, Rank::Seven);
    const auto six = Card::of(Suit::Clubs, Rank::Six);

    const auto simple_simon = SimpleSimon{MakeState({
        {destination, Tableau{Column::First, 0}},
        {eight, Tableau{Column::Second, 0}},
        {seven, Tableau{Column::Second, 1}},
        {six, Tableau{Column::Second, 2}},
    })};

    Check(
        simple_simon.can_move_to_tableau(eight, Column::First),
        "Same-suit descending tail can move onto the next higher rank");

    const auto next = simple_simon.move_to_tableau(eight, Column::First);

    CheckEqual(
        Get<Tableau>(simple_simon.position_of(eight)),
        Tableau{Column::Second, 0},
        "Original state is unchanged after tableau move");
    CheckEqual(
        Get<Tableau>(next.position_of(eight)),
        Tableau{Column::First, 1},
        "Selected card lands first on the destination tableau");
    CheckEqual(
        Get<Tableau>(next.position_of(seven)),
        Tableau{Column::First, 2},
        "Tail order is preserved during the move");
    CheckEqual(
        Get<Tableau>(next.position_of(six)),
        Tableau{Column::First, 3},
        "Entire same-suit tail moves together");
}

void TestMixedSuitTailCannotMoveAsOne() {
    const auto destination = Card::of(Suit::Hearts, Rank::Nine);
    const auto eight = Card::of(Suit::Clubs, Rank::Eight);
    const auto seven = Card::of(Suit::Spades, Rank::Seven);

    const auto simple_simon = SimpleSimon{MakeState({
        {destination, Tableau{Column::First, 0}},
        {eight, Tableau{Column::Second, 0}},
        {seven, Tableau{Column::Second, 1}},
    })};

    Check(
        !simple_simon.can_move_to_tableau(eight, Column::First),
        "Mixed-suit tail is not movable as one operation");
}

void TestMoveToEmptyTableau() {
    const auto five = Card::of(Suit::Diamonds, Rank::Five);
    const auto four = Card::of(Suit::Diamonds, Rank::Four);
    const auto filler = Card::of(Suit::Spades, Rank::King);

    const auto simple_simon = SimpleSimon{MakeState({
        {five, Tableau{Column::First, 0}},
        {four, Tableau{Column::First, 1}},
        {filler, Tableau{Column::Second, 0}},
    }, Column::Third)};

    Check(
        simple_simon.can_move_to_tableau(five, Column::Tenth),
        "Movable same-suit tail can move to an empty tableau column");
}

void TestWinAndInvalidCases() {
    const auto win = SimpleSimon{AllFoundationState()};
    Check(win.is_win(), "All-foundation state is a win");

    bool invalid_state = false;
    try {
        auto positions = MakeState({
            {Card::of(Suit::Clubs, Rank::King), Foundation{}},
        });
        positions.erase(Card::of(Suit::Clubs, Rank::Queen));
        positions.emplace(Card::from_id(CardId::Joker), Tableau{Column::First, 0});
        static_cast<void>(SimpleSimon{std::move(positions)});
    } catch (const std::invalid_argument&) {
        invalid_state = true;
    }
    Check(invalid_state, "Simple Simon rejects joker states");

    const auto king = Card::of(Suit::Clubs, Rank::King);
    const auto queen = Card::of(Suit::Clubs, Rank::Queen);
    const auto simple_simon = SimpleSimon{MakeState({
        {king, Tableau{Column::First, 0}},
        {queen, Tableau{Column::First, 1}},
    })};

    bool invalid_move = false;
    try {
        static_cast<void>(simple_simon.move_to_foundation(king));
    } catch (const std::logic_error&) {
        invalid_move = true;
    }
    Check(invalid_move, "Illegal moves throw");
}

}  // namespace

int main() {
    TestDealBuildsInitialState();
    TestMoveCompleteSuitRunToFoundationReturnsNewState();
    TestMoveSameSuitTailToTableauReturnsNewState();
    TestMixedSuitTailCannotMoveAsOne();
    TestMoveToEmptyTableau();
    TestWinAndInvalidCases();

    if (g_failures != 0) {
        std::cerr << g_failures << " Simple Simon test(s) failed.\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
