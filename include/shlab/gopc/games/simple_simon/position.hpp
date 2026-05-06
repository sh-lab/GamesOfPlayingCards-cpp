#pragma once

#include <compare>
#include <cstdint>
#include <stdexcept>
#include <type_traits>

#include "shlab/gopc/games/simple_simon/column.hpp"

namespace shlab::gopc::games::simple_simon {

struct Foundation {
    auto operator<=>(const Foundation&) const noexcept = default;
};

struct Tableau {
    Column column{};
    int number{};

    auto operator<=>(const Tableau&) const noexcept = default;
};

class Position {
public:
    enum class Kind : std::uint8_t {
        Foundation,
        Tableau,
    };

    constexpr Position() noexcept = default;

    constexpr Position(Foundation value) noexcept
        : kind_(Kind::Foundation) {
        storage_.foundation = value;
    }

    constexpr Position(Tableau value) noexcept
        : kind_(Kind::Tableau) {
        storage_.tableau = value;
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
        if constexpr (std::is_same_v<T, Foundation>) {
            return kind_ == Kind::Foundation;
        }
        else if constexpr (std::is_same_v<T, Tableau>) {
            return kind_ == Kind::Tableau;
        }
        else {
            return false;
        }
    }

    template <typename T>
    [[nodiscard]] constexpr T* get_if() noexcept {
        if constexpr (std::is_same_v<T, Foundation>) {
            return kind_ == Kind::Foundation ? &storage_.foundation : nullptr;
        }
        else if constexpr (std::is_same_v<T, Tableau>) {
            return kind_ == Kind::Tableau ? &storage_.tableau : nullptr;
        }
        else {
            return nullptr;
        }
    }

    template <typename T>
    [[nodiscard]] constexpr const T* get_if() const noexcept {
        if constexpr (std::is_same_v<T, Foundation>) {
            return kind_ == Kind::Foundation ? &storage_.foundation : nullptr;
        }
        else if constexpr (std::is_same_v<T, Tableau>) {
            return kind_ == Kind::Tableau ? &storage_.tableau : nullptr;
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
        case Kind::Foundation:
            return lhs.storage_.foundation == rhs.storage_.foundation;
        case Kind::Tableau:
            return lhs.storage_.tableau == rhs.storage_.tableau;
        }

        return false;
    }

private:
    union Storage {
        Foundation foundation;
        Tableau tableau;

        constexpr Storage() noexcept
            : foundation{} {}
    };

    Kind kind_{Kind::Foundation};
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

}  // namespace shlab::gopc::games::simple_simon
