#pragma once

#include <compare>
#include <cstdint>
#include <stdexcept>
#include <type_traits>



namespace shlab::gopc::games::four_leaf_clover {

struct Field {
    int number{};

    auto operator<=>(const Field&) const noexcept = default;
};

struct Stock {
    int number{};

    auto operator<=>(const Stock&) const noexcept = default;
};

struct Removed {
    auto operator<=>(const Removed&) const noexcept = default;
};

class Position {
public:
    enum class Kind : std::uint8_t {
        Field,
        Stock,
        Removed,
    };

    constexpr Position() noexcept = default;

    constexpr Position(Field value) noexcept
        : kind_(Kind::Field) {
        storage_.field = value;
    }

    constexpr Position(Stock value) noexcept
        : kind_(Kind::Stock) {
        storage_.stock = value;
    }

    constexpr Position(Removed value) noexcept
        : kind_(Kind::Removed) {
        storage_.removed = value;
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
        if constexpr (std::is_same_v<T, Field>) {
            return kind_ == Kind::Field;
        }
        else if constexpr (std::is_same_v<T, Stock>) {
            return kind_ == Kind::Stock;
        }
        else if constexpr (std::is_same_v<T, Removed>) {
            return kind_ == Kind::Removed;
        }
        else {
            return false;
        }
    }

    template <typename T>
    [[nodiscard]] constexpr T* get_if() noexcept {
        if constexpr (std::is_same_v<T, Field>) {
            return kind_ == Kind::Field ? &storage_.field : nullptr;
        }
        else if constexpr (std::is_same_v<T, Stock>) {
            return kind_ == Kind::Stock ? &storage_.stock : nullptr;
        }
        else if constexpr (std::is_same_v<T, Removed>) {
            return kind_ == Kind::Removed ? &storage_.removed : nullptr;
        }
        else {
            return nullptr;
        }
    }

    template <typename T>
    [[nodiscard]] constexpr const T* get_if() const noexcept {
        if constexpr (std::is_same_v<T, Field>) {
            return kind_ == Kind::Field ? &storage_.field : nullptr;
        }
        else if constexpr (std::is_same_v<T, Stock>) {
            return kind_ == Kind::Stock ? &storage_.stock : nullptr;
        }
        else if constexpr (std::is_same_v<T, Removed>) {
            return kind_ == Kind::Removed ? &storage_.removed : nullptr;
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
        case Kind::Field:
            return lhs.storage_.field == rhs.storage_.field;
        case Kind::Stock:
            return lhs.storage_.stock == rhs.storage_.stock;
        case Kind::Removed:
            return lhs.storage_.removed == rhs.storage_.removed;
        }

        return false;
    }

private:
    union Storage {
        Field field;
        Stock stock;
        Removed removed;

        constexpr Storage() noexcept
            : field{} {}
    };

    Kind kind_{Kind::Field};
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
    case Position::Kind::Field:
        return vis(pos.get<Field>());
    case Position::Kind::Stock:
        return vis(pos.get<Stock>());
    case Position::Kind::Removed:
        return vis(pos.get<Removed>());
    }
    __builtin_unreachable();
}

template <typename Visitor>
constexpr decltype(auto) visit(Position& pos, Visitor&& vis) {
    switch (pos.kind()) {
    case Position::Kind::Field:
        return vis(pos.get<Field>());
    case Position::Kind::Stock:
        return vis(pos.get<Stock>());
    case Position::Kind::Removed:
        return vis(pos.get<Removed>());
    }
    __builtin_unreachable();
}

}  // namespace shlab::gopc::games::four_leaf_clover
