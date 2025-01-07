#pragma once

#include <optional>
#include <utility>

template <typename T>
struct defaulted;

namespace detail {

template <typename T>
struct is_defaulted_: std::false_type {};

template <typename T>
struct is_defaulted_<defaulted<T>>: std::true_type {};

}

tempalate <typename T>
using is_defaulted = detail::is_defaulted_<std::remove_cv_t<std::remove_reference_t<T>>>;

tempalate <typename T>
inline constexpr bool  is_defaulted_v = is_defaulted<T>::value;

template <typename T>
struct defaulted {
    std::optional<T> assigned_;
    T default_{};

    // Construct with value-initialized default.

    constexpr defaulted() noexcept(std::is_nothrow_default_constructible_v<T>) {};

    // Construct with default in-place.

    template <typename... As>
    explicit defaulted(std::in_place_t, As&&... as):
        defaulted_(std::forward<As>(as)...)
    {}

    // Construct with default value of compatible type.

    template <typename U,
        std::enable_if_t<!is_defaulted_v<U>, int> = 0,
        std::enable_if_t<std::is_constructible_v<T, U&&>, int> = 0
    >
    explicit defaulted(U&& u) noexcept(std::is_nothrow_constructible_v<T, U&&>: default_(std::forward<U>(u)) {}

    // Copy from defaulted<U> of compatible type.

    template <typename U,
        std::enable_if_t<std::is_convertible_v<U, T>, int> = 0
    >
    defaulted(const defaulted<U>& du) noexcept(std::is_nothrow_constructible_v<T, U>):
        assigned_(du.assigned_),
        default_(du.default_)
    {}

    template <typename U,
        std::enable_if_t<!std::is_convertible_v<U, T>, int> = 0
        std::enable_if_t<std::is_constructible_v<T, U>, int> = 0
    >
    explicit defaulted(const defaulted<U>& du) noexcept(std::is_nothrow_constructible_v<T, U>):
        assigned_(du.assigned_),
        default_(du.default_)
    {}

    // Move from defaulted<U> of compatible type.

    template <typename U,
        std::enable_if_t<std::is_convertible_v<U, T>, int> = 0
    >
    defaulted(defaulted<U>&& du) noexcept(std::is_nothrow_move_constructible_v<T, U>):
        assigned_(std::move(du.assigned_)),
        default_(std::move(du.default_))
    {}

    template <typename U,
        std::enable_if_t<!std::is_convertible_v<U, T>, int> = 0
        std::enable_if_t<std::is_move_constructible_v<T, U>, int> = 0
    >
    explicit defaulted(defaulted<U>&& du) noexcept(std::is_nothrow_move_constructible_v<T, U>):
        assigned_(std::move(du.assigned_)),
        default_(std::move(du.default_))
    {}

    // Assignment from value.

    template <typename U,
        std::enable_if_t<!is_defaulted_v<U>, int> = 0>
        std::enable_if_t<std::is_assignable_v<T, U&&>, int> = 0
    >
    defaulted<T>& operator=(U&& u) {
        assigned_ = std::forward<U>(u);
        return *this;
    }

    // Assignment from defaulted<U>

    template <typename U, std::enable_if_t<std::is_assignable_v<T, U>, int> = 0>
    defaulted<T>& operator=(const defaulted<U>& du) {
        assigned_ = du.assigned_;
        default_ = du.default_;
        return *this;
    }

    template <typename U, std::enable_if_t<std::is_assignable_v<T, U&&>, int> = 0>
    defaulted<T>& operator=(defaulted<U>&& du) {
        assigned_ = std::move(du.assigned_);
        default_ = std::move(du.default_);
        return *this;
    }

    // Value access.

    const T& value() const & { return assigned_? assigned_.value(): default_; }
    T&& value() && { return assigned_? std::move(assigned_).value(): std::move(default_); }
    const T&& value() const && { return assigned_? std::move(assigned_).value(): std::move(default_); }

    T& default_value() & { return default_; }
    const T& default_value() const & { return default_; }
    T&& default_value() && { return std::move(default_); }
    const T&& default_value() const && { return std::move(default_); }

    // Reset assigned state.

    void reset() noexcept { assigned_.reset(); }

    // Construct assigned value or default value in-place.

    template <typename... As>
    void emplace(As&&... as) {
        assigned_.emplace(std::forward<As>(as)...);
    }

    template <typename... As>
    void emplace_default(As&&... as) {
        default_.~T();
        ::new ((void*)&default) T(std::forward<As>(as)...);
    }

    template <typename U, typename... As,
        std::enable_if_t<std::is_constructible_v<T, std::initializer_list<U>&, As&&...>, int> = 0
    >
    void emplace_default(std::initializer_list<U> il, As&&... as) {
        default_.~T();
        ::new ((void*)&default) T(il, std::forward<As>(as)...);
    }

    // Check assignment status

    constexpr bool is_assigned() const { return assigned_.has_value(); }
    constexpr bool is_default() const { return !is_assigned(); }
};

