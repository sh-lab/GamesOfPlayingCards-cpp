#include "shlab/gopc/games/yukon.hpp"

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

using shlab::gopc::games::yukon::Column;
using shlab::gopc::games::yukon::Foundation;
using shlab::gopc::games::yukon::Position;
using shlab::gopc::games::yukon::Tableau;
using shlab::gopc::games::yukon::Yukon;
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

Yukon::state_type AllFoundationState() {
    Yukon::state_type positions;
    positions.reserve(52);

    for (const auto& card : StandardDeck()) {
        positions.emplace(card, Foundation{});
    }

    return positions;
}

Yukon::state_type MakeState(
    const std::vector<std::pair<Card, Position>>& fixed_positions,
    const Column parking_column = Column::Seventh) {
    Yukon::state_type positions;
    positions.reserve(52);

    std::unordered_set<Card> assigned;
    assigned.reserve(52);
    std::array<std::vector<int>, 7> tableau_numbers;

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
            positions.emplace(card, Tableau{parking_column, parking_number++, true});
        }
    }

    return positions;
}

void TestDealBuildsInitialState() {
    const auto deck = StandardDeck();
    const auto yukon = Yukon::deal(deck);

    CheckEqual(yukon.card_positions().size(), std::size_t{52}, "Deal keeps all cards");
    Check(!yukon.is_win(), "Initial deal is not a win");

    CheckEqual(
        Get<Tableau>(yukon.position_of(deck[0])),
        Tableau{Column::First, 0, true},
        "First tableau column starts with one open card");
    CheckEqual(
        Get<Tableau>(yukon.position_of(deck[1])),
        Tableau{Column::Second, 0, false},
        "Second tableau column starts with one closed card");
    CheckEqual(
        Get<Tableau>(yukon.position_of(deck[2])),
        Tableau{Column::Second, 1, true},
        "Second tableau column then turns open");
    CheckEqual(
        Get<Tableau>(yukon.position_of(deck[30])),
        Tableau{Column::Fifth, 8, true},
        "Middle columns receive their extra open cards");
    CheckEqual(
        Get<Tableau>(yukon.position_of(deck[51])),
        Tableau{Column::Seventh, 10, true},
        "Last card ends on top of the seventh column");
}

void TestOpenReturnsNewState() {
    const auto target = Card::of(Suit::Spades, Rank::Ace);
    const auto yukon = Yukon{MakeState({
        {target, Tableau{Column::First, 0, false}},
    })};

    Check(yukon.can_open(target), "Top closed tableau card can be opened");

    const auto opened = yukon.open(target);

    CheckEqual(
        Get<Tableau>(yukon.position_of(target)),
        Tableau{Column::First, 0, false},
        "Open does not mutate the original state");
    CheckEqual(
        Get<Tableau>(opened.position_of(target)),
        Tableau{Column::First, 0, true},
        "Open returns a state with the card face up");
}

void TestMoveToFoundationAndSafetyQuery() {
    const auto ace = Card::of(Suit::Spades, Rank::Ace);
    const auto two = Card::of(Suit::Spades, Rank::Two);
    const auto yukon = Yukon{MakeState({
        {ace, Foundation{}},
        {two, Tableau{Column::First, 0, true}},
    })};

    Check(yukon.can_move_to_foundation(two), "Top open tableau card can move to foundation");
    Check(!yukon.is_moved_foundation_no_problem(two), "Safety query requires all lower-rank cards to be on foundation");

    const auto next = yukon.move_to_foundation(two);
    Check(HoldsAlternative<Foundation>(next.position_of(two)), "Move to foundation returns a foundation state");

    const auto safe_two = Card::of(Suit::Diamonds, Rank::Two);
    const auto safe_state = Yukon{MakeState({
        {Card::of(Suit::Hearts, Rank::Ace), Foundation{}},
        {Card::of(Suit::Spades, Rank::Ace), Foundation{}},
        {Card::of(Suit::Diamonds, Rank::Ace), Foundation{}},
        {Card::of(Suit::Clubs, Rank::Ace), Foundation{}},
        {safe_two, Tableau{Column::Second, 0, true}},
    })};

    Check(
        safe_state.is_moved_foundation_no_problem(safe_two),
        "Safety query passes when every lower-rank card is already on foundation");
}

void TestMoveTableauTailReturnsNewState() {
    const auto destination = Card::of(Suit::Diamonds, Rank::Eight);
    const auto moving_bottom = Card::of(Suit::Clubs, Rank::Seven);
    const auto moving_lower = Card::of(Suit::Clubs, Rank::King);
    const auto moving_top = Card::of(Suit::Spades, Rank::Five);

    const auto yukon = Yukon{MakeState({
        {destination, Tableau{Column::First, 0, true}},
        {moving_bottom, Tableau{Column::Second, 0, true}},
        {moving_lower, Tableau{Column::Second, 1, true}},
        {moving_top, Tableau{Column::Second, 2, true}},
    })};

    Check(
        yukon.can_move_to_tableau(moving_bottom, Column::First),
        "Open tableau card can move when its face card matches the destination");

    const auto next = yukon.move_to_tableau(moving_bottom, Column::First);

    CheckEqual(
        Get<Tableau>(next.position_of(moving_bottom)),
        Tableau{Column::First, 1, true},
        "Selected card lands first on the destination tableau");
    CheckEqual(
        Get<Tableau>(next.position_of(moving_lower)),
        Tableau{Column::First, 2, true},
        "Cards below the selected card move with it");
    CheckEqual(
        Get<Tableau>(next.position_of(moving_top)),
        Tableau{Column::First, 3, true},
        "Tail order is preserved during the move");
}

void TestMoveKingToEmptyColumn() {
    const auto king = Card::of(Suit::Clubs, Rank::King);
    const auto blocker = Card::of(Suit::Spades, Rank::Ace);
    const auto yukon = Yukon{MakeState({
        {king, Tableau{Column::First, 0, true}},
        {blocker, Tableau{Column::Second, 0, true}},
    }, Column::Third)};

    Check(yukon.can_move_to_tableau(king, Column::Seventh), "King can move to an empty tableau column");
}

void TestWinAndInvalidCases() {
    const auto win = Yukon{AllFoundationState()};
    Check(win.is_win(), "All-foundation state is a win");

    bool invalid_state = false;
    try {
        auto positions = MakeState({
            {Card::of(Suit::Clubs, Rank::King), Foundation{}},
        });
        positions.erase(Card::of(Suit::Clubs, Rank::Queen));
        positions.emplace(Card::from_id(CardId::Joker), Tableau{Column::First, 0, true});
        static_cast<void>(Yukon{std::move(positions)});
    } catch (const std::exception&) {
        invalid_state = true;
    }
    Check(invalid_state, "Yukon joker states remain invalid");

    const auto closed = Card::of(Suit::Spades, Rank::Queen);
    const auto invalid_move_state = Yukon{MakeState({
        {closed, Tableau{Column::First, 0, false}},
    })};

    bool invalid_move = false;
    try {
        static_cast<void>(invalid_move_state.move_to_foundation(closed));
    } catch (const std::logic_error&) {
        invalid_move = true;
    }
    Check(invalid_move, "Illegal moves throw");
}

}  // namespace

int main() {
    TestDealBuildsInitialState();
    TestOpenReturnsNewState();
    TestMoveToFoundationAndSafetyQuery();
    TestMoveTableauTailReturnsNewState();
    TestMoveKingToEmptyColumn();
    TestWinAndInvalidCases();

    if (g_failures != 0) {
        std::cerr << g_failures << " test(s) failed.\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
