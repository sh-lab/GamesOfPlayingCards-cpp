#include "shlab/gopc/games/freecell.hpp"

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

using shlab::gopc::games::freecell::Cell;
using shlab::gopc::games::freecell::Column;
using shlab::gopc::games::freecell::Foundation;
using shlab::gopc::games::freecell::FreeCell;
using shlab::gopc::games::freecell::Position;
using shlab::gopc::games::freecell::Tableau;
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

FreeCell::state_type AllFoundationState() {
    FreeCell::state_type positions;
    positions.reserve(52);

    for (const auto& card : StandardDeck()) {
        positions.emplace(card, Foundation{});
    }

    return positions;
}

FreeCell::state_type MakeState(
    const std::vector<std::pair<Card, Position>>& fixed_positions,
    const Column parking_column = Column::Eighth) {
    FreeCell::state_type positions;
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
    const auto free_cell = FreeCell::deal(deck);

    CheckEqual(free_cell.card_positions().size(), std::size_t{52}, "Deal keeps all cards");
    Check(free_cell.contains(deck[0]), "Deal contains the first card");
    Check(!free_cell.is_win(), "Initial deal is not a win");

    CheckEqual(
        Get<Tableau>(free_cell.position_of(deck[0])),
        Tableau{Column::First, 0},
        "First card starts in the first tableau column");
    CheckEqual(
        Get<Tableau>(free_cell.position_of(deck[6])),
        Tableau{Column::First, 6},
        "First column receives seven cards");
    CheckEqual(
        Get<Tableau>(free_cell.position_of(deck[7])),
        Tableau{Column::Second, 0},
        "Second column starts after the first seven cards");
    CheckEqual(
        Get<Tableau>(free_cell.position_of(deck[27])),
        Tableau{Column::Fourth, 6},
        "Fourth column is the last seven-card tableau");
    CheckEqual(
        Get<Tableau>(free_cell.position_of(deck[28])),
        Tableau{Column::Fifth, 0},
        "Fifth column starts the six-card tableaux");
    CheckEqual(
        Get<Tableau>(free_cell.position_of(deck[51])),
        Tableau{Column::Eighth, 5},
        "Last card lands on top of the last tableau column");
}

void TestMoveToCellReturnsNewState() {
    const auto blocked = Card::of(Suit::Clubs, Rank::Queen);
    const auto moving = Card::of(Suit::Spades, Rank::King);
    const auto free_cell = FreeCell{MakeState({
        {blocked, Tableau{Column::First, 0}},
        {moving, Tableau{Column::First, 1}},
    })};

    Check(free_cell.can_move_to_cell(moving), "Top tableau card can move to a cell");
    Check(!free_cell.can_move_to_cell(blocked), "Only the top tableau card can move to a cell");

    const auto next = free_cell.move_to_cell(moving);

    CheckEqual(
        Get<Tableau>(free_cell.position_of(moving)),
        Tableau{Column::First, 1},
        "Original state is unchanged after moving to a cell");
    CheckEqual(
        Get<Cell>(next.position_of(moving)),
        Cell{0},
        "Moved card occupies the first empty cell");
}

void TestMoveToFoundationFromCellAndTableau() {
    const auto ace_of_spades = Card::of(Suit::Spades, Rank::Ace);
    const auto from_cell = FreeCell{MakeState({
        {ace_of_spades, Cell{2}},
    })};

    Check(from_cell.can_move_to_foundation(ace_of_spades), "Ace in a cell can move to foundation");

    const auto cell_next = from_cell.move_to_foundation(ace_of_spades);
    Check(
        HoldsAlternative<Foundation>(cell_next.position_of(ace_of_spades)),
        "Moving to foundation returns a foundation state");

    const auto two_of_spades = Card::of(Suit::Spades, Rank::Two);
    const auto from_tableau = FreeCell{MakeState({
        {ace_of_spades, Foundation{}},
        {two_of_spades, Tableau{Column::First, 0}},
    })};

    Check(
        from_tableau.can_move_to_foundation(two_of_spades),
        "Top tableau card can move to foundation when the lower rank is already there");
}

void TestMoveFromCellToTableau() {
    const auto destination = Card::of(Suit::Hearts, Rank::Five);
    const auto moving = Card::of(Suit::Clubs, Rank::Four);
    const auto free_cell = FreeCell{MakeState({
        {destination, Tableau{Column::First, 0}},
        {moving, Cell{1}},
    })};

    Check(free_cell.can_move_to_tableau(moving, Column::First), "Cell card can move onto a valid tableau top");

    const auto next = free_cell.move_to_tableau(moving, Column::First);

    CheckEqual(
        Get<Cell>(free_cell.position_of(moving)),
        Cell{1},
        "Original state is unchanged after moving from a cell");
    CheckEqual(
        Get<Tableau>(next.position_of(moving)),
        Tableau{Column::First, 1},
        "Cell card becomes the new tableau top");
}

void TestMoveTableauStack() {
    const auto destination = Card::of(Suit::Diamonds, Rank::Eight);
    const auto moving_bottom = Card::of(Suit::Clubs, Rank::Seven);
    const auto moving_middle = Card::of(Suit::Hearts, Rank::Six);
    const auto moving_top = Card::of(Suit::Spades, Rank::Five);

    const auto free_cell = FreeCell{MakeState({
        {destination, Tableau{Column::First, 0}},
        {moving_bottom, Tableau{Column::Second, 0}},
        {moving_middle, Tableau{Column::Second, 1}},
        {moving_top, Tableau{Column::Second, 2}},
    })};

    Check(
        free_cell.can_move_to_tableau(moving_bottom, Column::First),
        "A valid tableau stack can move onto a matching destination");

    const auto next = free_cell.move_to_tableau(moving_bottom, Column::First);

    CheckEqual(
        Get<Tableau>(next.position_of(moving_bottom)),
        Tableau{Column::First, 1},
        "Bottom card lands first on the destination tableau");
    CheckEqual(
        Get<Tableau>(next.position_of(moving_middle)),
        Tableau{Column::First, 2},
        "Middle card keeps its relative order");
    CheckEqual(
        Get<Tableau>(next.position_of(moving_top)),
        Tableau{Column::First, 3},
        "Top card keeps its relative order");
}

void TestStandardStackMoveCapacityRule() {
    const auto moving_bottom = Card::of(Suit::Clubs, Rank::Seven);
    const auto card_2 = Card::of(Suit::Hearts, Rank::Six);
    const auto card_3 = Card::of(Suit::Spades, Rank::Five);
    const auto card_4 = Card::of(Suit::Diamonds, Rank::Four);
    const auto occupied_cell_1 = Card::of(Suit::Clubs, Rank::King);
    const auto occupied_cell_2 = Card::of(Suit::Hearts, Rank::King);
    const auto occupied_cell_3 = Card::of(Suit::Spades, Rank::King);
    const auto occupied_second = Card::of(Suit::Diamonds, Rank::King);
    const auto occupied_third = Card::of(Suit::Clubs, Rank::Ace);
    const auto occupied_sixth = Card::of(Suit::Hearts, Rank::Ace);
    const auto occupied_seventh = Card::of(Suit::Diamonds, Rank::Ace);

    const auto free_cell = FreeCell{MakeState({
        {moving_bottom, Tableau{Column::First, 0}},
        {card_2, Tableau{Column::First, 1}},
        {card_3, Tableau{Column::First, 2}},
        {card_4, Tableau{Column::First, 3}},
        {occupied_second, Tableau{Column::Second, 0}},
        {occupied_third, Tableau{Column::Third, 0}},
        {occupied_sixth, Tableau{Column::Sixth, 0}},
        {occupied_seventh, Tableau{Column::Seventh, 0}},
        {occupied_cell_1, Cell{1}},
        {occupied_cell_2, Cell{2}},
        {occupied_cell_3, Cell{3}},
    })};

    Check(
        free_cell.can_move_to_tableau(moving_bottom, Column::Fourth),
        "Stack movement follows the standard FreeCell capacity rule");
}

void TestWinAndInvalidCases() {
    const auto win = FreeCell{AllFoundationState()};
    Check(win.is_win(), "All-foundation state is a win");

    bool invalid_state = false;
    try {
        auto positions = AllFoundationState();
        positions.erase(Card::of(Suit::Clubs, Rank::King));
        positions.emplace(Card::from_id(CardId::Joker), Foundation{});
        static_cast<void>(FreeCell{std::move(positions)});
    } catch (const std::invalid_argument&) {
        invalid_state = true;
    }
    Check(invalid_state, "FreeCell rejects joker states");

    const auto blocked = Card::of(Suit::Spades, Rank::Four);
    const auto covering = Card::of(Suit::Clubs, Rank::Three);
    const auto destination = Card::of(Suit::Hearts, Rank::Five);
    const auto invalid_move_state = FreeCell{MakeState({
        {blocked, Tableau{Column::First, 0}},
        {covering, Tableau{Column::First, 1}},
        {destination, Tableau{Column::Second, 0}},
    })};

    bool invalid_move = false;
    try {
        static_cast<void>(invalid_move_state.move_to_cell(blocked));
    } catch (const std::logic_error&) {
        invalid_move = true;
    }
    Check(invalid_move, "Illegal moves throw");
}

}  // namespace

int main() {
    TestDealBuildsInitialState();
    TestMoveToCellReturnsNewState();
    TestMoveToFoundationFromCellAndTableau();
    TestMoveFromCellToTableau();
    TestMoveTableauStack();
    TestStandardStackMoveCapacityRule();
    TestWinAndInvalidCases();

    if (g_failures != 0) {
        std::cerr << g_failures << " test(s) failed.\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
