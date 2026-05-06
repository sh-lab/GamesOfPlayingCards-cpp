#pragma once

#include <compare>
#include <cstdint>
#include <stdexcept>
#include <type_traits>



namespace shlab::gopc::games::pyramid {

struct Deck {
    int number{};

    auto operator<=>(const Deck&) const noexcept = default;
};

struct Discard {
    int number{};

    auto operator<=>(const Discard&) const noexcept = default;
};

struct Field {
    int row{};
    int number{};

    auto operator<=>(const Field&) const noexcept = default;
};

struct Hand {
    auto operator<=>(const Hand&) const noexcept = default;
};

struct FreeSpace {
    int number{};

    auto operator<=>(const FreeSpace&) const noexcept = default;
};

struct Outside {
    auto operator<=>(const Outside&) const noexcept = default;
};

class Position {
public:
    enum class Kind : std::uint8_t {
        Deck,
        Discard,
        Field,
        Hand,
        FreeSpace,
        Outside,
    };

    constexpr Position() noexcept = default;

    constexpr Position(Deck value) noexcept
        : kind_(Kind::Deck) {
        storage_.deck = value;
    }

    constexpr Position(Discard value) noexcept
        : kind_(Kind::Discard) {
        storage_.discard = value;
    }

    constexpr Position(Field value) noexcept
        : kind_(Kind::Field) {
        storage_.field = value;
    }

    constexpr Position(Hand value) noexcept
        : kind_(Kind::Hand) {
        storage_.hand = value;
    }

    constexpr Position(FreeSpace value) noexcept
        : kind_(Kind::FreeSpace) {
        storage_.free_space = value;
    }

    constexpr Position(Outside value) noexcept
        : kind_(Kind::Outside) {
        storage_.outside = value;
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
        if constexpr (std::is_same_v<T, Deck>) {
            return kind_ == Kind::Deck;
        }
        else if constexpr (std::is_same_v<T, Discard>) {
            return kind_ == Kind::Discard;
        }
        else if constexpr (std::is_same_v<T, Field>) {
            return kind_ == Kind::Field;
        }
        else if constexpr (std::is_same_v<T, Hand>) {
            return kind_ == Kind::Hand;
        }
        else if constexpr (std::is_same_v<T, FreeSpace>) {
            return kind_ == Kind::FreeSpace;
        }
        else if constexpr (std::is_same_v<T, Outside>) {
            return kind_ == Kind::Outside;
        }
        else {
            return false;
        }
    }

    template <typename T>
    [[nodiscard]] constexpr T* get_if() noexcept {
        if constexpr (std::is_same_v<T, Deck>) {
            return kind_ == Kind::Deck ? &storage_.deck : nullptr;
        }
        else if constexpr (std::is_same_v<T, Discard>) {
            return kind_ == Kind::Discard ? &storage_.discard : nullptr;
        }
        else if constexpr (std::is_same_v<T, Field>) {
            return kind_ == Kind::Field ? &storage_.field : nullptr;
        }
        else if constexpr (std::is_same_v<T, Hand>) {
            return kind_ == Kind::Hand ? &storage_.hand : nullptr;
        }
        else if constexpr (std::is_same_v<T, FreeSpace>) {
            return kind_ == Kind::FreeSpace ? &storage_.free_space : nullptr;
        }
        else if constexpr (std::is_same_v<T, Outside>) {
            return kind_ == Kind::Outside ? &storage_.outside : nullptr;
        }
        else {
            return nullptr;
        }
    }

    template <typename T>
    [[nodiscard]] constexpr const T* get_if() const noexcept {
        if constexpr (std::is_same_v<T, Deck>) {
            return kind_ == Kind::Deck ? &storage_.deck : nullptr;
        }
        else if constexpr (std::is_same_v<T, Discard>) {
            return kind_ == Kind::Discard ? &storage_.discard : nullptr;
        }
        else if constexpr (std::is_same_v<T, Field>) {
            return kind_ == Kind::Field ? &storage_.field : nullptr;
        }
        else if constexpr (std::is_same_v<T, Hand>) {
            return kind_ == Kind::Hand ? &storage_.hand : nullptr;
        }
        else if constexpr (std::is_same_v<T, FreeSpace>) {
            return kind_ == Kind::FreeSpace ? &storage_.free_space : nullptr;
        }
        else if constexpr (std::is_same_v<T, Outside>) {
            return kind_ == Kind::Outside ? &storage_.outside : nullptr;
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
        case Kind::Deck:
            return lhs.storage_.deck == rhs.storage_.deck;
        case Kind::Discard:
            return lhs.storage_.discard == rhs.storage_.discard;
        case Kind::Field:
            return lhs.storage_.field == rhs.storage_.field;
        case Kind::Hand:
            return lhs.storage_.hand == rhs.storage_.hand;
        case Kind::FreeSpace:
            return lhs.storage_.free_space == rhs.storage_.free_space;
        case Kind::Outside:
            return lhs.storage_.outside == rhs.storage_.outside;
        }

        return false;
    }

private:
    union Storage {
        Deck deck;
        Discard discard;
        Field field;
        Hand hand;
        FreeSpace free_space;
        Outside outside;

        constexpr Storage() noexcept
            : deck{} {}
    };

    Kind kind_{Kind::Deck};
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

}  // namespace shlab::gopc::games::pyramid
