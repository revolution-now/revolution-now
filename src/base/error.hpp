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
#include "adl-tag.hpp"
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
      "fatal error", fmt::format( "" __VA_ARGS__ ) ) );

/****************************************************************
** Main check-fail macros.
*****************************************************************/
// Non-testing code should use this. Code written in unit test
// cpp files should use BASE_CHECK to avoid a collision with a
// similar Catch2 symbol.
#define CHECK( ... )    BASE_CHECK( __VA_ARGS__ )
#define CHECK_EQ( ... ) BASE_CHECK_EQ( __VA_ARGS__ )

#define BASE_CHECK( a, ... )                             \
  {                                                      \
    if( !( a ) ) {                                       \
      ::base::abort_with_msg( ::base::detail::check_msg( \
          #a, fmt::format( "" __VA_ARGS__ ) ) );         \
    }                                                    \
  }

// We use a lambda for this so that we can reference the input
// expressions twice (once to evaluate them and once to print
// them) without having to store them in a local variable or ref-
// erence, because doing so is tricky since if we stored them by
// value then we have copy move issues, while if we stored them
// by reference then we'd potentially have lifetime issues with
// expressions like a.b.c(), since reference lifetime extension
// only applies to the final value returned. By passing the
// values into a lambda we know that they will only be evaluated
// once and that their entire expressions will stay alive until
// the lambda returns.
#define BASE_CHECK_EQ( x, y )                                \
  []( auto&& l, auto&& r ) {                                 \
    if( l != r )                                             \
      ::base::abort_with_msg( ::base::detail::check_msg(     \
          #x " == " #y, fmt::format( "{} != {}", l, r ) ) ); \
  }( x, y )

// DCHECK is CHECK in debug builds, but compiles to nothing in
// release builds.
#ifdef NDEBUG
#  define DCHECK( ... )
#else
#  define DCHECK( ... ) BASE_CHECK( __VA_ARGS__ )
#endif

/****************************************************************
** Equality/Inequality checks
*****************************************************************/
// Greater or equal
#define CHECK_GE( a, b )                          \
  {                                               \
    auto const& STRING_JOIN( __a, __LINE__ ) = a; \
    auto const& STRING_JOIN( __b, __LINE__ ) = b; \
    CHECK( STRING_JOIN( __a, __LINE__ ) >=        \
               STRING_JOIN( __b, __LINE__ ),      \
           "{} is not >= than {}",                \
           STRING_JOIN( __a, __LINE__ ),          \
           STRING_JOIN( __b, __LINE__ ) );        \
  }

// Less or equal
#define CHECK_LE( a, b )                          \
  {                                               \
    auto const& STRING_JOIN( __a, __LINE__ ) = a; \
    auto const& STRING_JOIN( __b, __LINE__ ) = b; \
    CHECK( STRING_JOIN( __a, __LINE__ ) <=        \
               STRING_JOIN( __b, __LINE__ ),      \
           "{} is not <= than {}",                \
           STRING_JOIN( __a, __LINE__ ),          \
           STRING_JOIN( __b, __LINE__ ) );        \
  }

// Less than
#define CHECK_LT( ... ) \
  PP_N_OR_MORE_ARGS_2( CHECK_LT, __VA_ARGS__ )

#define CHECK_LT_SINGLE( a, b )                                 \
  {                                                             \
    auto const& STRING_JOIN( __a, __LINE__ ) = a;               \
    auto const& STRING_JOIN( __b, __LINE__ ) = b;               \
    CHECK( STRING_JOIN( __a, __LINE__ ) <                       \
               STRING_JOIN( __b, __LINE__ ),                    \
           "{} is not < than {}", STRING_JOIN( __a, __LINE__ ), \
           STRING_JOIN( __b, __LINE__ ) );                      \
  }

#define CHECK_LT_MULTI( a, b, ... )               \
  {                                               \
    auto const& STRING_JOIN( __a, __LINE__ ) = a; \
    auto const& STRING_JOIN( __b, __LINE__ ) = b; \
    CHECK( STRING_JOIN( __a, __LINE__ ) <         \
               STRING_JOIN( __b, __LINE__ ),      \
           "{} is not < than {}: {}",             \
           STRING_JOIN( __a, __LINE__ ),          \
           STRING_JOIN( __b, __LINE__ ),          \
           fmt::format( __VA_ARGS__ ) );          \
  }

// Greater than
#define CHECK_GT( ... ) \
  PP_N_OR_MORE_ARGS_2( CHECK_GT, __VA_ARGS__ )

#define CHECK_GT_SINGLE( a, b )                                 \
  {                                                             \
    auto const& STRING_JOIN( __a, __LINE__ ) = a;               \
    auto const& STRING_JOIN( __b, __LINE__ ) = b;               \
    CHECK( STRING_JOIN( __a, __LINE__ ) >                       \
               STRING_JOIN( __b, __LINE__ ),                    \
           "{} is not > than {}", STRING_JOIN( __a, __LINE__ ), \
           STRING_JOIN( __b, __LINE__ ) );                      \
  }

#define CHECK_GT_MULTI( a, b, ... )               \
  {                                               \
    auto const& STRING_JOIN( __a, __LINE__ ) = a; \
    auto const& STRING_JOIN( __b, __LINE__ ) = b; \
    CHECK( STRING_JOIN( __a, __LINE__ ) >         \
               STRING_JOIN( __b, __LINE__ ),      \
           "{} is not > than {}: {}",             \
           STRING_JOIN( __a, __LINE__ ),          \
           STRING_JOIN( __b, __LINE__ ),          \
           fmt::format( __VA_ARGS__ ) );          \
  }

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

#define CHECK_HAS_VALUE_MSG( e, ... )                    \
  {                                                      \
    auto const& STRING_JOIN( __e, __LINE__ ) = e;        \
    if( !bool( STRING_JOIN( __e, __LINE__ ) ) ) {        \
      ::base::abort_with_msg( fmt::format(               \
          "bad unwrap, original error: {}, message: {}", \
          STRING_JOIN( __e, __LINE__ ).error(),          \
          fmt::format( __VA_ARGS__ ) ) );                \
    }                                                    \
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

#define UNWRAP_BREAK( a, e )                             \
  auto&& STRING_JOIN( __e, __LINE__ ) = e;               \
  if( !STRING_JOIN( __e, __LINE__ ).has_value() ) break; \
  auto&& BASE_IDENTITY( a ) = *STRING_JOIN( __e, __LINE__ )

#define UNWRAP_CHECK_MSG( a, e, ... )                  \
  auto&& STRING_JOIN( __e, __LINE__ ) = e;             \
  if( !STRING_JOIN( __e, __LINE__ ).has_value() ) {    \
    ::base::abort_with_msg( fmt::format(               \
        "bad unwrap, original error: {}, message: {}", \
        STRING_JOIN( __e, __LINE__ ).error(),          \
        fmt::format( __VA_ARGS__ ) ) );                \
  }                                                    \
  auto&& BASE_IDENTITY( a ) = *STRING_JOIN( __e, __LINE__ )

#define UNWRAP_RETURN( var, ... )                             \
  auto&& STRING_JOIN( __x, __LINE__ ) = __VA_ARGS__;          \
  if( !STRING_JOIN( __x, __LINE__ ).has_value() )             \
    return std::move( STRING_JOIN( __x, __LINE__ ) ).error(); \
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
** If false, return formatted string as error.
*****************************************************************/
#define RETURN_IF_FALSE( ... ) \
  PP_ONE_OR_MORE_ARGS( RETURN_IF_FALSE, __VA_ARGS__ )

#define RETURN_IF_FALSE_SINGLE( e )          \
  if( auto&& __expr = e; !bool( __expr ) ) { \
    return fmt::format( #e );                \
  }

#define RETURN_IF_FALSE_MULTI( e, ... )      \
  if( auto&& __expr = e; !bool( __expr ) ) { \
    return fmt::format( __VA_ARGS__ );       \
  }

#define RETURN_IF( ... ) \
  PP_ONE_OR_MORE_ARGS( RETURN_IF, __VA_ARGS__ )

#define RETURN_IF_SINGLE( e )               \
  if( auto&& __expr = e; bool( __expr ) ) { \
    return fmt::format( #e );               \
  }

#define RETURN_IF_MULTI( e, ... )           \
  if( auto&& __expr = e; bool( __expr ) ) { \
    return fmt::format( __VA_ARGS__ );      \
  }

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

void to_str( generic_err const& ge, std::string& out, ADL_t );

// This is supposed to be an exception in the std::exception hi-
// erarchy. If it is not then it will still work, but will return
// "unknown exception type" or something like that.
std::string rethrow_and_get_msg( std::exception_ptr p );

} // namespace base

template<>
struct fmt::formatter<::base::generic_err>
  : fmt::formatter<std::string> {
  template<typename FormatContext>
  auto format( ::base::generic_err const& o,
               FormatContext&             ctx ) {
    std::string out;
    to_str( o, out, base::ADL );
    return fmt::formatter<std::string>::format( out, ctx );
  }
};
