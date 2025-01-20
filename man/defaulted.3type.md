% PARAPARA::DEFAULTED(3) Parapara Manual

# NAME

parapara::defaulted<T> - wrapper for std::optional with default value

# SYNOPSIS

```
#include <parapara/parapara.h>
```

## Public Types

typedef T **value_type**

## Public Member Functions

constexpr **defaulted** () noexcept(...)

template <typename... As> constexpr explicit **defaulted** (std::in_place_t, As&&...)

template <typename U, typename... As> constexpr explicit **defaulted** (std::in_place_t, std::initializer_list<U>, As&&...)

constexpr **defaulted** (const defaulted&) noexcept(...)

constexpr **defaulted** (defaulted&&) noexcept(...)

template <typename U> constexpr explicit **defaulted** (U&& u) noexcept(...)

constexpr **defaulted** (const defaulted<U>&) noexcept(...)

constexpr **defaulted** (defaulted<U>&&) noexcept(...)

template <typename U> defaulted<T>& **operator=** (U&&)

template <typename U> defaulted<T>& **operator=** (const defaulted<U>&)

template <typename U> defaulted<T>& **operator=** (defaulted<U>&&)

const T& **value** () const &

T&& **value** () &&

const T&& **value** () const &&

T& **default_value** () &

const T& **default_value** () const &

T&& **default_value** () &&

const T&& **default_value** () const &&

void **reset** () noexcept

template <typename... As> void **emplace** (As&&...)

template <typename U, typename... As> void **emplace** (std::initializer_list<U>, As&&...)

template <typename... As> void **emplace_default** (As&&...)

template <typename U, typename... As> void **emplace_default** (std::initializer_list<U>, As&&...)

constexpr bool **is_assigned** () const

constexpr bool **is_default** () const

# DESCRIPTION

`parapara::defaulted<T>` represents an optional 'assigned' value and fallback
'default' value of type T. Unless constructed from another `defaulted` object,
a `defaulted<T>` value will be in an unassigned state, where the wrapped
optional assigned value is empty. The `value()` method returns the assigned
value if in the assigned state, or the default value if in the unassigned
state. The methods `defaulted<T>::is_assigned()` and `defaulted<T>::is_default()`
query the assigned state.

A `defaulted<T>` object can be given an assigned value in three ways:

1. With the `defaulted<T>::emplace(..)` method.

2. Through assignment from a value of a type convertible to T.

3. Through assignment from another `defaulted<U>` object which is in the assigned state, where U is convertible to T.

In the third case, it is important to note that assignment from another `defaulted` object _does not_ change the
set default value. The assigned state can be cleared (and the assigned value destroyed) in two ways:

1. With the `defaulted<T>::reset()` method.

2. Through assignment from another `defaulted<U>` object which is in the unassigned state, where U is convertible to T.

The default value associated with a `defaulted<T>` object can be retrieved and modified with the
`defaulted<T>::default_value()` method, or constructed in-place with `defaulted<T>::emplace_default()`.

## Constructors

**constexpr defaulted() noexcept(...)**
 ~  Construct with value-constructed default value, nothrow if T is nothrow default-constructible.

**template <typename... As> constexpr explicit defaulted(std::in_place_t, As&&... as)**
 ~  Construct with default value constructed in place from argument list.

**template <typename U, typename... As> constexpr explicit defaulted(std::in_place_t, std::initializer_list<U> il, As&&... as)**
 ~  Construct with default value constructed in place from initializer list and argument list.

**constexpr defaulted(const defaulted& d) noexcept(...)**
 ~  Copy constructor, nothrow if T is nothrow copy-constructible.

**constexpr defaulted(defaulted&& d) noexcept(...)**
 ~  Move constructor, nothrow if T is nothrow move-constructible.

## Assignment

**template <typename U> defaulted<T>& operator=(U&&)**
 ~ Assign a value (not of type defaulted<U> for some U), setting the assigned state.

**template <typename U> defaulted<T>& operator=(const defaulted<U>&)**
 ~ If the argument is in the assigned state, copy (if not already assigned) or copy-assign the assigned value of the argument and set the assigned state.
Otherwise clear the assigned state and destroy any assigned value.

**template <typename U> defaulted<T>& operator=(defaulted<U>&&)**
 ~ If the argument is in the assigned state, move (if not already assigned) or move-assign the assigned value of the argument and set the assigned state.
Otherwise clear the assigned state and destroy any assigned value.

## Value access

**const T& value() const &**

**T&& value() &&**

**const T&& value() const &&**
~ Return const or rvalue reference to assigned value if in assigned state, or else the default value.

**T& default_value() &**

**const T& default_value() const &**

**T&& default_value() &&**

**const T&& default_value() const &&**
~ Return lvalue or reference to default value.

**constexpr bool is_assigned() const**
~ Returns true if the object is in the assigned state.

**constexpr bool is_default() const**
~ Returns true if the object is not in the assigned state.

## Mutating operations

**void **reset**() noexcept**
~ Set the state to unassigned and destroy the assigned value if present.

**template <typename... As> void emplace(As&&...)**

**template <typename U, typename... As> void emplace(std::initializer_list<U>, As&&...)**
~ Construct the assigned value from the given arguments in place and place the object in the assigned state.

**template <typename... As> void emplace_default(As&&...)**

**template <typename U, typename... As> void emplace_default(std::initializer_list<U>, As&&...)**
~ Construct the default value from the given arguments in place.

