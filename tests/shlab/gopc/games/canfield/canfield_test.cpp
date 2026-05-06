#include "shlab/gopc/games/canfield.hpp"

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

using shlab::gopc::games::canfield::Canfield;
using shlab::gopc::games::canfield::Column;
using shlab::gopc::games::canfield::Foundation;
using shlab::gopc::games::canfield::Position;
using shlab::gopc::games::canfield::Reserve;
using shlab::gopc::games::canfield::Stock;
using shlab::gopc::games::canfield::Tableau;
using shlab::gopc::games::canfield::WastePile;
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

Rank AdvanceRank(const Rank rank, const int steps) noexcept {
    const auto zero_based = static_cast<int>(rank) - 1;
    return static_cast<Rank>((zero_based + steps) % 13 + 1);
}

Canfield::state_type AllFoundationState(const Rank foundation_base_rank) {
    Canfield::state_type positions;
    positions.reserve(52);

    std::unordered_set<Card> assigned;
    assigned.reserve(52);

    constexpr std::array suits{
        Suit::Spades,
        Suit::Hearts,
        Suit::Diamonds,
        Suit::Clubs,
    };

    for (const auto suit : suits) {
        for (int i = 0; i < 13; ++i) {
            const auto card = Card::of(suit, AdvanceRank(foundation_base_rank, i));
            assigned.insert(card);
            positions.emplace(card, Foundation{});
        }
    }

    return positions;
}

Canfield::state_type MakeState(
    const std::vector<std::pair<Card, Position>>& fixed_positions,
    const Rank foundation_base_rank,
    const bool fill_remaining_with_stock = true) {
    Canfield::state_type positions;
    positions.reserve(52);

    std::unordered_set<Card> assigned;
    assigned.reserve(52);
    std::array<std::vector<int>, 4> tableau_numbers;
    std::vector<int> stock_numbers;
    std::vector<int> waste_numbers;
    std::vector<int> reserve_numbers;

    for (const auto& [card, position] : fixed_positions) {
        const auto [unused_it, inserted] = assigned.insert(card);
        (void)unused_it;
        if (!inserted) {
            throw std::invalid_argument("Duplicate fixed card.");
        }

        if (const auto* tableau = GetIf<Tableau>(&position); tableau != nullptr) {
            tableau_numbers[static_cast<std::size_t>(tableau->column)].push_back(tableau->number);
        } else if (const auto* stock = GetIf<Stock>(&position); stock != nullptr) {
            stock_numbers.push_back(stock->number);
        } else if (const auto* waste = GetIf<WastePile>(&position); waste != nullptr) {
            waste_numbers.push_back(waste->number);
        } else if (const auto* reserve = GetIf<Reserve>(&position); reserve != nullptr) {
            reserve_numbers.push_back(reserve->number);
        }

        positions.emplace(card, position);
    }

    for (auto& numbers : tableau_numbers) {
        std::sort(numbers.begin(), numbers.end());
        for (std::size_t i = 0; i < numbers.size(); ++i) {
            if (numbers[i] != static_cast<int>(i)) {
                throw std::invalid_argument("Fixed tableau positions must be contiguous.");
            }
        }
    }

    std::sort(stock_numbers.begin(), stock_numbers.end());
    std::sort(waste_numbers.begin(), waste_numbers.end());
    std::sort(reserve_numbers.begin(), reserve_numbers.end());

    for (std::size_t i = 0; i < stock_numbers.size(); ++i) {
        if (stock_numbers[i] != static_cast<int>(i)) {
            throw std::invalid_argument("Fixed stock positions must be contiguous.");
        }
    }
    for (std::size_t i = 0; i < waste_numbers.size(); ++i) {
        if (waste_numbers[i] != static_cast<int>(i)) {
            throw std::invalid_argument("Fixed waste positions must be contiguous.");
        }
    }
    for (std::size_t i = 0; i < reserve_numbers.size(); ++i) {
        if (reserve_numbers[i] != static_cast<int>(i)) {
            throw std::invalid_argument("Fixed reserve positions must be contiguous.");
        }
    }

    int next_stock = static_cast<int>(stock_numbers.size());
    std::array<int, 4> next_tableau_numbers{};
    for (std::size_t i = 0; i < tableau_numbers.size(); ++i) {
        next_tableau_numbers[i] = static_cast<int>(tableau_numbers[i].size());
    }
    std::size_t tableau_fill_index = 0;
    for (const auto& card : StandardDeck()) {
        if (!assigned.contains(card)) {
            if (fill_remaining_with_stock) {
                positions.emplace(card, Stock{next_stock++});
            } else {
                const auto column_index = tableau_fill_index % next_tableau_numbers.size();
                positions.emplace(
                    card,
                    Tableau{static_cast<Column>(column_index), next_tableau_numbers[column_index]++});
                ++tableau_fill_index;
            }
        }
    }
    return positions;
}

void TestDealBuildsInitialState() {
    const auto deck = StandardDeck();
    const auto canfield = Canfield::deal(deck);

    CheckEqual(canfield.card_positions().size(), std::size_t{52}, "Deal keeps all cards");
    CheckEqual(canfield.foundation_base_rank(), deck[13].rank(), "Base rank comes from the foundation base card");
    CheckEqual(canfield.reserve_count(), std::size_t{13}, "Deal creates 13 reserve cards");
    CheckEqual(canfield.stock_count(), std::size_t{34}, "Deal creates 34 stock cards");
    CheckEqual(canfield.waste_count(), std::size_t{0}, "Deal starts with empty waste");
    Check(!canfield.is_win(), "Initial deal is not a win");

    CheckEqual(Get<Reserve>(canfield.position_of(deck[0])), Reserve{0}, "First card starts at reserve bottom");
    CheckEqual(Get<Reserve>(canfield.position_of(deck[12])), Reserve{12}, "Thirteenth card starts at reserve top");
    Check(HoldsAlternative<Foundation>(canfield.position_of(deck[13])), "Next card becomes the foundation base");
    CheckEqual(Get<Tableau>(canfield.position_of(deck[14])), Tableau{Column::First, 0}, "First tableau card is dealt after the base");
    CheckEqual(Get<Tableau>(canfield.position_of(deck[17])), Tableau{Column::Fourth, 0}, "Four tableau columns start with one card each");
    CheckEqual(Get<Stock>(canfield.position_of(deck[51])), Stock{33}, "Last card becomes stock top");
}

void TestDrawThreeCardsAndRedeal() {
    const auto one = Card::of(Suit::Clubs, Rank::Ace);
    const auto two = Card::of(Suit::Clubs, Rank::Two);
    const auto three = Card::of(Suit::Clubs, Rank::Three);
    const auto canfield = Canfield{MakeState({
        {Card::of(Suit::Hearts, Rank::Seven), Foundation{}},
        {Card::of(Suit::Spades, Rank::King), Tableau{Column::First, 0}},
        {Card::of(Suit::Hearts, Rank::King), Tableau{Column::Second, 0}},
        {Card::of(Suit::Diamonds, Rank::King), Tableau{Column::Third, 0}},
        {Card::of(Suit::Clubs, Rank::King), Tableau{Column::Fourth, 0}},
        {one, Stock{0}},
        {two, Stock{1}},
        {three, Stock{2}},
    }, Rank::Seven, false), Rank::Seven};

    Check(canfield.can_draw(), "Stock can be drawn");

    const auto drawn = canfield.draw();
    CheckEqual(drawn.stock_count(), std::size_t{0}, "Draw consumes the available stock");
    CheckEqual(drawn.waste_count(), std::size_t{3}, "Draw exposes up to three waste cards");
    CheckEqual(Get<WastePile>(drawn.position_of(three)), WastePile{0}, "Top stock card is dealt first into waste");
    CheckEqual(Get<WastePile>(drawn.position_of(one)), WastePile{2}, "Third dealt card becomes the waste top");

    Check(drawn.can_redeal(), "Empty stock with waste can redeal");

    const auto redealt = drawn.redeal();
    CheckEqual(redealt.stock_count(), std::size_t{3}, "Redeal restores stock");
    CheckEqual(Get<Stock>(redealt.position_of(one)), Stock{0}, "Waste top becomes stock bottom on redeal");
}

void TestMoveReserveAndWasteToFoundation() {
    const auto reserve_top = Card::of(Suit::Clubs, Rank::Seven);
    const auto waste_top = Card::of(Suit::Hearts, Rank::Eight);
    const auto canfield = Canfield{MakeState({
        {Card::of(Suit::Clubs, Rank::Six), Foundation{}},
        {reserve_top, Reserve{0}},
        {waste_top, WastePile{0}},
        {Card::of(Suit::Spades, Rank::King), Tableau{Column::First, 0}},
        {Card::of(Suit::Hearts, Rank::King), Tableau{Column::Second, 0}},
        {Card::of(Suit::Diamonds, Rank::King), Tableau{Column::Third, 0}},
        {Card::of(Suit::Clubs, Rank::King), Tableau{Column::Fourth, 0}},
    }, Rank::Six), Rank::Six};

    Check(canfield.can_move_to_foundation(reserve_top), "Top reserve card can move to foundation when it matches the next rank");
    Check(!canfield.can_move_to_foundation(waste_top), "Waste card cannot move when it does not match the next suit rank");
}

void TestMoveToTableauAndReserveAutofill() {
    const auto reserve_top = Card::of(Suit::Clubs, Rank::Four);
    const auto moving = Card::of(Suit::Hearts, Rank::Five);
    const auto destination = Card::of(Suit::Spades, Rank::Six);

    const auto canfield = Canfield{MakeState({
        {Card::of(Suit::Diamonds, Rank::Eight), Foundation{}},
        {reserve_top, Reserve{0}},
        {moving, Tableau{Column::First, 0}},
        {destination, Tableau{Column::Second, 0}},
        {Card::of(Suit::Diamonds, Rank::King), Tableau{Column::Third, 0}},
        {Card::of(Suit::Clubs, Rank::King), Tableau{Column::Fourth, 0}},
    }, Rank::Eight), Rank::Eight};

    Check(canfield.can_move_to_tableau(moving, Column::Second), "Top tableau card can move onto alternating descending tableau");

    const auto next = canfield.move_to_tableau(moving, Column::Second);
    CheckEqual(Get<Tableau>(next.position_of(moving)), Tableau{Column::Second, 1}, "Moved tableau card lands on destination top");
    CheckEqual(Get<Tableau>(next.position_of(reserve_top)), Tableau{Column::First, 0}, "Empty tableau is automatically filled from reserve");
}

void TestMoveWholeTableauColumn() {
    const auto destination = Card::of(Suit::Spades, Rank::Seven);
    const auto bottom = Card::of(Suit::Hearts, Rank::Six);
    const auto top = Card::of(Suit::Clubs, Rank::Five);

    const auto canfield = Canfield{MakeState({
        {Card::of(Suit::Diamonds, Rank::Nine), Foundation{}},
        {bottom, Tableau{Column::First, 0}},
        {top, Tableau{Column::First, 1}},
        {destination, Tableau{Column::Second, 0}},
        {Card::of(Suit::Diamonds, Rank::King), Tableau{Column::Third, 0}},
        {Card::of(Suit::Clubs, Rank::King), Tableau{Column::Fourth, 0}},
    }, Rank::Nine), Rank::Nine};

    Check(canfield.can_move_to_tableau(bottom, Column::Second), "A complete valid tableau column can move as a sequence");

    const auto next = canfield.move_to_tableau(bottom, Column::Second);
    CheckEqual(Get<Tableau>(next.position_of(bottom)), Tableau{Column::Second, 1}, "Bottom card moves first");
    CheckEqual(Get<Tableau>(next.position_of(top)), Tableau{Column::Second, 2}, "Entire column tail moves in order");
}

void TestWinAndInvalidCases() {
    const auto win = Canfield{AllFoundationState(Rank::Five), Rank::Five};
    Check(win.is_win(), "All-foundation state is a win");

    bool invalid_state = false;
    try {
        auto positions = MakeState({
            {Card::of(Suit::Clubs, Rank::Ace), Foundation{}},
        }, Rank::Five);
        positions.erase(Card::of(Suit::Clubs, Rank::King));
        positions.emplace(Card::from_id(CardId::Joker), Stock{0});
        static_cast<void>(Canfield{std::move(positions), Rank::Five});
    } catch (const std::exception&) {
        invalid_state = true;
    }
    Check(invalid_state, "Canfield joker states remain invalid");

    const auto canfield = Canfield{MakeState({
        {Card::of(Suit::Spades, Rank::Seven), Foundation{}},
        {Card::of(Suit::Spades, Rank::King), Tableau{Column::First, 0}},
        {Card::of(Suit::Hearts, Rank::King), Tableau{Column::Second, 0}},
        {Card::of(Suit::Diamonds, Rank::King), Tableau{Column::Third, 0}},
        {Card::of(Suit::Clubs, Rank::King), Tableau{Column::Fourth, 0}},
    }, Rank::Seven, false), Rank::Seven};

    bool invalid_move = false;
    try {
        static_cast<void>(canfield.draw());
    } catch (const std::logic_error&) {
        invalid_move = true;
    }
    Check(invalid_move, "Illegal draw throws");
}

}  // namespace

int main() {
    TestDealBuildsInitialState();
    TestDrawThreeCardsAndRedeal();
    TestMoveReserveAndWasteToFoundation();
    TestMoveToTableauAndReserveAutofill();
    TestMoveWholeTableauColumn();
    TestWinAndInvalidCases();

    if (g_failures != 0) {
        std::cerr << g_failures << " test(s) failed.\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
