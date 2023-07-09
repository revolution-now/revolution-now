/****************************************************************
**scope-exit.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-08-09.
*
* Description: Scope Guard.
*
*****************************************************************/
#pragma once

#include "config.hpp"

// base
#include "function-ref.hpp"
#include "macros.hpp"

// C++ standard library
#include <exception>

namespace base {

// TODO: when C++23 is fully implemented there will be a
// std::scope_exit (std::experimental::scope_exit at the time of
// writing) that we might want to use to replace some of the
// things in this header.

/****************************************************************
** ScopedSet
*****************************************************************/
// Temporarily set a variable to a new value, then on scope exit
// either restore it to the old value (SCOPED_SET_AND_RESTORE) or
// restore it to the specified value (SCOPED_SET_AND_CHANGE).
//
// Example:
//
//   int n = 5;
//   {
//     SCOPED_SET_AND_RESTORE( n, 4 );
//     ...
//     REQUIRE( n == 4 );
//   }
//   REQUIRE( n == 5 );
//
// Example:
//
//   int n = 5;
//   {
//     SCOPED_SET_AND_CHANGE( n, 4, 6 );
//     ...
//     REQUIRE( n == 4 );
//   }
//   REQUIRE( n == 6 );
//
#define SCOPED_SET_AND_RESTORE( var, new_val )               \
  base::ScopedSet STRING_JOIN( __scoped_setter_, __LINE__ )( \
      var, new_val );

#define SCOPED_SET_AND_CHANGE( var, new_val, restore_val )   \
  base::ScopedSet STRING_JOIN( __scoped_setter_, __LINE__ )( \
      var, new_val, restore_val );

// Temporarily set a variable to a new value and then restore the
// old value on scope exit. May be easier to just use the macro
// version (above).
//
// Example:
//
//   int n = 5;
//   {
//     ScopedSet setter( n, 4 );
//     ...
//   }
//
template<typename T>
struct ScopedSet {
  ScopedSet( ScopedSet const& )            = delete;
  ScopedSet& operator=( ScopedSet const& ) = delete;
  ScopedSet( ScopedSet&& )                 = delete;
  ScopedSet& operator=( ScopedSet&& )      = delete;

  template<typename U>
  explicit ScopedSet( T& var, U&& new_val )
    : var_( var ), restore_val_( var ) {
    var = std::forward<U>( new_val );
  }

  template<typename U>
  explicit ScopedSet( T& var, U&& new_val, U&& restore_val )
    : var_( var ), restore_val_( restore_val ) {
    var = std::forward<U>( new_val );
  }

  ~ScopedSet() noexcept { var_ = restore_val_; }

  T& var_;
  T  restore_val_;
};

/****************************************************************
** SCOPE_EXIT
*****************************************************************/
// Run some code at scope exit.  Example usage:
//
// Small expression:
//
//   SCOPE_EXIT( x = 5 );
//
// Multi-line:
//
//   void foo() {
//     ...
//     int x = 3;
//     ...
//     SCOPE_EXIT( {
//       x = 4;           // Captures outer context by ref.
//       CHECK( x == 4 ); // Ok to throw exceptions.
//       fmt::print( "hello" );
//     } );
//     ...
//   }
//
// The code snippet inside SCOPE_EXIT will be run on any scope
// exit, meaning 1) control runs passed the end of a closing
// brace, a return from a function, or an exception thrown.
//
// Exceptions: it is generally OK to throw exceptions from inside
// the code snippet; see below for details.
#define SCOPE_EXIT( code )                        \
  base::ScopeExit STRING_JOIN( exit_, __LINE__ )( \
      __FILE__, __LINE__, [&] { code; } )

namespace detail {

void run_func_noexcept( char const* file, int line,
                        function_ref<void()> func ) noexcept;

}

// Defines a utility that can be used to run arbitrary code at
// scope exit.
template<typename T>
struct ScopeExit {
  ScopeExit( char const* file, int line, T&& func )
    : file_( file ),
      line_( line ),
      func_( std::forward<T>( func ) ),
      exceptions_in_flight_( std::uncaught_exceptions() ) {
    static_assert( std::is_rvalue_reference_v<
                   decltype( std::forward<T>( func_ ) )> );
  }

  ~ScopeExit() noexcept( false ) {
    // Exceptions: The func_ code snippet is being run inside the
    // destructor of this class. Therefore it is sensitive to ex-
    // ceptions thrown from within it. Before running the snip-
    // pet, we will will determine if we are being run as part of
    // stack unwinding due to an uncaught exception in flight
    // (i.e., not just stack unwinding due to a scope exit). If
    // so, then any exception thrown (and that escapes) from the
    // snippet (and this destructor) would immediately cause the
    // runtime to immediately terminate the process, so therefore
    // in this situation we will catch all exceptions and log
    // them as critical errors and return gracefully. If, on the
    // other hand, we are not being called as part of a stack un-
    // winding due to an in-flight exception then any exceptions
    // thrown by the snippet will be allowed to escape.
    //
    // The mechanism used here to detect a stack unwinding due to
    // an in-flight exception is based on the
    // std::uncaught_exceptions() mechanism described here:
    //
    //   https://isocpp.org/files/papers/N4152.pdf
    if( std::uncaught_exceptions() > exceptions_in_flight_ ) {
      // This destructor is being run as part of a stack
      // unwinding due to a new exception that has been thrown
      // since our constructor ran.  Therefore it is not safe to
      // throw exceptions.
      detail::run_func_noexcept( file_, line_, func_ );
    } else {
      func_();
    }
  }

  char const* file_{};
  int         line_{};
  T           func_;
  int         exceptions_in_flight_{};
};

} // namespace base
