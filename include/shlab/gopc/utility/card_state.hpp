#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "shlab/gopc/playing_cards/card.hpp"

namespace shlab::gopc::utility {

namespace detail {

inline constexpr std::size_t kStandardCardSlotCount = 52;
inline constexpr std::size_t kCardStateSlotCount = 54;

[[nodiscard]] constexpr std::size_t CardStateSlotIndex(
    const shlab::gopc::playing_cards::CardId id) noexcept {
    using shlab::gopc::playing_cards::CardId;

    if (id == CardId::Joker) {
        return 52;
    }

    if (id == CardId::ExtraJoker) {
        return 53;
    }

    const auto raw = static_cast<std::uint8_t>(id);
    const auto suit = static_cast<std::uint8_t>(raw >> 4U);
    const auto rank = static_cast<std::uint8_t>(raw & 0x0FU);

    if (suit < 1 || suit > 4 || rank < 1 || rank > 13) {
        return kCardStateSlotCount;
    }

    return static_cast<std::size_t>((suit - 1U) * 13U + (rank - 1U));
}

[[nodiscard]] constexpr std::size_t CardStateSlotIndex(
    const shlab::gopc::playing_cards::Card& card) noexcept {
    return CardStateSlotIndex(card.id());
}

[[nodiscard]] inline shlab::gopc::playing_cards::Card CardStateCardAt(
    const std::size_t index) noexcept {
    using shlab::gopc::playing_cards::Card;
    using shlab::gopc::playing_cards::CardId;

    if (index == 52) {
        return Card::from_id(CardId::Joker);
    }

    if (index == 53) {
        return Card::from_id(CardId::ExtraJoker);
    }

    const auto suit = static_cast<std::uint8_t>(index / 13U + 1U);
    const auto rank = static_cast<std::uint8_t>(index % 13U + 1U);
    const auto raw = static_cast<std::uint8_t>((suit << 4U) | rank);
    return Card::from_id(static_cast<CardId>(raw));
}

}  // namespace detail

template <typename Position, std::size_t Capacity>
class CardState {
public:
    using key_type = shlab::gopc::playing_cards::Card;
    using mapped_type = Position;
    using value_type = std::pair<key_type, mapped_type>;
    using size_type = std::size_t;

private:
    static_assert(Capacity <= detail::kCardStateSlotCount, "CardState capacity cannot exceed 54.");

    using storage_type = std::array<std::optional<mapped_type>, Capacity>;
    using occupancy_type = std::array<std::uint64_t, (Capacity + 63U) / 64U>;

    template <bool IsConst>
    class BasicIterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = CardState::value_type;
        using difference_type = std::ptrdiff_t;

        struct ReferenceProxy {
            key_type first;
            std::conditional_t<IsConst, const mapped_type&, mapped_type&> second;

            [[nodiscard]] operator value_type() const {
                return {first, second};
            }
        };

        using reference = ReferenceProxy;
        using pointer = std::conditional_t<IsConst, const ReferenceProxy*, ReferenceProxy*>;

        BasicIterator() = default;

        [[nodiscard]] reference operator*() const noexcept {
            return {detail::CardStateCardAt(index_), state_->positions_.at(index_).value()};
        }

        [[nodiscard]] pointer operator->() const noexcept {
            cache_.emplace(detail::CardStateCardAt(index_), state_->positions_.at(index_).value());
            return &cache_.value();
        }

        BasicIterator& operator++() noexcept {
            ++index_;
            AdvanceToOccupied();
            return *this;
        }

        BasicIterator operator++(int) noexcept {
            auto copy = *this;
            ++(*this);
            return copy;
        }

        [[nodiscard]] bool operator==(const BasicIterator& other) const noexcept {
            return state_ == other.state_ && index_ == other.index_;
        }

    private:
        friend class CardState;
        template <bool>
        friend class BasicIterator;

        using state_type = std::conditional_t<IsConst, const CardState, CardState>;

        template <bool WasConst = IsConst, typename = std::enable_if_t<WasConst>>
        BasicIterator(const BasicIterator<false>& other) noexcept
            : state_(other.state_)
            , index_(other.index_) {}

        BasicIterator(state_type* state, const size_type index, const bool skip_to_occupied = true) noexcept
            : state_(state)
            , index_(index) {
            if (skip_to_occupied) {
                AdvanceToOccupied();
            }
        }

        void AdvanceToOccupied() noexcept {
            while (state_ != nullptr && index_ < Capacity && !state_->IsOccupied(index_)) {
                ++index_;
            }
        }

        state_type* state_{};
        size_type index_{Capacity};
        mutable std::optional<ReferenceProxy> cache_{};
    };

public:
    using iterator = BasicIterator<false>;
    using const_iterator = BasicIterator<true>;

    CardState() = default;

    CardState(std::initializer_list<value_type> initial_values) {
        for (const auto& [card, position] : initial_values) {
            emplace(card, position);
        }
    }

    void reserve(std::size_t) noexcept {}

    [[nodiscard]] std::size_t size() const noexcept {
        return size_;
    }

    [[nodiscard]] bool empty() const noexcept {
        return size_ == 0;
    }

    [[nodiscard]] iterator begin() noexcept {
        return iterator{this, 0};
    }

    [[nodiscard]] const_iterator begin() const noexcept {
        return const_iterator{this, 0};
    }

    [[nodiscard]] const_iterator cbegin() const noexcept {
        return begin();
    }

    [[nodiscard]] iterator end() noexcept {
        return iterator{this, Capacity, false};
    }

    [[nodiscard]] const_iterator end() const noexcept {
        return const_iterator{this, Capacity, false};
    }

    [[nodiscard]] const_iterator cend() const noexcept {
        return end();
    }

    [[nodiscard]] iterator find(const key_type& card) noexcept {
        if (const auto index = FindSlot(card); index.has_value()) {
            return iterator{this, *index, false};
        }

        return end();
    }

    [[nodiscard]] const_iterator find(const key_type& card) const noexcept {
        if (const auto index = FindSlot(card); index.has_value()) {
            return const_iterator{this, *index, false};
        }

        return end();
    }

    [[nodiscard]] bool contains(const key_type& card) const noexcept {
        return FindSlot(card).has_value();
    }

    std::pair<iterator, bool> emplace(const key_type& card, const mapped_type& position) {
        const auto index = RequireSupportedSlot(card);
        if (const auto it = find(card); it != end()) {
            return {it, false};
        }

        EnsureCapacity();
        positions_[index].emplace(position);
        SetOccupied(index);
        ++size_;
        return {iterator{this, index, false}, true};
    }

    mapped_type& operator[](const key_type& card) {
        const auto index = RequireSupportedSlot(card);
        if (IsOccupied(index)) {
            return positions_[index].value();
        }

        EnsureCapacity();
        positions_[index].emplace(mapped_type{});
        SetOccupied(index);
        ++size_;
        return positions_[index].value();
    }

    mapped_type& at(const key_type& card) {
        return positions_[RequirePresentSlot(card)].value();
    }

    [[nodiscard]] const mapped_type& at(const key_type& card) const {
        return positions_[RequirePresentSlot(card)].value();
    }

    std::size_t erase(const key_type& card) noexcept {
        const auto index = detail::CardStateSlotIndex(card);
        if (index >= Capacity || !IsOccupied(index)) {
            return 0;
        }

        positions_[index].reset();
        ClearOccupied(index);
        --size_;
        return 1;
    }

    [[nodiscard]] bool operator==(const CardState& other) const noexcept {
        if (size_ != other.size_) {
            return false;
        }

        for (size_type index = 0; index < Capacity; ++index) {
            if (IsOccupied(index) != other.IsOccupied(index)) {
                return false;
            }

            if (IsOccupied(index) && positions_[index].value() != other.positions_[index].value()) {
                return false;
            }
        }

        return true;
    }

private:
    [[nodiscard]] std::optional<size_type> FindSlot(const key_type& card) const noexcept {
        const auto index = detail::CardStateSlotIndex(card);
        if (index >= Capacity || !IsOccupied(index)) {
            return std::nullopt;
        }

        return index;
    }

    [[nodiscard]] size_type RequireSupportedSlot(const key_type& card) const {
        const auto index = detail::CardStateSlotIndex(card);
        if (index >= Capacity) {
            throw std::out_of_range("Card is not supported by this state.");
        }

        return index;
    }

    [[nodiscard]] size_type RequirePresentSlot(const key_type& card) const {
        const auto index = RequireSupportedSlot(card);
        if (!IsOccupied(index)) {
            throw std::out_of_range("Card is not part of this state.");
        }

        return index;
    }

    void EnsureCapacity() const {
        if (size_ >= Capacity) {
            throw std::length_error("CardState capacity exceeded.");
        }
    }

    [[nodiscard]] bool IsOccupied(const size_type index) const noexcept {
        const auto word = index / 64U;
        const auto bit = index % 64U;
        return (occupied_[word] & (std::uint64_t{1} << bit)) != 0;
    }

    void SetOccupied(const size_type index) noexcept {
        const auto word = index / 64U;
        const auto bit = index % 64U;
        occupied_[word] |= (std::uint64_t{1} << bit);
    }

    void ClearOccupied(const size_type index) noexcept {
        const auto word = index / 64U;
        const auto bit = index % 64U;
        occupied_[word] &= ~(std::uint64_t{1} << bit);
    }

    storage_type positions_{};
    occupancy_type occupied_{};
    size_type size_{};
};

}  // namespace shlab::gopc::utility
