#pragma once

#include <compare>
#include <cstdint>
#include <stdexcept>
#include <type_traits>

#include "shlab/gopc/games/golf/lane.hpp"

namespace shlab::gopc::games::golf {

struct Hand {
    int number{};

    auto operator<=>(const Hand&) const noexcept = default;
};

struct Deck {
    int number{};

    auto operator<=>(const Deck&) const noexcept = default;
};

struct Field {
    Lane lane{};
    int number{};

    auto operator<=>(const Field&) const noexcept = default;
};

class Position {
public:
    enum class Kind : std::uint8_t {
        Hand,
        Deck,
        Field,
    };

    constexpr Position() noexcept = default;

    constexpr Position(Hand value) noexcept
        : kind_(Kind::Hand) {
        storage_.hand = value;
    }

    constexpr Position(Deck value) noexcept
        : kind_(Kind::Deck) {
        storage_.deck = value;
    }

    constexpr Position(Field value) noexcept
        : kind_(Kind::Field) {
        storage_.field = value;
    }
    constexpr Position(const Position&) noexcept = default;
    constexpr Position(Position&&) noexcept = default;
    constexpr Position& operator=(const Position&) noexcept = default;
    constexpr Position& operator=(Position&&) noexcept = default;
    ~Position() = default;

    [[nodiscard]] constexpr Kind kind() const noexcept {
        return kind_;
    }

    template <typename T>
    [[nodiscard]] constexpr bool is() const noexcept {
        if constexpr (std::is_same_v<T, Hand>) {
            return kind_ == Kind::Hand;
        }
        else if constexpr (std::is_same_v<T, Deck>) {
            return kind_ == Kind::Deck;
        }
        else if constexpr (std::is_same_v<T, Field>) {
            return kind_ == Kind::Field;
        }
        else {
            return false;
        }
    }

    template <typename T>
    [[nodiscard]] constexpr T* get_if() noexcept {
        if constexpr (std::is_same_v<T, Hand>) {
            return kind_ == Kind::Hand ? &storage_.hand : nullptr;
        }
        else if constexpr (std::is_same_v<T, Deck>) {
            return kind_ == Kind::Deck ? &storage_.deck : nullptr;
        }
        else if constexpr (std::is_same_v<T, Field>) {
            return kind_ == Kind::Field ? &storage_.field : nullptr;
        }
        else {
            return nullptr;
        }
    }

    template <typename T>
    [[nodiscard]] constexpr const T* get_if() const noexcept {
        if constexpr (std::is_same_v<T, Hand>) {
            return kind_ == Kind::Hand ? &storage_.hand : nullptr;
        }
        else if constexpr (std::is_same_v<T, Deck>) {
            return kind_ == Kind::Deck ? &storage_.deck : nullptr;
        }
        else if constexpr (std::is_same_v<T, Field>) {
            return kind_ == Kind::Field ? &storage_.field : nullptr;
        }
        else {
            return nullptr;
        }
    }

    template <typename T>
    [[nodiscard]] T& get() {
        if (auto* value = get_if<T>(); value != nullptr) {
            return *value;
        }

        throw std::logic_error("Position does not hold the requested type.");
    }

    template <typename T>
    [[nodiscard]] const T& get() const {
        if (const auto* value = get_if<T>(); value != nullptr) {
            return *value;
        }

        throw std::logic_error("Position does not hold the requested type.");
    }

    friend constexpr bool operator==(const Position& lhs, const Position& rhs) noexcept {
        if (lhs.kind_ != rhs.kind_) {
            return false;
        }

        switch (lhs.kind_) {
        case Kind::Hand:
            return lhs.storage_.hand == rhs.storage_.hand;
        case Kind::Deck:
            return lhs.storage_.deck == rhs.storage_.deck;
        case Kind::Field:
            return lhs.storage_.field == rhs.storage_.field;
        }

        return false;
    }

private:
    union Storage {
        Hand hand;
        Deck deck;
        Field field;

        constexpr Storage() noexcept
            : hand{} {}
    };

    Kind kind_{Kind::Hand};
    Storage storage_{};
};

template <typename T>
[[nodiscard]] constexpr bool HoldsAlternative(const Position& position) noexcept {
    return position.template is<T>();
}

template <typename T>
[[nodiscard]] constexpr T* GetIf(Position* position) noexcept {
    return position == nullptr ? nullptr : position->template get_if<T>();
}

template <typename T>
[[nodiscard]] constexpr const T* GetIf(const Position* position) noexcept {
    return position == nullptr ? nullptr : position->template get_if<T>();
}

template <typename T>
[[nodiscard]] T& Get(Position& position) {
    return position.template get<T>();
}

template <typename T>
[[nodiscard]] const T& Get(const Position& position) {
    return position.template get<T>();
}

template <typename Visitor>
constexpr decltype(auto) visit(const Position& pos, Visitor&& vis) {
    switch (pos.kind()) {
    case Position::Kind::Hand:
        return vis(pos.get<Hand>());
    case Position::Kind::Deck:
        return vis(pos.get<Deck>());
    case Position::Kind::Field:
        return vis(pos.get<Field>());
    }
    __builtin_unreachable();
}

template <typename Visitor>
constexpr decltype(auto) visit(Position& pos, Visitor&& vis) {
    switch (pos.kind()) {
    case Position::Kind::Hand:
        return vis(pos.get<Hand>());
    case Position::Kind::Deck:
        return vis(pos.get<Deck>());
    case Position::Kind::Field:
        return vis(pos.get<Field>());
    }
    __builtin_unreachable();
}

}  // namespace shlab::gopc::games::golf
