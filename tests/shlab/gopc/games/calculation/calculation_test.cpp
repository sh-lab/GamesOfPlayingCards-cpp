#include "shlab/gopc/games/calculation.hpp"

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

using shlab::gopc::games::calculation::Calculation;
using shlab::gopc::games::calculation::Foundation;
using shlab::gopc::games::calculation::FoundationColumn;
using shlab::gopc::games::calculation::Position;
using shlab::gopc::games::calculation::Stock;
using shlab::gopc::games::calculation::Tableau;
using shlab::gopc::games::calculation::TableauColumn;
using shlab::gopc::games::calculation::WastePile;
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

Calculation::state_type AllFoundationState() {
    Calculation::state_type positions;
    positions.reserve(52);

    const auto deck = StandardDeck();
    std::unordered_set<Card> assigned;
    assigned.reserve(deck.size());

    const auto add_by_rank = [&](const Rank rank, const FoundationColumn column) {
        for (const auto& card : deck) {
            if (card.rank() == rank && !assigned.contains(card)) {
                assigned.insert(card);
                positions.emplace(card, Foundation{column});
                return;
            }
        }
        throw std::invalid_argument("Missing rank for foundation sequence.");
    };

    constexpr std::array first_sequence{
        Rank::Ace, Rank::Two, Rank::Three, Rank::Four, Rank::Five, Rank::Six, Rank::Seven,
        Rank::Eight, Rank::Nine, Rank::Ten, Rank::Jack, Rank::Queen, Rank::King,
    };
    constexpr std::array second_sequence{
        Rank::Two, Rank::Four, Rank::Six, Rank::Eight, Rank::Ten, Rank::Queen, Rank::Ace,
        Rank::Three, Rank::Five, Rank::Seven, Rank::Nine, Rank::Jack, Rank::King,
    };
    constexpr std::array third_sequence{
        Rank::Three, Rank::Six, Rank::Nine, Rank::Queen, Rank::Two, Rank::Five, Rank::Eight,
        Rank::Jack, Rank::Ace, Rank::Four, Rank::Seven, Rank::Ten, Rank::King,
    };
    constexpr std::array fourth_sequence{
        Rank::Four, Rank::Eight, Rank::Queen, Rank::Three, Rank::Seven, Rank::Jack, Rank::Two,
        Rank::Six, Rank::Ten, Rank::Ace, Rank::Five, Rank::Nine, Rank::King,
    };

    for (const auto rank : first_sequence) {
        add_by_rank(rank, FoundationColumn::First);
    }
    for (const auto rank : second_sequence) {
        add_by_rank(rank, FoundationColumn::Second);
    }
    for (const auto rank : third_sequence) {
        add_by_rank(rank, FoundationColumn::Third);
    }
    for (const auto rank : fourth_sequence) {
        add_by_rank(rank, FoundationColumn::Fourth);
    }

    return positions;
}

Calculation::state_type MakeState(
    const std::vector<std::pair<Card, Position>>& fixed_positions,
    const TableauColumn parking_column = TableauColumn::Fourth,
    const bool add_default_waste = true) {
    Calculation::state_type positions;
    positions.reserve(52);

    std::unordered_set<Card> assigned;
    assigned.reserve(52);
    std::array<std::vector<int>, 4> tableau_numbers;
    bool has_waste = false;
    std::vector<int> stock_numbers;

    for (const auto& [card, position] : fixed_positions) {
        const auto [unused_it, inserted] = assigned.insert(card);
        (void)unused_it;
        if (!inserted) {
            throw std::invalid_argument("Duplicate fixed card.");
        }

        if (const auto* tableau = GetIf<Tableau>(&position); tableau != nullptr) {
            tableau_numbers[static_cast<std::size_t>(tableau->column)].push_back(tableau->number);
        } else if (HoldsAlternative<WastePile>(position)) {
            has_waste = true;
        } else if (const auto* stock = GetIf<Stock>(&position); stock != nullptr) {
            stock_numbers.push_back(stock->number);
        }

        positions.emplace(card, position);
    }

    auto& parking_numbers = tableau_numbers[static_cast<std::size_t>(parking_column)];
    std::sort(parking_numbers.begin(), parking_numbers.end());
    for (std::size_t i = 0; i < parking_numbers.size(); ++i) {
        if (parking_numbers[i] != static_cast<int>(i)) {
            throw std::invalid_argument("Parking tableau column must be contiguous.");
        }
    }

    std::sort(stock_numbers.begin(), stock_numbers.end());
    for (std::size_t i = 0; i < stock_numbers.size(); ++i) {
        if (stock_numbers[i] != static_cast<int>(i)) {
            throw std::invalid_argument("Fixed stock positions must be contiguous.");
        }
    }

    int stock_number = static_cast<int>(stock_numbers.size());
    int parking_number = static_cast<int>(parking_numbers.size());
    for (const auto& card : StandardDeck()) {
        if (!assigned.contains(card)) {
            if (add_default_waste && !has_waste) {
                positions.emplace(card, WastePile{});
                has_waste = true;
            } else if (add_default_waste) {
                positions.emplace(card, Stock{stock_number++});
            } else {
                positions.emplace(card, Tableau{parking_column, parking_number++});
            }
        }
    }

    return positions;
}

void TestDealBuildsInitialState() {
    const auto deck = StandardDeck();
    const auto calculation = Calculation::deal(deck);

    CheckEqual(calculation.card_positions().size(), std::size_t{52}, "Deal keeps all cards");
    CheckEqual(calculation.stock_count(), std::size_t{47}, "Deal creates 47 stock cards");
    Check(!calculation.is_win(), "Initial deal is not a win");

    CheckEqual(calculation.next_rank(FoundationColumn::First), Rank::Two, "First foundation starts from ace");
    CheckEqual(calculation.next_rank(FoundationColumn::Second), Rank::Four, "Second foundation starts from two");
    CheckEqual(calculation.next_rank(FoundationColumn::Third), Rank::Six, "Third foundation starts from three");
    CheckEqual(calculation.next_rank(FoundationColumn::Fourth), Rank::Eight, "Fourth foundation starts from four");

    Check(HoldsAlternative<WastePile>(calculation.position_of(deck[4])), "First remaining card becomes waste");
}

void TestDrawReturnsNewState() {
    const auto top_stock = Card::of(Suit::Clubs, Rank::King);
    const auto blocked_stock = Card::of(Suit::Clubs, Rank::Queen);
    const auto calculation = Calculation{MakeState({
        {Card::of(Suit::Clubs, Rank::Ace), Foundation{FoundationColumn::First}},
        {Card::of(Suit::Clubs, Rank::Two), Foundation{FoundationColumn::Second}},
        {Card::of(Suit::Clubs, Rank::Three), Foundation{FoundationColumn::Third}},
        {Card::of(Suit::Clubs, Rank::Four), Foundation{FoundationColumn::Fourth}},
        {blocked_stock, Stock{0}},
        {top_stock, Stock{1}},
    }, TableauColumn::Fourth, false)};

    Check(calculation.can_draw(top_stock), "Top stock card can be drawn when waste is empty");
    Check(!calculation.can_draw(blocked_stock), "Only the top stock card can be drawn");

    const auto next = calculation.draw(top_stock);
    CheckEqual(Get<Stock>(calculation.position_of(top_stock)), Stock{1}, "Original state is unchanged after draw");
    Check(HoldsAlternative<WastePile>(next.position_of(top_stock)), "Draw moves the card to waste");
}

void TestWasteMoveToTableauReturnsNewState() {
    const auto waste = Card::of(Suit::Hearts, Rank::Nine);
    const auto calculation = Calculation{MakeState({
        {Card::of(Suit::Clubs, Rank::Ace), Foundation{FoundationColumn::First}},
        {Card::of(Suit::Clubs, Rank::Two), Foundation{FoundationColumn::Second}},
        {Card::of(Suit::Clubs, Rank::Three), Foundation{FoundationColumn::Third}},
        {Card::of(Suit::Clubs, Rank::Four), Foundation{FoundationColumn::Fourth}},
        {waste, WastePile{}},
    })};

    Check(calculation.can_move_to_tableau(waste), "Waste card can move to tableau");

    const auto next = calculation.move_to_tableau(waste, TableauColumn::Second);
    Check(HoldsAlternative<WastePile>(calculation.position_of(waste)), "Original state is unchanged after tableau move");
    CheckEqual(Get<Tableau>(next.position_of(waste)), Tableau{TableauColumn::Second, 0}, "Waste card becomes the tableau top");
}

void TestMoveToFoundationFromWasteAndTableau() {
    const auto ace = Card::of(Suit::Spades, Rank::Ace);
    const auto two = Card::of(Suit::Hearts, Rank::Two);

    const auto from_waste = Calculation{MakeState({
        {ace, Foundation{FoundationColumn::First}},
        {Card::of(Suit::Clubs, Rank::Two), Foundation{FoundationColumn::Second}},
        {Card::of(Suit::Clubs, Rank::Three), Foundation{FoundationColumn::Third}},
        {Card::of(Suit::Clubs, Rank::Four), Foundation{FoundationColumn::Fourth}},
        {two, WastePile{}},
    })};

    Check(
        from_waste.can_move_to_foundation(two, FoundationColumn::First),
        "Waste card can move to foundation when it matches the next rank");

    const auto waste_next = from_waste.move_to_foundation(two, FoundationColumn::First);
    CheckEqual(
        Get<Foundation>(waste_next.position_of(two)),
        Foundation{FoundationColumn::First},
        "Waste card moves to the requested foundation column");

    const auto tableau_top = Card::of(Suit::Diamonds, Rank::Four);
    const auto tableau_blocker = Card::of(Suit::Diamonds, Rank::King);
    const auto from_tableau = Calculation{MakeState({
        {Card::of(Suit::Clubs, Rank::Ace), Foundation{FoundationColumn::First}},
        {Card::of(Suit::Clubs, Rank::Two), Foundation{FoundationColumn::Second}},
        {Card::of(Suit::Clubs, Rank::Three), Foundation{FoundationColumn::Third}},
        {tableau_blocker, Tableau{TableauColumn::First, 0}},
        {tableau_top, Tableau{TableauColumn::First, 1}},
    })};

    Check(
        from_tableau.can_move_to_foundation(tableau_top, FoundationColumn::Fourth),
        "Top tableau card can move to foundation when it matches the next rank");
    Check(
        !from_tableau.can_move_to_foundation(tableau_blocker, FoundationColumn::Fourth),
        "Non-top tableau cards cannot move to foundation");
}

void TestQueriesForWinAndNextRank() {
    const auto win = Calculation{AllFoundationState()};
    Check(win.is_win(), "All-foundation state is a win");
    Check(!win.next_rank(FoundationColumn::First).has_value(), "Completed foundation has no next rank");
}

void TestInvalidStateAndInvalidMove() {
    bool invalid_joker = false;
    try {
        Calculation::state_type positions;
        positions.reserve(52);
        positions.emplace(Card::from_id(CardId::Joker), WastePile{});
        for (const auto& card : StandardDeck()) {
            if (positions.size() == 52) {
                break;
            }
            positions.emplace(card, Stock{static_cast<int>(positions.size() - 1)});
        }
        static_cast<void>(Calculation{std::move(positions)});
    } catch (const std::exception&) {
        invalid_joker = true;
    }
    Check(invalid_joker, "Calculation joker states remain invalid");

    const auto waste = Card::of(Suit::Hearts, Rank::Nine);
    const auto blocked_draw = Card::of(Suit::Hearts, Rank::Ten);
    const auto calculation = Calculation{MakeState({
        {Card::of(Suit::Clubs, Rank::Ace), Foundation{FoundationColumn::First}},
        {Card::of(Suit::Clubs, Rank::Two), Foundation{FoundationColumn::Second}},
        {Card::of(Suit::Clubs, Rank::Three), Foundation{FoundationColumn::Third}},
        {Card::of(Suit::Clubs, Rank::Four), Foundation{FoundationColumn::Fourth}},
        {waste, WastePile{}},
        {blocked_draw, Stock{0}},
    })};

    bool invalid_move = false;
    try {
        static_cast<void>(calculation.draw(blocked_draw));
    } catch (const std::logic_error&) {
        invalid_move = true;
    }
    Check(invalid_move, "Illegal draws throw");
}

}  // namespace

int main() {
    TestDealBuildsInitialState();
    TestDrawReturnsNewState();
    TestWasteMoveToTableauReturnsNewState();
    TestMoveToFoundationFromWasteAndTableau();
    TestQueriesForWinAndNextRank();
    TestInvalidStateAndInvalidMove();

    if (g_failures != 0) {
        std::cerr << g_failures << " test(s) failed.\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
