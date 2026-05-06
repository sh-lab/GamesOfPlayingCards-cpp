#include "shlab/gopc/games/klondike.hpp"

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

using shlab::gopc::games::klondike::Column;
using shlab::gopc::games::klondike::Foundation;
using shlab::gopc::games::klondike::Klondike;
using shlab::gopc::games::klondike::Position;
using shlab::gopc::games::klondike::Stock;
using shlab::gopc::games::klondike::Tableau;
using shlab::gopc::games::klondike::WastePile;
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

Klondike::state_type MakeState(
    const std::vector<std::pair<Card, Position>>& fixed_positions,
    const std::vector<Card>& stock_tail = {}) {
    Klondike::state_type positions;
    positions.reserve(52);

    std::unordered_set<Card> assigned;
    assigned.reserve(52);

    for (const auto& [card, position] : fixed_positions) {
        const auto [unused_it, inserted] = assigned.insert(card);
        (void)unused_it;
        if (!inserted) {
            throw std::invalid_argument("Duplicate fixed card.");
        }
        positions.emplace(card, position);
    }

    for (const auto& card : stock_tail) {
        const auto [unused_it, inserted] = assigned.insert(card);
        (void)unused_it;
        if (!inserted) {
            throw std::invalid_argument("Duplicate stock card.");
        }
    }

    int stock_number = 0;
    for (const auto& card : StandardDeck()) {
        if (!assigned.contains(card)) {
            positions.emplace(card, Stock{stock_number++});
        }
    }

    for (const auto& card : stock_tail) {
        positions.emplace(card, Stock{stock_number++});
    }

    return positions;
}

Klondike::state_type AllFoundationState() {
    Klondike::state_type positions;
    positions.reserve(52);

    for (const auto& card : StandardDeck()) {
        positions.emplace(card, Foundation{});
    }

    return positions;
}

void TestDealBuildsInitialState() {
    const auto deck = StandardDeck();
    const auto klondike = Klondike::deal(deck);

    CheckEqual(klondike.card_positions().size(), std::size_t{52}, "Deal keeps all cards");
    CheckEqual(klondike.stock_count(), std::size_t{24}, "Deal creates 24 stock cards");
    Check(!klondike.is_pre_win(), "Initial deal is not pre-win");
    Check(!klondike.is_win(), "Initial deal is not win");

    CheckEqual(
        Get<Tableau>(klondike.position_of(deck[0])),
        Tableau{Column::First, 0, true},
        "First tableau card is open");
    CheckEqual(
        Get<Tableau>(klondike.position_of(deck[1])),
        Tableau{Column::Second, 0, false},
        "Second column starts with a closed card");
    CheckEqual(
        Get<Tableau>(klondike.position_of(deck[2])),
        Tableau{Column::Second, 1, true},
        "Second column top card is open");
    CheckEqual(
        Get<Stock>(klondike.position_of(deck[51])),
        Stock{23},
        "Last undealt card becomes stock top");
}

void TestOpenReturnsNewState() {
    const auto target = Card::of(Suit::Spades, Rank::Ace);
    const auto klondike = Klondike{MakeState({
        {target, Tableau{Column::First, 0, false}},
    })};

    Check(klondike.can_open(target), "Top closed tableau card can be opened");

    const auto opened = klondike.open(target);

    CheckEqual(
        Get<Tableau>(klondike.position_of(target)),
        Tableau{Column::First, 0, false},
        "Open does not mutate the original state");
    CheckEqual(
        Get<Tableau>(opened.position_of(target)),
        Tableau{Column::First, 0, true},
        "Open returns a state with the card face up");
}

void TestDrawReturnsNewState() {
    const auto top_stock = Card::of(Suit::Clubs, Rank::Ace);
    const auto blocked_stock = Card::of(Suit::Spades, Rank::Ace);
    const auto klondike = Klondike{MakeState({}, {top_stock})};

    Check(klondike.can_draw(top_stock), "Top stock card can be drawn");
    Check(!klondike.can_draw(blocked_stock), "Only the stock top can be drawn");

    const auto drawn = klondike.draw(top_stock);

    CheckEqual(
        Get<Stock>(klondike.position_of(top_stock)).number + 1,
        static_cast<int>(klondike.stock_count()),
        "Original stock order is unchanged");
    CheckEqual(
        Get<WastePile>(drawn.position_of(top_stock)),
        WastePile{0},
        "Draw moves the card to the waste pile");
    CheckEqual(drawn.stock_count(), std::size_t{51}, "Draw reduces stock count");
}

void TestMoveToFoundationFromWaste() {
    const auto ace_of_clubs = Card::of(Suit::Clubs, Rank::Ace);
    const auto klondike = Klondike{MakeState({
        {ace_of_clubs, WastePile{0}},
    })};

    Check(klondike.can_move_to_foundation(ace_of_clubs), "Waste ace can move to foundation");

    const auto next = klondike.move_to_foundation(ace_of_clubs);

    Check(HoldsAlternative<Foundation>(next.position_of(ace_of_clubs)), "Ace moved to foundation");
    Check(HoldsAlternative<WastePile>(klondike.position_of(ace_of_clubs)), "Original state is unchanged");
}

void TestRedealFlipsWasteBackIntoStock() {
    Klondike::state_type positions;
    positions.reserve(52);

    int waste_number = 0;
    for (const auto& card : StandardDeck()) {
        positions.emplace(card, WastePile{waste_number++});
    }

    const auto klondike = Klondike{std::move(positions)};
    const auto redealt = klondike.redeal();
    const auto first = StandardDeck()[0];
    const auto last = StandardDeck()[51];

    Check(klondike.can_redeal(), "Waste-only state can be redealt");
    CheckEqual(Get<Stock>(redealt.position_of(last)), Stock{0}, "Waste top becomes stock bottom");
    CheckEqual(Get<Stock>(redealt.position_of(first)), Stock{51}, "Waste bottom becomes stock top");
}

void TestMoveWasteCardToTableau() {
    const auto destination = Card::of(Suit::Hearts, Rank::Five);
    const auto moving = Card::of(Suit::Clubs, Rank::Four);
    const auto klondike = Klondike{MakeState({
        {destination, Tableau{Column::First, 0, true}},
        {moving, WastePile{0}},
    })};

    Check(klondike.can_move_to_tableau(moving, Column::First), "Top waste card can move onto alternating tableau card");

    const auto next = klondike.move_to_tableau(moving, Column::First);

    CheckEqual(
        Get<Tableau>(next.position_of(moving)),
        Tableau{Column::First, 1, true},
        "Waste card lands on top of destination tableau");
}

void TestMoveTableauStack() {
    const auto destination = Card::of(Suit::Diamonds, Rank::Eight);
    const auto moving_bottom = Card::of(Suit::Clubs, Rank::Seven);
    const auto moving_middle = Card::of(Suit::Hearts, Rank::Six);
    const auto moving_top = Card::of(Suit::Spades, Rank::Five);

    const auto klondike = Klondike{MakeState({
        {destination, Tableau{Column::First, 0, true}},
        {moving_bottom, Tableau{Column::Second, 0, true}},
        {moving_middle, Tableau{Column::Second, 1, true}},
        {moving_top, Tableau{Column::Second, 2, true}},
    })};

    Check(klondike.can_move_to_tableau(moving_bottom, Column::First), "A valid open tableau stack can move");

    const auto next = klondike.move_to_tableau(moving_bottom, Column::First);

    CheckEqual(
        Get<Tableau>(next.position_of(moving_bottom)),
        Tableau{Column::First, 1, true},
        "Moved stack keeps the bottom card order");
    CheckEqual(
        Get<Tableau>(next.position_of(moving_middle)),
        Tableau{Column::First, 2, true},
        "Moved stack keeps the middle card order");
    CheckEqual(
        Get<Tableau>(next.position_of(moving_top)),
        Tableau{Column::First, 3, true},
        "Moved stack keeps the top card order");
}

void TestMoveFoundationKingToEmptyTableau() {
    const auto king_of_spades = Card::of(Suit::Spades, Rank::King);
    const auto klondike = Klondike{AllFoundationState()};

    Check(
        klondike.can_move_to_tableau(king_of_spades, Column::First),
        "Top foundation king can move to an empty tableau column");

    const auto next = klondike.move_to_tableau(king_of_spades, Column::First);

    CheckEqual(
        Get<Tableau>(next.position_of(king_of_spades)),
        Tableau{Column::First, 0, true},
        "Foundation king moves onto an empty tableau");
}

void TestQueriesForWinAndPreWin() {
    const auto king_of_spades = Card::of(Suit::Spades, Rank::King);
    auto pre_win_positions = AllFoundationState();
    pre_win_positions[king_of_spades] = Tableau{Column::First, 0, true};
    const auto pre_win = Klondike{std::move(pre_win_positions)};
    const auto win = Klondike{AllFoundationState()};

    Check(pre_win.is_pre_win(), "All foundation or open tableau counts as pre-win");
    Check(!pre_win.is_win(), "Open tableau card prevents win");
    Check(win.is_pre_win(), "Win state is also pre-win");
    Check(win.is_win(), "All foundation is win");
}

void TestInvalidStateAndInvalidMove() {
    bool invalid_state_threw = false;
    try {
        auto positions = MakeState({});
        positions.erase(Card::of(Suit::Clubs, Rank::King));
        positions.emplace(Card::from_id(shlab::gopc::playing_cards::CardId::Joker), Stock{51});
        static_cast<void>(Klondike{std::move(positions)});
    } catch (const std::exception&) {
        invalid_state_threw = true;
    }

    Check(invalid_state_threw, "Klondike joker states remain invalid");

    const auto top_stock = Card::of(Suit::Clubs, Rank::Ace);
    const auto blocked_stock = Card::of(Suit::Spades, Rank::Ace);
    const auto klondike = Klondike{MakeState({}, {top_stock})};

    bool invalid_move_threw = false;
    try {
        static_cast<void>(klondike.draw(blocked_stock));
    } catch (const std::logic_error&) {
        invalid_move_threw = true;
    }

    Check(invalid_move_threw, "Illegal draws throw");
}

}  // namespace

int main() {
    TestDealBuildsInitialState();
    TestOpenReturnsNewState();
    TestDrawReturnsNewState();
    TestMoveToFoundationFromWaste();
    TestRedealFlipsWasteBackIntoStock();
    TestMoveWasteCardToTableau();
    TestMoveTableauStack();
    TestMoveFoundationKingToEmptyTableau();
    TestQueriesForWinAndPreWin();
    TestInvalidStateAndInvalidMove();

    if (g_failures != 0) {
        std::cerr << g_failures << " test(s) failed.\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
