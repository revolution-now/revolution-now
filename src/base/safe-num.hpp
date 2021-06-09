/****************************************************************
**safe-num.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-05-29.
*
* Description: Safe versions of primitives (required initializa-
*              tion and no implicit conversions.
*
*****************************************************************/
#pragma once

#include "config.hpp"

// base
#include "fmt.hpp"

// C++ standard library
#include <compare>

namespace base::safe {

// We don't really need many operations on these because they are
// mainly used as function parameter types to prevent implicit
// conversions from other types for the purpose of overload se-
// lection.

struct boolean {
  boolean() = delete;
  // clang-format off
  template<typename U>
  requires( std::is_same_v<U, bool> )
  boolean( U b ) noexcept : value_( b ) {}
  // clang-format on

  auto operator<=>( boolean const& ) const = default;

  operator bool() const noexcept { return value_; }

  bool operator!() const noexcept { return !value_; }

  bool get() const noexcept { return value_; }

private:
  bool value_;
};

template<typename T>
struct integral {
  integral() = delete;

  // clang-format off
  template<typename U>
  requires( std::is_integral_v<U> &&
            std::is_constructible_v<T, U> &&
           !std::is_same_v<U, bool> &&
            sizeof( U ) <= sizeof( T ) &&
            std::is_signed_v<T> == std::is_signed_v<U> )
  integral( U n ) noexcept : value_( n ) {}
  // clang-format on

  auto operator<=>( integral const& ) const = default;

  operator T() const noexcept { return value_; }

  bool get() const noexcept { return value_; }

private:
  T value_;
};

template<typename T>
struct floating {
  floating() = delete;

  // clang-format off
  template<typename U>
  requires( std::is_floating_point_v<U> &&
            std::is_constructible_v<T, U> &&
            sizeof( U ) <= sizeof( T ) )
  floating( U n ) noexcept : value_( n ) {}
  // clang-format on

  auto operator<=>( floating const& ) const = default;

  operator T() const noexcept { return value_; }

  bool get() const noexcept { return value_; }

private:
  T value_;
};

} // namespace base::safe

DEFINE_FORMAT( base::safe::boolean, "{}", o.get() );
DEFINE_FORMAT_T( ( T ), (base::safe::integral<T>), "{}",
                 o.get() );
DEFINE_FORMAT_T( ( T ), (base::safe::floating<T>), "{}",
                 o.get() );
