#include "shlab/gopc/games/pyramid.hpp"

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

using shlab::gopc::games::pyramid::Deck;
using shlab::gopc::games::pyramid::Discard;
using shlab::gopc::games::pyramid::Field;
using shlab::gopc::games::pyramid::FreeSpace;
using shlab::gopc::games::pyramid::Hand;
using shlab::gopc::games::pyramid::Outside;
using shlab::gopc::games::pyramid::Position;
using shlab::gopc::games::pyramid::Pyramid;
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

std::vector<Card> PyramidDeck() {
    auto deck = StandardDeck();
    deck.push_back(Card::from_id(CardId::Joker));
    deck.push_back(Card::from_id(CardId::ExtraJoker));
    return deck;
}

Pyramid::state_type MakeState(const std::vector<std::pair<Card, Position>>& fixed_positions) {
    Pyramid::state_type positions;
    positions.reserve(54);

    std::unordered_set<Card> assigned;
    assigned.reserve(54);

    bool free_space_0_used = false;
    bool free_space_1_used = false;

    for (const auto& [card, position] : fixed_positions) {
        const auto [unused_it, inserted] = assigned.insert(card);
        (void)unused_it;
        if (!inserted) {
            throw std::invalid_argument("Duplicate fixed card.");
        }

        if (const auto* free_space = GetIf<FreeSpace>(&position); free_space != nullptr) {
            if (free_space->number == 0) {
                free_space_0_used = true;
            } else if (free_space->number == 1) {
                free_space_1_used = true;
            }
        }

        positions.emplace(card, position);
    }

    const auto joker = Card::from_id(CardId::Joker);
    if (!assigned.contains(joker)) {
        if (free_space_0_used) {
            positions.emplace(joker, Outside{});
        } else {
            positions.emplace(joker, FreeSpace{0});
            free_space_0_used = true;
        }
    }

    const auto extra_joker = Card::from_id(CardId::ExtraJoker);
    if (!assigned.contains(extra_joker)) {
        if (free_space_1_used) {
            positions.emplace(extra_joker, Outside{});
        } else {
            positions.emplace(extra_joker, FreeSpace{1});
        }
    }

    for (const auto& card : StandardDeck()) {
        if (!assigned.contains(card)) {
            positions.emplace(card, Outside{});
        }
    }

    return positions;
}

void TestDealBuildsInitialState() {
    const auto deck = PyramidDeck();
    const auto pyramid = Pyramid::deal(deck);

    CheckEqual(pyramid.card_positions().size(), std::size_t{54}, "Deal keeps all cards");
    CheckEqual(pyramid.deck_count(), std::size_t{24}, "Deal creates 24 deck cards");
    Check(!pyramid.is_win(), "Initial deal is not a win");

    CheckEqual(Get<Field>(pyramid.position_of(deck[0])), Field{0, 0}, "First card starts at the pyramid top");
    CheckEqual(Get<Field>(pyramid.position_of(deck[1])), Field{1, 0}, "Second card starts the second row");
    CheckEqual(Get<Field>(pyramid.position_of(deck[2])), Field{1, 1}, "Third card completes the second row");
    CheckEqual(Get<Field>(pyramid.position_of(deck[27])), Field{6, 6}, "Twenty-eighth card ends the seventh row");
    CheckEqual(Get<Deck>(pyramid.position_of(deck[28])), Deck{0}, "Next card becomes the deck bottom");
    CheckEqual(Get<Deck>(pyramid.position_of(deck[51])), Deck{23}, "Last standard card becomes the deck top");
    CheckEqual(
        Get<FreeSpace>(pyramid.position_of(Card::from_id(CardId::Joker))),
        FreeSpace{0},
        "Joker starts in the first free space");
    CheckEqual(
        Get<FreeSpace>(pyramid.position_of(Card::from_id(CardId::ExtraJoker))),
        FreeSpace{1},
        "Extra joker starts in the second free space");
}

void TestPairAndSingleRemove() {
    const auto king = Card::of(Suit::Spades, Rank::King);
    const auto ace = Card::of(Suit::Clubs, Rank::Ace);
    const auto queen = Card::of(Suit::Clubs, Rank::Queen);
    const auto pyramid = Pyramid{MakeState({
        {king, Field{0, 0}},
        {ace, Field{1, 0}},
        {queen, Field{1, 1}},
    })};

    Check(pyramid.can_remove(ace, queen), "Exposed cards summing to 13 can be removed as a pair");
    Check(!pyramid.can_remove(king), "Covered king cannot be removed alone");

    const auto after_pair = pyramid.remove(ace, queen);
    Check(HoldsAlternative<Outside>(after_pair.position_of(ace)), "First removed card moves outside");
    Check(HoldsAlternative<Outside>(after_pair.position_of(queen)), "Second removed card moves outside");
    Check(HoldsAlternative<Field>(pyramid.position_of(ace)), "Original state stays unchanged after pair removal");
    Check(after_pair.can_remove(king), "Removing the covering cards exposes the king");

    const auto after_king = after_pair.remove(king);
    Check(HoldsAlternative<Outside>(after_king.position_of(king)), "Single removable king moves outside");
}

void TestJokerWildcardRemoval() {
    const auto joker = Card::from_id(CardId::Joker);
    const auto four = Card::of(Suit::Hearts, Rank::Four);
    const auto pyramid = Pyramid{MakeState({
        {joker, FreeSpace{0}},
        {four, Hand{}},
    })};

    Check(pyramid.can_select_for_remove(joker), "Joker in free space can be selected for removal");
    Check(pyramid.can_remove(joker, four), "Joker acts as a wildcard for pair removal");

    const auto next = pyramid.remove(joker, four);
    Check(HoldsAlternative<Outside>(next.position_of(joker)), "Joker pair removal moves the joker outside");
    Check(HoldsAlternative<Outside>(next.position_of(four)), "Joker pair removal moves the partner outside");
}

void TestDrawMovesPreviousHandToDiscard() {
    const auto first = Card::of(Suit::Clubs, Rank::Jack);
    const auto second = Card::of(Suit::Spades, Rank::Queen);
    const auto hand = Card::of(Suit::Hearts, Rank::Ace);
    const auto pyramid = Pyramid{MakeState({
        {first, Deck{0}},
        {second, Deck{1}},
        {hand, Hand{}},
    })};

    Check(!pyramid.can_draw(first), "Only the top deck card can be drawn");
    Check(pyramid.can_draw(second), "Top deck card can be drawn");

    const auto next = pyramid.draw(second);

    CheckEqual(Get<Hand>(next.position_of(second)), Hand{}, "Drawn card becomes the new hand");
    CheckEqual(Get<Discard>(next.position_of(hand)), Discard{0}, "Previous hand moves to the discard pile");
    CheckEqual(Get<Deck>(pyramid.position_of(second)), Deck{1}, "Original state is unchanged after drawing");
}

void TestRedealReversesDiscardIntoDeck() {
    const auto first = Card::of(Suit::Clubs, Rank::Queen);
    const auto second = Card::of(Suit::Spades, Rank::Jack);
    const auto pyramid = Pyramid{MakeState({
        {first, Discard{0}},
        {second, Discard{1}},
    })};

    Check(pyramid.can_redeal(), "Redeal is allowed when the deck is empty");

    const auto redealt = pyramid.redeal();
    CheckEqual(Get<Deck>(redealt.position_of(second)), Deck{0}, "Top discard becomes the new deck bottom");
    CheckEqual(Get<Deck>(redealt.position_of(first)), Deck{1}, "Bottom discard becomes the new deck top");
}

void TestWinAndInvalidCases() {
    const auto win = Pyramid{MakeState({})};
    Check(win.is_win(), "A state with no field cards is a win");

    bool invalid_state = false;
    try {
        auto positions = MakeState({
            {Card::of(Suit::Clubs, Rank::King), FreeSpace{0}},
        });
        static_cast<void>(Pyramid{std::move(positions)});
    } catch (const std::invalid_argument&) {
        invalid_state = true;
    }
    Check(invalid_state, "Standard cards cannot occupy FreeSpace");

    const auto queen = Card::of(Suit::Diamonds, Rank::Queen);
    const auto invalid_move_state = Pyramid{MakeState({
        {queen, Hand{}},
    })};

    bool invalid_move = false;
    try {
        static_cast<void>(invalid_move_state.remove(queen));
    } catch (const std::logic_error&) {
        invalid_move = true;
    }
    Check(invalid_move, "Illegal single-card removals throw");
}

}  // namespace

int main() {
    TestDealBuildsInitialState();
    TestPairAndSingleRemove();
    TestJokerWildcardRemoval();
    TestDrawMovesPreviousHandToDiscard();
    TestRedealReversesDiscardIntoDeck();
    TestWinAndInvalidCases();

    if (g_failures != 0) {
        std::cerr << g_failures << " test(s) failed.\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
