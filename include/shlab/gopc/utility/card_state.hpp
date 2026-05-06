#pragma once

#include <array>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <utility>

#include "shlab/gopc/playing_cards/card.hpp"

namespace shlab::gopc::utility {

template <typename Position, std::size_t Capacity>
class CardState {
public:
    using key_type = shlab::gopc::playing_cards::Card;
    using mapped_type = Position;
    using value_type = std::pair<key_type, mapped_type>;
    using size_type = std::size_t;

private:
    using storage_type = std::array<std::optional<value_type>, Capacity>;

    template <bool IsConst>
    class BasicIterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = CardState::value_type;
        using difference_type = std::ptrdiff_t;
        using reference = std::conditional_t<IsConst, const value_type&, value_type&>;
        using pointer = std::conditional_t<IsConst, const value_type*, value_type*>;

        constexpr BasicIterator() = default;

        constexpr reference operator*() const noexcept {
            return storage_->at(index_).value();
        }

        constexpr pointer operator->() const noexcept {
            return &storage_->at(index_).value();
        }

        constexpr BasicIterator& operator++() noexcept {
            ++index_;
            return *this;
        }

        constexpr BasicIterator operator++(int) noexcept {
            auto copy = *this;
            ++(*this);
            return copy;
        }

        [[nodiscard]] constexpr bool operator==(const BasicIterator& other) const noexcept = default;

    private:
        friend class CardState;

        constexpr BasicIterator(
            std::conditional_t<IsConst, const storage_type*, storage_type*> storage,
            const size_type index) noexcept
            : storage_(storage)
            , index_(index) {}

        std::conditional_t<IsConst, const storage_type*, storage_type*> storage_{};
        size_type index_{};
    };

public:
    using iterator = BasicIterator<false>;
    using const_iterator = BasicIterator<true>;

    constexpr CardState() = default;

    constexpr CardState(std::initializer_list<value_type> initial_values) {
        for (const auto& [card, position] : initial_values) {
            emplace(card, position);
        }
    }

    constexpr void reserve(std::size_t) noexcept {}

    [[nodiscard]] constexpr std::size_t size() const noexcept {
        return size_;
    }

    [[nodiscard]] constexpr bool empty() const noexcept {
        return size_ == 0;
    }

    [[nodiscard]] constexpr iterator begin() noexcept {
        return iterator{&entries_, 0};
    }

    [[nodiscard]] constexpr const_iterator begin() const noexcept {
        return const_iterator{&entries_, 0};
    }

    [[nodiscard]] constexpr const_iterator cbegin() const noexcept {
        return begin();
    }

    [[nodiscard]] constexpr iterator end() noexcept {
        return iterator{&entries_, size_};
    }

    [[nodiscard]] constexpr const_iterator end() const noexcept {
        return const_iterator{&entries_, size_};
    }

    [[nodiscard]] constexpr const_iterator cend() const noexcept {
        return end();
    }

    [[nodiscard]] constexpr iterator find(const key_type& card) noexcept {
        for (size_type index = 0; index < size_; ++index) {
            if (entries_[index]->first == card) {
                return iterator{&entries_, index};
            }
        }

        return end();
    }

    [[nodiscard]] constexpr const_iterator find(const key_type& card) const noexcept {
        for (size_type index = 0; index < size_; ++index) {
            if (entries_[index]->first == card) {
                return const_iterator{&entries_, index};
            }
        }

        return end();
    }

    [[nodiscard]] constexpr bool contains(const key_type& card) const noexcept {
        return find(card) != end();
    }

    constexpr std::pair<iterator, bool> emplace(const key_type& card, const mapped_type& position) {
        if (const auto it = find(card); it != end()) {
            return {it, false};
        }

        EnsureCapacity();
        entries_[size_].emplace(card, position);
        ++size_;
        return {iterator{&entries_, size_ - 1}, true};
    }

    constexpr mapped_type& operator[](const key_type& card) {
        if (const auto it = find(card); it != end()) {
            return it->second;
        }

        EnsureCapacity();
        entries_[size_].emplace(card, mapped_type{});
        ++size_;
        return entries_[size_ - 1]->second;
    }

    constexpr mapped_type& at(const key_type& card) {
        if (const auto it = find(card); it != end()) {
            return it->second;
        }

        throw std::out_of_range("Card is not part of this state.");
    }

    [[nodiscard]] constexpr const mapped_type& at(const key_type& card) const {
        if (const auto it = find(card); it != end()) {
            return it->second;
        }

        throw std::out_of_range("Card is not part of this state.");
    }

    constexpr std::size_t erase(const key_type& card) noexcept {
        size_type index = 0;
        while (index < size_ && entries_[index]->first != card) {
            ++index;
        }

        if (index == size_) {
            return 0;
        }

        for (size_type next = index + 1; next < size_; ++next) {
            entries_[next - 1] = std::move(entries_[next]);
        }

        entries_[size_ - 1].reset();
        --size_;
        return 1;
    }

    [[nodiscard]] constexpr bool operator==(const CardState& other) const noexcept {
        if (size_ != other.size_) {
            return false;
        }

        for (const auto& [card, position] : *this) {
            const auto it = other.find(card);
            if (it == other.end() || it->second != position) {
                return false;
            }
        }

        return true;
    }

private:
    constexpr void EnsureCapacity() const {
        if (size_ >= Capacity) {
            throw std::length_error("CardState capacity exceeded.");
        }
    }

    storage_type entries_{};
    size_type size_{};
};

}  // namespace shlab::gopc::utility
