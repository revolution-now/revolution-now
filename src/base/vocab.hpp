/****************************************************************
**vocab.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-12-03.
*
* Description: Some miscellaneous vocabulary types.
*
*****************************************************************/
#pragma once

#include "config.hpp"

// C++ standard library.
#include <type_traits>
#include <utility>

namespace base {

// This wrapper does everything possible to try to prevent its
// contained value from being copied.
template<typename T> /* clang-format off */
  requires( !std::is_const_v<T> && !std::is_reference_v<T> )
struct NoCopy {
  /* clang-format on */
  /**************************************************************
  ** Allowed
  ***************************************************************/
  NoCopy() requires( std::is_default_constructible_v<T> ) =
      default;

  NoCopy( T&& val_ ) : val( std::move( val_ ) ) {}

  NoCopy( NoCopy&& ) requires(
      std::is_trivially_move_constructible_v<T> ) = default;

  NoCopy( NoCopy&& o ) requires(
      std::is_move_constructible_v<T> &&
      !std::is_trivially_move_constructible_v<T> )
    : val( std::move( o.val ) ) {}

  NoCopy& operator=( T&& val_ ) { val = std::move( val_ ); }

  NoCopy& operator                             =( NoCopy&& ) &
      requires( std::is_move_assignable_v<T> ) = default;

  /**************************************************************
  ** Now Allowed
  ***************************************************************/
  NoCopy( T const& )      = delete;
  NoCopy( NoCopy const& ) = delete;
  NoCopy& operator=( T const& ) = delete;
  NoCopy& operator=( NoCopy const& ) = delete;

  /**************************************************************
  ** Implicit conversions
  ***************************************************************/
  operator T const &() const& noexcept { return val; }
  operator T&() & noexcept { return val; }
  operator T const &&() const&& noexcept {
    return std::move( val );
  }
  operator T&&() && noexcept { return std::move( val ); }

  T const* operator->() const noexcept { return &val; }
  T*       operator->() noexcept { return &val; }

  /**************************************************************
  ** Comparison, just for convenience.
  ***************************************************************/
  template<typename U>
  bool operator==( U&& u ) const {
    return ( val == std::forward<U>( u ) );
  }

  /**************************************************************
  ** The value.
  ***************************************************************/
private:
  T val;
};

} // namespace base