cmake_minimum_required(VERSION 3.21)

project(GamesOfPlayingCards LANGUAGES CXX)

include(CTest)

add_library(games_of_playing_cards
    src/shlab/gopc/playing_cards/card.cpp
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

    target_link_libraries(playing_cards_card_tests
        PRIVATE
            games_of_playing_cards
    )

    add_test(NAME playing_cards_card_tests COMMAND playing_cards_card_tests)
endif()
