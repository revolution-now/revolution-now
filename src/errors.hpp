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

// base-util
#include "base-util/macros.hpp"

// {fmt}
#include "fmt/format.h"
#include "fmt/ostream.h"

// c++ standard library
#include <memory>
#include <stdexcept>
#include <string_view>

// This is obviously a no-op but is an attempt to suppress some
// compiler warnings about parenthesis around macro parameters
// in inconsistent ways by different compilers.
#define ID_( a ) a

/****************************************************************
**Error Checking Macros
*****************************************************************/

#define DIE( msg ) rn::die( __FILE__, __LINE__, msg )

#define SHOULD_NOT_BE_HERE \
  DIE( "programmer error: should not be here" )

// This is used to wrap calls to fmt::format that want
// compile-time format string checking. It assumes that the first
// argument is some kind of constexpr expression (maybe has to
// specifically be a string literal, not sure) and will wrap that
// first argument in the fmt() macro. The fmt() macro is enabled
// with the FMT_STRING_ALIAS=1 compiler definition.
#define FMT_SAFE( fmt_str, ... ) \
  fmt::format( fmt( fmt_str ), ##__VA_ARGS__ )

namespace detail {

template<typename... Args>
auto check_msg( char const* expr, Args... args ) {
  std::string msg    = fmt::format( args... );
  std::string suffix = msg.empty() ? "." : ( ": " + msg );
  return fmt::format( "CHECK( {} ) failed{}", expr, suffix );
}

} // namespace detail

// This CHECK macro should be used most of the time to do
// assertions.
//
// Note: It is important that this macro should only evaluate `a`
// once in case evaluating it either has side effects or is
// expensive. Hopefully the implementation below conforms to
// this.
#define CHECK( a, ... )                                         \
  if( !( a ) ) {                                                \
    DIE( detail::check_msg( #a, FMT_SAFE( "" __VA_ARGS__ ) ) ); \
  }

// This takes care to only evaluate (b) once, since it may be
// e.g. a function call. This function will evaluate (b) which is
// expected to result in a std::optional (ideally it should be
// returned from a function where elision is possible, otherwise
// there may be a copy happening). It will assign that std::op-
// tional by value (hopefully elided) to a local anonymous vari-
// able and will then inspect it to see if it has a value. If
// not, error is thrown. If it does have a value then another
// local reference variable will be created to reference the
// value inside the optional, so there should not be any unneces-
// sary copies.
//
// The ID_ is to suppress warnings about parenthesis around
// macro parameters.
#define ASSIGN_CHECK_OPT( a, b )          \
  auto STRING_JOIN( __x, __LINE__ ) = b;  \
  if( !( STRING_JOIN( __x, __LINE__ ) ) ) \
    DIE( TO_STRING( b ) " is false." );   \
  auto& ID_( a ) = *STRING_JOIN( __x, __LINE__ )

// Same as above but returns on failure instead of throwing. As
// can be seen, this macro should be used inside functions that
// return a default-initialized value to mean failure, such as
// bool or optional.
#define ASSIGN_OR_RETURN( a, b )                     \
  auto STRING_JOIN( __x, __LINE__ ) = b;             \
  if( !( STRING_JOIN( __x, __LINE__ ) ) ) return {}; \
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
  if( !( a ) ) { DIE( TO_STRING( b ) " is false." ); }

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

// All code in RN should use these functions to interact with
// stack traces.

// Get a stack at the location where this function is called;
// will include the stack from inside this function.
ND StackTrace stack_trace_here();

// Print stack trace to stderr with sensible options and skip
// the latest `skip` number of frames.  That is, if stack
// traces have been enabled in the build.
void print_stack_trace( StackTrace const& st, int skip = 0 );

void die( char const* file, int line, std::string_view msg );

} // namespace rn
