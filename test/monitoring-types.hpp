/****************************************************************
**monitoring-types.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-08.
*
* Description: Some types used for tracking during unit tests.
*
*****************************************************************/
#pragma once

// Must be last.
#include "test/catch-common.hpp"

// base
#include "base/fmt.hpp"

// C++ standard library
#include <type_traits>

namespace testing::monitoring_types {

/****************************************************************
** Tracker
*****************************************************************/
// Tracks number of constructions and destructions.
struct Tracker {
  static inline int constructed      = 0;
  static inline int destructed       = 0;
  static inline int copied           = 0;
  static inline int move_constructed = 0;
  static inline int move_assigned    = 0;
  static void       reset() {
          constructed = destructed = copied = move_constructed =
              move_assigned                 = 0;
  }

  Tracker() noexcept { ++constructed; }
  Tracker( Tracker const& ) noexcept { ++copied; }
  Tracker( Tracker&& ) noexcept { ++move_constructed; }
  ~Tracker() noexcept { ++destructed; }

  Tracker& operator=( Tracker const& ) = delete;
  Tracker& operator                    =( Tracker&& ) noexcept {
                        ++move_assigned;
                        return *this;
  }

  friend void to_str( Tracker const&, std::string& out,
                      base::ADL_t ) {
    out += "Tracker";
  }
};

/****************************************************************
** Formattable
*****************************************************************/
struct Formattable {
  int         n = 5;
  double      d = 7.7;
  std::string s = "hello";

  friend void to_str( Formattable const& o, std::string& out,
                      base::ADL_t ) {
    out += fmt::format( "Formattable{{n={},d={},s={}}}", o.n,
                        o.d, o.s );
  }
};

/****************************************************************
** Constexpr type
*****************************************************************/
struct Constexpr {
  constexpr Constexpr() = default;
  constexpr Constexpr( int n_ ) noexcept : n( n_ ) {}
  constexpr Constexpr( Constexpr const& ) = default;
  constexpr Constexpr( Constexpr&& )      = default;
  constexpr Constexpr& operator=( Constexpr const& ) = default;
  constexpr Constexpr& operator=( Constexpr&& )       = default;
  constexpr bool operator==( Constexpr const& ) const = default;

  int n;
};

/****************************************************************
** Empty
*****************************************************************/
struct Empty {};

inline void to_str( Empty const&, std::string& out,
                    base::ADL_t ) {
  out += "Empty{}";
}

/****************************************************************
** Non-Copyable
*****************************************************************/
struct NoCopy {
  explicit NoCopy( char c_ ) : c( c_ ) {}
  NoCopy( NoCopy const& ) = delete;
  NoCopy( NoCopy&& )      = default;
  NoCopy&     operator=( NoCopy const& ) = delete;
  NoCopy&     operator=( NoCopy&& )              = default;
  bool        operator==( NoCopy const& ) const& = default;
  char        c;
  friend void to_str( NoCopy const& o, std::string& out,
                      base::ADL_t ) {
    out += fmt::format( "NoCopy{{c={}}}", o.c );
  }
};
static_assert( !std::is_copy_constructible_v<NoCopy> );
static_assert( std::is_move_constructible_v<NoCopy> );
static_assert( !std::is_copy_assignable_v<NoCopy> );
static_assert( std::is_move_assignable_v<NoCopy> );

/****************************************************************
** Non-Copyable, Non-Movable
*****************************************************************/
struct NoCopyNoMove {
  NoCopyNoMove( char c_ ) : c( c_ ) {}
  NoCopyNoMove( NoCopyNoMove const& ) = delete;
  NoCopyNoMove( NoCopyNoMove&& )      = delete;
  NoCopyNoMove& operator=( NoCopyNoMove const& ) = delete;
  NoCopyNoMove& operator=( NoCopyNoMove&& ) = delete;
  NoCopyNoMove& operator=( char c_ ) noexcept {
    c = c_;
    return *this;
  }
  bool operator==( NoCopyNoMove const& ) const& = default;
  char c;
};
static_assert( !std::is_copy_constructible_v<NoCopyNoMove> );
static_assert( !std::is_move_constructible_v<NoCopyNoMove> );
static_assert( !std::is_copy_assignable_v<NoCopyNoMove> );
static_assert( !std::is_move_assignable_v<NoCopyNoMove> );

/****************************************************************
** Thrower
*****************************************************************/
struct Throws {
  Throws( bool should_throw = true ) {
    if( should_throw )
      throw std::runtime_error( "default construction" );
  }
  [[noreturn]] Throws( Throws const& ) {
    throw std::runtime_error( "copy construction" );
  }
  [[noreturn]] Throws( Throws&& ) {
    throw std::runtime_error( "move construction" );
  }
  Throws& operator=( Throws const& ) {
    throw std::runtime_error( "copy assignment" );
  }
  Throws& operator=( Throws&& ) {
    throw std::runtime_error( "move assignment" );
  }
};

/****************************************************************
** Trivial Everything
*****************************************************************/
struct Trivial {
  Trivial()                 = default;
  ~Trivial()                = default;
  Trivial( Trivial const& ) = default;
  Trivial( Trivial&& )      = default;
  Trivial& operator=( Trivial const& ) = default;
  Trivial& operator=( Trivial&& ) = default;

  double d;
  int    n;
};

/****************************************************************
** Convertibles
*****************************************************************/
struct Boolable {
  Boolable() = default;
  Boolable( bool m ) : n( m ) {}
  // clang-format off
  operator bool() const { return n; }
  // clang-format on
  bool n = {};
};

struct Intable {
  Intable() = default;
  Intable( int m ) : n( m ) {}
  // clang-format off
  operator int() const { return n; }
  // clang-format on
  int n = {};
};

struct Stringable {
  Stringable() = default;
  Stringable( std::string s_ ) : s( s_ ) {}
  // clang-format off
  operator std::string() const { return s; }
  // clang-format on
  std::string s = {};
};

/****************************************************************
** Base/Derived
*****************************************************************/
struct BaseClass {};
struct DerivedClass : BaseClass {};

} // namespace testing::monitoring_types
