cmake_minimum_required(VERSION 3.21)

project(GamesOfPlayingCards LANGUAGES CXX)

include(CTest)

add_library(games_of_playing_cards
    src/shlab/gopc/games/beleaguered_castle.cpp
    src/shlab/gopc/playing_cards/card.cpp
    src/shlab/gopc/games/canfield.cpp
    src/shlab/gopc/games/calculation.cpp
    src/shlab/gopc/games/freecell.cpp
    src/shlab/gopc/games/four_leaf_clover.cpp
    src/shlab/gopc/games/golf.cpp
    src/shlab/gopc/games/klondike.cpp
    src/shlab/gopc/games/pyramid.cpp
    src/shlab/gopc/games/simple_simon.cpp
    src/shlab/gopc/games/yukon.cpp
)

add_library(sh_lab::games_of_playing_cards ALIAS games_of_playing_cards)

target_compile_features(games_of_playing_cards PUBLIC cxx_std_20)

target_include_directories(games_of_playing_cards
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(BUILD_TESTING)
    add_executable(playing_cards_card_tests
        tests/shlab/gopc/playing_cards/card_test.cpp
    )

    add_executable(klondike_tests
        tests/shlab/gopc/games/klondike/klondike_test.cpp
    )

    add_executable(freecell_tests
        tests/shlab/gopc/games/freecell/freecell_test.cpp
    )

    add_executable(calculation_tests
        tests/shlab/gopc/games/calculation/calculation_test.cpp
    )

    add_executable(canfield_tests
        tests/shlab/gopc/games/canfield/canfield_test.cpp
    )

    add_executable(four_leaf_clover_tests
        tests/shlab/gopc/games/four_leaf_clover/four_leaf_clover_test.cpp
    )

    add_executable(beleaguered_castle_tests
        tests/shlab/gopc/games/beleaguered_castle/beleaguered_castle_test.cpp
    )

    add_executable(yukon_tests
        tests/shlab/gopc/games/yukon/yukon_test.cpp
    )

    add_executable(pyramid_tests
        tests/shlab/gopc/games/pyramid/pyramid_test.cpp
    )

    add_executable(simple_simon_tests
        tests/shlab/gopc/games/simple_simon/simple_simon_test.cpp
    )

    add_executable(golf_tests
        tests/shlab/gopc/games/golf/golf_test.cpp
    )

    target_link_libraries(playing_cards_card_tests
        PRIVATE
            games_of_playing_cards
    )

    target_link_libraries(klondike_tests
        PRIVATE
            games_of_playing_cards
    )

    target_link_libraries(freecell_tests
        PRIVATE
            games_of_playing_cards
    )

    target_link_libraries(calculation_tests
        PRIVATE
            games_of_playing_cards
    )

    target_link_libraries(canfield_tests
        PRIVATE
            games_of_playing_cards
    )

    target_link_libraries(four_leaf_clover_tests
        PRIVATE
            games_of_playing_cards
    )

    target_link_libraries(beleaguered_castle_tests
        PRIVATE
            games_of_playing_cards
    )

    target_link_libraries(yukon_tests
        PRIVATE
            games_of_playing_cards
    )

    target_link_libraries(pyramid_tests
        PRIVATE
            games_of_playing_cards
    )

    target_link_libraries(simple_simon_tests
        PRIVATE
            games_of_playing_cards
    )

    target_link_libraries(golf_tests
        PRIVATE
            games_of_playing_cards
    )

    add_test(NAME playing_cards_card_tests COMMAND playing_cards_card_tests)
    add_test(NAME klondike_tests COMMAND klondike_tests)
    add_test(NAME freecell_tests COMMAND freecell_tests)
    add_test(NAME calculation_tests COMMAND calculation_tests)
    add_test(NAME canfield_tests COMMAND canfield_tests)
    add_test(NAME four_leaf_clover_tests COMMAND four_leaf_clover_tests)
    add_test(NAME beleaguered_castle_tests COMMAND beleaguered_castle_tests)
    add_test(NAME yukon_tests COMMAND yukon_tests)
    add_test(NAME pyramid_tests COMMAND pyramid_tests)
    add_test(NAME simple_simon_tests COMMAND simple_simon_tests)
    add_test(NAME golf_tests COMMAND golf_tests)
endif()
