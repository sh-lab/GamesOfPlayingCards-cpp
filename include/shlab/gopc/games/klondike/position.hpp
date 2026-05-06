#pragma once

#include <compare>
#include <cstdint>
#include <stdexcept>
#include <type_traits>

#include "shlab/gopc/games/klondike/column.hpp"

namespace shlab::gopc::games::klondike {

struct Foundation {
    auto operator<=>(const Foundation&) const noexcept = default;
};

struct Stock {
    int number{};

    auto operator<=>(const Stock&) const noexcept = default;
};

struct WastePile {
    int number{};

    auto operator<=>(const WastePile&) const noexcept = default;
};

struct Tableau {
    Column column{};
    int number{};
    bool open{};

    auto operator<=>(const Tableau&) const noexcept = default;
};

class Position {
public:
    enum class Kind : std::uint8_t {
        Foundation,
        Stock,
        WastePile,
        Tableau,
    };

    constexpr Position() noexcept = default;

    constexpr Position(Foundation value) noexcept
        : kind_(Kind::Foundation) {
        storage_.foundation = value;
    }

    constexpr Position(Stock value) noexcept
        : kind_(Kind::Stock) {
        storage_.stock = value;
    }

    constexpr Position(WastePile value) noexcept
        : kind_(Kind::WastePile) {
        storage_.waste_pile = value;
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
        else if constexpr (std::is_same_v<T, Stock>) {
            return kind_ == Kind::Stock;
        }
        else if constexpr (std::is_same_v<T, WastePile>) {
            return kind_ == Kind::WastePile;
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
        else if constexpr (std::is_same_v<T, Stock>) {
            return kind_ == Kind::Stock ? &storage_.stock : nullptr;
        }
        else if constexpr (std::is_same_v<T, WastePile>) {
            return kind_ == Kind::WastePile ? &storage_.waste_pile : nullptr;
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
        else if constexpr (std::is_same_v<T, Stock>) {
            return kind_ == Kind::Stock ? &storage_.stock : nullptr;
        }
        else if constexpr (std::is_same_v<T, WastePile>) {
            return kind_ == Kind::WastePile ? &storage_.waste_pile : nullptr;
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
        case Kind::Stock:
            return lhs.storage_.stock == rhs.storage_.stock;
        case Kind::WastePile:
            return lhs.storage_.waste_pile == rhs.storage_.waste_pile;
        case Kind::Tableau:
            return lhs.storage_.tableau == rhs.storage_.tableau;
        }

        return false;
    }

private:
    union Storage {
        Foundation foundation;
        Stock stock;
        WastePile waste_pile;
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

template <typename Visitor>
constexpr decltype(auto) visit(const Position& pos, Visitor&& vis) {
    switch (pos.kind()) {
    case Position::Kind::Foundation:
        return vis(pos.get<Foundation>());
    case Position::Kind::Stock:
        return vis(pos.get<Stock>());
    case Position::Kind::WastePile:
        return vis(pos.get<WastePile>());
    case Position::Kind::Tableau:
        return vis(pos.get<Tableau>());
    }
    __builtin_unreachable();
}

template <typename Visitor>
constexpr decltype(auto) visit(Position& pos, Visitor&& vis) {
    switch (pos.kind()) {
    case Position::Kind::Foundation:
        return vis(pos.get<Foundation>());
    case Position::Kind::Stock:
        return vis(pos.get<Stock>());
    case Position::Kind::WastePile:
        return vis(pos.get<WastePile>());
    case Position::Kind::Tableau:
        return vis(pos.get<Tableau>());
    }
    __builtin_unreachable();
}

}  // namespace shlab::gopc::games::klondike
