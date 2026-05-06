#pragma once

#include <compare>
#include <cstdint>
#include <stdexcept>
#include <type_traits>

#include "shlab/gopc/games/freecell/column.hpp"

namespace shlab::gopc::games::freecell {

struct Foundation {
    auto operator<=>(const Foundation&) const noexcept = default;
};

struct Cell {
    int number{};

    auto operator<=>(const Cell&) const noexcept = default;
};

struct Tableau {
    Column column{};
    int number{};

    auto operator<=>(const Tableau&) const noexcept = default;
};

class Position {
public:
    enum class Kind : std::uint8_t {
        Cell,
        Foundation,
        Tableau,
    };

    constexpr Position() noexcept = default;

    constexpr Position(Cell value) noexcept
        : kind_(Kind::Cell) {
        storage_.cell = value;
    }

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
        if constexpr (std::is_same_v<T, Cell>) {
            return kind_ == Kind::Cell;
        }
        else if constexpr (std::is_same_v<T, Foundation>) {
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
        if constexpr (std::is_same_v<T, Cell>) {
            return kind_ == Kind::Cell ? &storage_.cell : nullptr;
        }
        else if constexpr (std::is_same_v<T, Foundation>) {
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
        if constexpr (std::is_same_v<T, Cell>) {
            return kind_ == Kind::Cell ? &storage_.cell : nullptr;
        }
        else if constexpr (std::is_same_v<T, Foundation>) {
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
        case Kind::Cell:
            return lhs.storage_.cell == rhs.storage_.cell;
        case Kind::Foundation:
            return lhs.storage_.foundation == rhs.storage_.foundation;
        case Kind::Tableau:
            return lhs.storage_.tableau == rhs.storage_.tableau;
        }

        return false;
    }

private:
    union Storage {
        Cell cell;
        Foundation foundation;
        Tableau tableau;

        constexpr Storage() noexcept
            : cell{} {}
    };

    Kind kind_{Kind::Cell};
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
    case Position::Kind::Cell:
        return vis(pos.get<Cell>());
    case Position::Kind::Foundation:
        return vis(pos.get<Foundation>());
    case Position::Kind::Tableau:
        return vis(pos.get<Tableau>());
    }
    __builtin_unreachable();
}

template <typename Visitor>
constexpr decltype(auto) visit(Position& pos, Visitor&& vis) {
    switch (pos.kind()) {
    case Position::Kind::Cell:
        return vis(pos.get<Cell>());
    case Position::Kind::Foundation:
        return vis(pos.get<Foundation>());
    case Position::Kind::Tableau:
        return vis(pos.get<Tableau>());
    }
    __builtin_unreachable();
}

}  // namespace shlab::gopc::games::freecell
