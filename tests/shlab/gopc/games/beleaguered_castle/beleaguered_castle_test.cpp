#include "shlab/gopc/games/beleaguered_castle.hpp"

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

using shlab::gopc::games::beleaguered_castle::BeleagueredCastle;
using shlab::gopc::games::beleaguered_castle::Column;
using shlab::gopc::games::beleaguered_castle::Foundation;
using shlab::gopc::games::beleaguered_castle::Position;
using shlab::gopc::games::beleaguered_castle::Tableau;
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

BeleagueredCastle::state_type AllFoundationState() {
    BeleagueredCastle::state_type positions;
    positions.reserve(52);

    for (const auto& card : StandardDeck()) {
        positions.emplace(card, Foundation{});
    }

    return positions;
}

BeleagueredCastle::state_type MakeState(
    const std::vector<std::pair<Card, Position>>& fixed_positions,
    const Column parking_column = Column::Eighth) {
    BeleagueredCastle::state_type positions;
    positions.reserve(52);

    std::unordered_set<Card> assigned;
    assigned.reserve(52);
    std::array<std::vector<int>, 8> tableau_numbers;

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
    const auto castle = BeleagueredCastle::deal(deck);

    CheckEqual(castle.card_positions().size(), std::size_t{52}, "Deal keeps all cards");
    Check(!castle.is_win(), "Initial deal is not a win");

    Check(HoldsAlternative<Foundation>(castle.position_of(Card::of(Suit::Spades, Rank::Ace))), "Ace of spades starts on foundation");
    Check(HoldsAlternative<Foundation>(castle.position_of(Card::of(Suit::Hearts, Rank::Ace))), "Ace of hearts starts on foundation");
    Check(HoldsAlternative<Foundation>(castle.position_of(Card::of(Suit::Diamonds, Rank::Ace))), "Ace of diamonds starts on foundation");
    Check(HoldsAlternative<Foundation>(castle.position_of(Card::of(Suit::Clubs, Rank::Ace))), "Ace of clubs starts on foundation");

    CheckEqual(
        Get<Tableau>(castle.position_of(deck[1])),
        Tableau{Column::First, 0},
        "First non-ace card starts the first tableau column");
    CheckEqual(
        Get<Tableau>(castle.position_of(deck[6])),
        Tableau{Column::First, 5},
        "First tableau column has six cards");
    CheckEqual(
        Get<Tableau>(castle.position_of(deck[7])),
        Tableau{Column::Second, 0},
        "Second tableau column follows the first");
    CheckEqual(
        Get<Tableau>(castle.position_of(deck[51])),
        Tableau{Column::Eighth, 5},
        "Last card becomes the top of the eighth tableau column");
}

void TestMoveToFoundationReturnsNewState() {
    const auto ace = Card::of(Suit::Spades, Rank::Ace);
    const auto two = Card::of(Suit::Spades, Rank::Two);
    const auto castle = BeleagueredCastle{MakeState({
        {ace, Foundation{}},
        {two, Tableau{Column::First, 0}},
    })};

    Check(castle.can_move_to_foundation(two), "Top tableau card can move to foundation when previous rank is present");

    const auto next = castle.move_to_foundation(two);

    CheckEqual(
        Get<Tableau>(castle.position_of(two)),
        Tableau{Column::First, 0},
        "Original state is unchanged after foundation move");
    Check(HoldsAlternative<Foundation>(next.position_of(two)), "Moved card becomes a foundation card");
}

void TestMoveToTableauReturnsNewState() {
    const auto moving = Card::of(Suit::Clubs, Rank::Seven);
    const auto destination = Card::of(Suit::Hearts, Rank::Eight);
    const auto castle = BeleagueredCastle{MakeState({
        {moving, Tableau{Column::First, 0}},
        {destination, Tableau{Column::Second, 0}},
    })};

    Check(castle.can_move_to_tableau(moving, Column::Second), "Top tableau card can move onto any higher next rank regardless of suit");

    const auto next = castle.move_to_tableau(moving, Column::Second);

    CheckEqual(
        Get<Tableau>(castle.position_of(moving)),
        Tableau{Column::First, 0},
        "Original state is unchanged after tableau move");
    CheckEqual(
        Get<Tableau>(next.position_of(moving)),
        Tableau{Column::Second, 1},
        "Moved card becomes the new tableau top");
}

void TestMoveToEmptyTableau() {
    const auto moving = Card::of(Suit::Diamonds, Rank::Queen);
    const auto filler = Card::of(Suit::Spades, Rank::King);
    const auto castle = BeleagueredCastle{MakeState({
        {moving, Tableau{Column::First, 0}},
        {filler, Tableau{Column::Second, 0}},
    }, Column::Third)};

    Check(castle.can_move_to_tableau(moving, Column::Eighth), "Any exposed card can move to an empty tableau");
}

void TestOnlyExposedCardsCanMove() {
    const auto blocked = Card::of(Suit::Diamonds, Rank::Nine);
    const auto top = Card::of(Suit::Clubs, Rank::Ten);
    const auto destination = Card::of(Suit::Spades, Rank::Jack);
    const auto castle = BeleagueredCastle{MakeState({
        {blocked, Tableau{Column::First, 0}},
        {top, Tableau{Column::First, 1}},
        {destination, Tableau{Column::Second, 0}},
    })};

    Check(!castle.can_move_to_foundation(blocked), "Only exposed tableau cards can move to foundation");
    Check(!castle.can_move_to_tableau(blocked, Column::Second), "Only exposed tableau cards can move to tableau");
    Check(castle.can_move_to_tableau(top, Column::Second), "Exposed tableau cards remain movable");
}

void TestWinAndInvalidCases() {
    const auto win = BeleagueredCastle{AllFoundationState()};
    Check(win.is_win(), "All-foundation state is a win");

    bool invalid_state = false;
    try {
        auto positions = MakeState({
            {Card::of(Suit::Clubs, Rank::King), Foundation{}},
        });
        positions.erase(Card::of(Suit::Clubs, Rank::Queen));
        positions.emplace(Card::from_id(CardId::Joker), Tableau{Column::First, 0});
        static_cast<void>(BeleagueredCastle{std::move(positions)});
    } catch (const std::invalid_argument&) {
        invalid_state = true;
    }
    Check(invalid_state, "Beleaguered Castle rejects joker states");

    const auto king = Card::of(Suit::Clubs, Rank::King);
    const auto castle = BeleagueredCastle{MakeState({
        {king, Tableau{Column::First, 0}},
    })};

    bool invalid_move = false;
    try {
        static_cast<void>(castle.move_to_foundation(king));
    } catch (const std::logic_error&) {
        invalid_move = true;
    }
    Check(invalid_move, "Illegal moves throw");
}

}  // namespace

int main() {
    TestDealBuildsInitialState();
    TestMoveToFoundationReturnsNewState();
    TestMoveToTableauReturnsNewState();
    TestMoveToEmptyTableau();
    TestOnlyExposedCardsCanMove();
    TestWinAndInvalidCases();

    if (g_failures != 0) {
        std::cerr << g_failures << " test(s) failed.\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
