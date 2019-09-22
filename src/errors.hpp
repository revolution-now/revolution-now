/****************************************************************
**errors.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-12-04.
*
* Description: Error handling code.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "fmt-helper.hpp"

// base-util
#include "base-util/macros.hpp"
#include "base-util/pp.hpp"
#include "base-util/variant.hpp"

// expected-lite
#include "nonstd/expected.hpp"

// c++ standard library
#include <memory>
#include <stdexcept>
#include <string_view>

// This is obviously a no-op but is an attempt to suppress some
// compiler warnings about parenthesis around macro parameters
// in inconsistent ways by different compilers.
#define ID_( a ) a

/****************************************************************
**Error Formatting Macros
*****************************************************************/

// Should not use this one directly, use FATAL()
#define FATAL_( msg ) rn::die( __FILE__, __LINE__, msg )

#define SHOULD_NOT_BE_HERE \
  FATAL_( "programmer error: should not be here" )

#define NOT_IMPLEMENTED \
  FATAL_( "programmer error: need to implement this" )

#define MUST_IMPROVE_IMPLEMENTATION_BEFORE_USE                \
  FATAL_(                                                     \
      "the implementation of this function must be improved " \
      "before use" )

#define WARNING_THIS_FUNCTION_HAS_NOT_BEEN_TESTED         \
  FATAL_(                                                 \
      "the implementation of this function has not been " \
      "verified." )

// This is used to wrap calls to fmt::format that want
// compile-time format string checking. It assumes that the first
// argument is some kind of constexpr expression (maybe has to
// specifically be a string literal, not sure) and will wrap that
// first argument in the fmt() macro. The fmt() macro is enabled
// with the FMT_STRING_ALIAS=1 compiler definition.
#define FMT_SAFE( fmt_str, ... ) \
  fmt::format( fmt( fmt_str ), ##__VA_ARGS__ )

namespace rn::detail {

std::string check_msg( char const*        expr,
                       std::string const& msg );

bool check_inline( bool b, char const* msg );

} // namespace rn::detail

/****************************************************************
**Error Checking Macros
*****************************************************************/

// This is used when you want to just fail but with formatting in
// the error message.
#define FATAL( ... )               \
  FATAL_( ::rn::detail::check_msg( \
      "fatal error", FMT_SAFE( "" __VA_ARGS__ ) ) );

// This CHECK macro should be used most of the time to do
// assertions.
//
// Note: It is important that this macro should only evaluate `a`
// once in case evaluating it either has side effects or is
// expensive. Hopefully the implementation below conforms to
// this.
#define RN_CHECK( a, ... )                  \
  if( !( a ) ) {                            \
    FATAL_( ::rn::detail::check_msg(        \
        #a, FMT_SAFE( "" __VA_ARGS__ ) ) ); \
  }

// Non-testing code should use this. Code written in unit test
// cpp files should use RN_CHECK to avoid a collision with a sim-
// ilar Catch2 symbol.
#define CHECK( ... ) RN_CHECK( __VA_ARGS__ )

// DCHECK is CHECK in debug builds, but compiles to nothing in
// release builds.
#ifdef NDEBUG
#  define DCHECK( ... )
#else
#  define DCHECK( ... ) RN_CHECK( __VA_ARGS__ )
#endif

// Use this when the check is on a boolean and the check itself
// must be an expression and return a boolean.
#define CHECK_INL( b ) ::rn::detail::check_inline( b, #b )

// This takes care to only evaluate (b) once, since it may be
// e.g. a function call. This function will evaluate (b) which is
// expected to result in a std::optional (ideally it should be
// returned from a function where elision is possible, otherwise
// there may be a copy happening). It will then keep the re-
// sulting variant alive by assign it to a reference and will
// then inspect it to see if it has a value. If not, error is
// thrown. If it does have a value then another local reference
// variable will be created to reference the value inside the op-
// tional, so there should not be any unnecessary copies.
//
// The ID_ is to suppress warnings about parenthesis around
// macro parameters.
#define ASSIGN_CHECK_OPT( a, b )               \
  auto&& STRING_JOIN( __x, __LINE__ ) = b;     \
  if( !( STRING_JOIN( __x, __LINE__ ) ) )      \
    FATAL_( TO_STRING( b ) " has no value." ); \
  auto&& ID_( a ) = *STRING_JOIN( __x, __LINE__ )

// This takes care to only evaluate `v_expr` once, since it may
// be e.g. a function call. The function will evaluate `v_expr`
// which is expected to result in a std::variant. It will then
// keep the resulting variant alive by assign it to a reference.
// Finally it will check that the variant is of the right type
// and, if so, it will assign it to a local reference of the
// given name. The ID_ is to suppress warnings about parenthesis
// around macro parameters.
//
// Example:
//
//   std::variant<Object, int> v = ...;
//
//   ASSIGN_CHECK_V( obj, v, Object );
//   obj.method();
//
#define ASSIGN_CHECK_V( ref, v_expr, type )               \
  auto&& STRING_JOIN( __x, __LINE__ ) = v_expr;           \
  auto*  STRING_JOIN( __ptr, __LINE__ ) =                 \
      std::get_if<type>( &STRING_JOIN( __x, __LINE__ ) ); \
  RN_CHECK( STRING_JOIN( __ptr, __LINE__ ) != nullptr );  \
  auto& ID_( ref ) = *STRING_JOIN( __ptr, __LINE__ )

// Same as above but returns on failure instead of throwing. As
// can be seen, this macro should be used inside functions that
// return a default-initialized value to mean failure, such as
// bool, std::optional, or std::expected.
#define ASSIGN_OR_RETURN( a, b )                     \
  auto STRING_JOIN( __x, __LINE__ ) = b;             \
  if( !( STRING_JOIN( __x, __LINE__ ) ) ) return {}; \
  auto& ID_( a ) = *STRING_JOIN( __x, __LINE__ )

// One that does not return a value.
#define ASSIGN_OR_RETURN_( a, b )                 \
  auto STRING_JOIN( __x, __LINE__ ) = b;          \
  if( !( STRING_JOIN( __x, __LINE__ ) ) ) return; \
  auto& ID_( a ) = *STRING_JOIN( __x, __LINE__ )

// This takes care to only evaluate (b) once, since it may be
// e.g. a function call. This function will evaluate (b) which is
// expected to result in a value that can be tested for true'-
// ness, and where a "true" value is interpreted as success. Oth-
// erwise an error is thrown.
//
// The ID_ is to suppress warnings about parenthesis around
// macro parameters.
#define ASSIGN_CHECK( a, b ) \
  auto ID_( a ) = b;         \
  if( !( a ) ) { FATAL_( TO_STRING( b ) " is false." ); }

// Here `expression` will be evaluated only once, so it can be an
// expensive operation that yields a variant; if it yields a
// temporary then that temporary will be kept alive for the
// duration of the scope in which this macro is called. The
// `type` can have a `const` on it if needed; in fact this will
// be required if the `expression` yields a const value. If the
// variant does not have the expected type then there will be a
// check failure.
#define GET_CHECK_VARIANT( dest, expression, type )         \
  auto& STRING_JOIN( __x, __LINE__ ) = expression;          \
  RN_CHECK(                                                 \
      ::util::holds<std::remove_cv_t<type>>(                \
          STRING_JOIN( __x, __LINE__ ) ),                   \
      "variant expected to be holding type `{}` but it is " \
      "holding index {}",                                   \
      #type, STRING_JOIN( __x, __LINE__ ).index() )         \
  type& ID_( dest ) = std::get<std::remove_cv_t<type>>(     \
      STRING_JOIN( __x, __LINE__ ) )

/****************************************************************
**Stack Trace Reporting
*****************************************************************/
// This decides when to enable stack traces in the build.
#ifndef NDEBUG
#  define STACK_TRACE_ON
#endif

// Forward declare this so that we can expose a pointer to it but
// hide implementation so that we can avoid including back-
// ward.hpp.
#ifdef STACK_TRACE_ON
namespace backward {
class StackTrace;
}
#endif

namespace rn {

#ifdef STACK_TRACE_ON
struct StackTrace {
  // We must define all of these StackTrace standard functions in
  // the cpp file since defining them here in the header would
  // require knowledge (in the header) of the unique_ptr's de-
  // structor (which is actually needed in the StackTrace con-
  // structor in case it throws an exception) which cannot be
  // generated in the header because we leave it as an incomplete
  // type here.
  StackTrace();
  ~StackTrace();
  StackTrace( std::unique_ptr<backward::StackTrace>&& st_ );
  StackTrace( StackTrace&& st );

  // Pointer so that we can avoid including backward.hpp here.
  std::unique_ptr<backward::StackTrace> st;
};
#else
struct StackTrace {};
#endif

struct exception_with_bt : public std::runtime_error {
  exception_with_bt( std::string msg, StackTrace st_ )
    : std::runtime_error( msg ), st( std::move( st_ ) ) {}
  StackTrace st; // will be empty in non-debug builds.
};

// An exception to throw when you just want to exit. Mainly just
// for use during development.
struct exception_exit : public std::exception {};

// All code in RN should use these functions to interact with
// stack traces.

// Get a stack at the location where this function is called;
// will include the stack from inside this function.
ND StackTrace stack_trace_here();

// Print stack trace to stderr with sensible options and skip
// the latest `skip` number of frames.  That is, if stack
// traces have been enabled in the build.
void print_stack_trace( StackTrace const& st, int skip = 0 );

[[noreturn]] void die( char const* file, int line,
                       std::string_view msg );

/****************************************************************
**Expected
*****************************************************************/

struct Unexpected {
  std::string what;
  size_t      line;
  fs::path    file;
};

// All `expected` types should use this so that they have a
// common error type.
template<typename T>
using expect = ::nonstd::expected<T, ::rn::Unexpected>;

// If there are >1 args then the 1st one must be a format string.
#define UNEXPECTED( ... ) \
  PP_ONE_OR_MORE_ARGS( UNEXPECTED, __VA_ARGS__ )

// Use this to construct unexpected's because it records file and
// line no.
#define UNEXPECTED_SINGLE( str )               \
  ::nonstd::make_unexpected( ::rn::Unexpected{ \
      fmt::format( "{}", str ), __LINE__, __FILE__} )

#define UNEXPECTED_MULTI( fmt_str, ... )                    \
  ::nonstd::make_unexpected(                                \
      ::rn::Unexpected{fmt::format( fmt_str, __VA_ARGS__ ), \
                       __LINE__, __FILE__} )

#define CHECK_XP( e )                                     \
  {                                                       \
    auto const& STRING_JOIN( __e, __LINE__ ) = e;         \
    RN_CHECK( STRING_JOIN( __e, __LINE__ ).has_value(),   \
              "unexpected:{}:{}: {}",                     \
              STRING_JOIN( __e, __LINE__ ).error().file,  \
              STRING_JOIN( __e, __LINE__ ).error().line,  \
              STRING_JOIN( __e, __LINE__ ).error().what ) \
  }

#define ASSIGN_CHECK_XP( a, b )             \
  auto STRING_JOIN( __x, __LINE__ ) = b;    \
  CHECK_XP( STRING_JOIN( __x, __LINE__ ) ); \
  auto& ID_( a ) = *STRING_JOIN( __x, __LINE__ )

// Used for converting a value of one expected type into another
// in the case when: 1) it is in an unexpected state, and 2) both
// expected types share the same error type. This is useful when
// recieving one unexpected type as a return value of a function
// and then propagating it as another type:
//
//   expect<string> foo( ... ) {}
//
//   expect<int> bar( ... ) {
//     auto res = foo();
//     if( !res.has_value() )
//       return propagate_unexpected( res );
//     ...
//   }
//
template<typename T>
auto propagate_unexpected( ::rn::expect<T> const& e ) {
  RN_CHECK( !e.has_value() );
  return ::nonstd::make_unexpected( e.error() );
}

#define XP_OR_RETURN( var, expr ) \
  auto var = expr;                \
  if( !var.has_value() ) return propagate_unexpected( var );

} // namespace rn

namespace fmt {

// {fmt} formatter for formatting `expected` whose contained
// types is formattable.
template<typename T>
struct formatter<::rn::expect<T>> : formatter_base {
  template<typename FormatContext>
  auto format( ::rn::expect<T> const& o, FormatContext& ctx ) {
    return formatter_base::format(
        fmt::format( o.has_value()
                         ? fmt::format( "{}", *o )
                         : fmt::format( "<unexpected:{}:{}: {}>",
                                        o.error().file.stem(),
                                        o.error().line,
                                        o.error().what ) ),
        ctx );
  }
};

} // namespace fmt
