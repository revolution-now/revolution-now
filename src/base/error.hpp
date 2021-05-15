/****************************************************************
**error.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-12-08.
*
* Description: Utilities for handling errors.
*
*****************************************************************/
#pragma once

#include "config.hpp"

// base
#include "macros.hpp"
#include "source-loc.hpp"

// base-util
#include "base-util/pp.hpp"

// {fmt}
#include "fmt/format.h"

// C++ standard library
#include <memory>

// There are two types of error-checking macros:
//
//   1. Those that abort the program.
//   2. Those that return an error.
//
// For those that abort the program, they will dump a stack trace
// to the console before aborting.

/****************************************************************
** Compile-time format string checking.
*****************************************************************/
// This is used to wrap calls to fmt::format that want compile
// time format string checking. It assumes that the first argu-
// ment is some kind of constexpr expression (maybe has to
// specifically be a string literal, not sure) and will wrap that
// first argument in the FMT_STRING() macro, which will enable
// compile-time checking that e.g. the number of {} in the format
// string matches the number of arguments.
#define FMT_SAFE( fmt_str, ... ) \
  fmt::format( FMT_STRING( fmt_str ), ##__VA_ARGS__ )

/****************************************************************
** Self-documenting one-line aborters.
*****************************************************************/
#define SHOULD_NOT_BE_HERE \
  ::base::abort_with_msg(  \
      "programmer error: should not be here" )

#define NOT_IMPLEMENTED   \
  ::base::abort_with_msg( \
      "programmer error: need to implement this" )

#define MUST_IMPROVE_IMPLEMENTATION_BEFORE_USE                \
  ::base::abort_with_msg(                                     \
      "the implementation of this function must be improved " \
      "before use" )

#define WARNING_THIS_FUNCTION_HAS_NOT_BEEN_TESTED         \
  ::base::abort_with_msg(                                 \
      "the implementation of this function has not been " \
      "verified." )

#define TODO( msg ) ::base::abort_with_msg( "TODO: " msg )

/****************************************************************
** Abort with formatted error message.
*****************************************************************/
// This is used when you want to just fail but with formatting in
// the error message. If you don't need formatting then just use
// the `abort_with_msg` function.
#define FATAL( ... )                                 \
  ::base::abort_with_msg( ::base::detail::check_msg( \
      "fatal error", FMT_SAFE( "" __VA_ARGS__ ) ) );

/****************************************************************
** Main check-fail macros.
*****************************************************************/
// Non-testing code should use this. Code written in unit test
// cpp files should use BASE_CHECK to avoid a collision with a
// similar Catch2 symbol.
#define CHECK( ... ) BASE_CHECK( __VA_ARGS__ )
#define CHECK_EQ( ... ) BASE_CHECK_EQ( __VA_ARGS__ )

#define BASE_CHECK( a, ... )                             \
  {                                                      \
    if( !( a ) ) {                                       \
      ::base::abort_with_msg( ::base::detail::check_msg( \
          #a, FMT_SAFE( "" __VA_ARGS__ ) ) );            \
    }                                                    \
  }

#define BASE_CHECK_EQ( x, y )                                \
  {                                                          \
    if( ( x ) != ( y ) ) {                                   \
      ::base::abort_with_msg( ::base::detail::check_msg(     \
          #x " != " #y, fmt::format( "{} != {}", x, y ) ) ); \
    }                                                        \
  }

// DCHECK is CHECK in debug builds, but compiles to nothing in
// release builds.
#ifdef NDEBUG
#  define DCHECK( ... )
#else
#  define DCHECK( ... ) BASE_CHECK( __VA_ARGS__ )
#endif

/****************************************************************
** Check that a wrapped type has a value.
*****************************************************************/
#define CHECK_HAS_VALUE( e )                        \
  {                                                 \
    auto const& STRING_JOIN( __e, __LINE__ ) = e;   \
    if( !bool( STRING_JOIN( __e, __LINE__ ) ) ) {   \
      ::base::abort_with_msg( fmt::format(          \
          "bad unwrap, original error: {}",         \
          STRING_JOIN( __e, __LINE__ ).error() ) ); \
    }                                               \
  }

#define HAS_VALUE_OR_RET( ... )                  \
  {                                              \
    if( auto xp__ = __VA_ARGS__; !bool( xp__ ) ) \
      return std::move( xp__.error() );          \
  }

/****************************************************************
** Try to unwrap a wrapped type.
*****************************************************************/
#define UNWRAP_CHECK( a, e )                                   \
  auto&& STRING_JOIN( __e, __LINE__ ) = e;                     \
  if( !STRING_JOIN( __e, __LINE__ ).has_value() ) {            \
    ::base::abort_with_msg(                                    \
        fmt::format( "bad unwrap, original error: {}",         \
                     STRING_JOIN( __e, __LINE__ ).error() ) ); \
  }                                                            \
  auto&& BASE_IDENTITY( a ) = *STRING_JOIN( __e, __LINE__ )

#define UNWRAP_RETURN( var, ... )                             \
  auto&& STRING_JOIN( __x, __LINE__ ) = __VA_ARGS__;          \
  if( !STRING_JOIN( __x, __LINE__ ).has_value() )             \
    return std::move( STRING_JOIN( __x, __LINE__ ).error() ); \
  auto&& var = *STRING_JOIN( __x, __LINE__ );

/****************************************************************
** Variants
*****************************************************************/
#define ASSIGN_CHECK_V( ref, v_expr, type )                  \
  auto&& STRING_JOIN( __x, __LINE__ )   = v_expr;            \
  auto*  STRING_JOIN( __ptr, __LINE__ ) = std::get_if<type>( \
      &STRING_JOIN( __x, __LINE__ ).as_std() );             \
  BASE_CHECK( STRING_JOIN( __ptr, __LINE__ ) != nullptr );   \
  auto& BASE_IDENTITY( ref ) = *STRING_JOIN( __ptr, __LINE__ )

/****************************************************************
** If false, create generic error.
*****************************************************************/
#define TRUE_OR_RETURN_GENERIC_ERR( ... ) \
  PP_ONE_OR_MORE_ARGS( TRUE_OR_RETURN_GENERIC_ERR, __VA_ARGS__ )

#define TRUE_OR_RETURN_GENERIC_ERR_SINGLE( expr ) \
  if( auto&& __e = expr; !__e ) {                 \
    return ::base::GenericError::create(          \
        "condition failed: " #expr );             \
  }

#define TRUE_OR_RETURN_GENERIC_ERR_MULTI( expr, ... )      \
  if( auto&& __e = expr; !__e ) {                          \
    return ::base::GenericError::create(                   \
        std::string( "condition failed: " #expr ) + ": " + \
        fmt::format( __VA_ARGS__ ) );                      \
  }

/****************************************************************
** Create a GenericError.
*****************************************************************/
#define GENERIC_ERROR( ... ) \
  ::base::GenericError::create( fmt::format( __VA_ARGS__ ) )

/****************************************************************
** Non-macro helpers.
*****************************************************************/
namespace base {

namespace detail {

// Format the message output on a CHECK-fail; just makes it look
// nice regardless of whether the message is empty or not.
std::string check_msg( char const*        expr,
                       std::string const& msg );

} // namespace detail

[[noreturn]] void abort_with_msg(
    std::string_view msg, SourceLoc loc = SourceLoc::current() );

struct GenericError {
  // This is to force storing these by pointer; storing them by
  // value is not efficient because they are too large, and it is
  // expected that they will only get created when errors happen
  // anyway.
  static std::unique_ptr<GenericError> create(
      std::string_view what,
      SourceLoc        loc = SourceLoc::current() ) {
    return std::unique_ptr<GenericError>(
        new GenericError( what, loc ) );
  }

  std::string what;
  SourceLoc   loc;

private:
  GenericError( std::string_view what_,
                SourceLoc        loc_ = SourceLoc::current() )
    : what( what_ ), loc( loc_ ) {}
};

static_assert(
    std::is_nothrow_move_constructible_v<GenericError> );
static_assert( std::is_nothrow_move_assignable_v<GenericError> );

using generic_err = std::unique_ptr<GenericError>;

void to_str( generic_err const& ge, std::string& out );

} // namespace base

template<>
struct fmt::formatter<::base::generic_err>
  : fmt::formatter<std::string> {
  template<typename FormatContext>
  auto format( ::base::generic_err const& o,
               FormatContext&             ctx ) {
    std::string out;
    to_str( o, out );
    return fmt::formatter<std::string>::format( out, ctx );
  }
};
