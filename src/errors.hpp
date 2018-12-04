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

#include "macros.hpp"

#include "base-util/macros.hpp"

#include <stdexcept>

/****************************************************************
**Error Checking Macros
*****************************************************************/

#define DIE( msg ) \
  { rn::die( __FILE__, __LINE__, msg ); }

// This is the CHECK macro which should be used most of the time,
// especially in performance-sensitive code because it does not
// contain any of the stream operations that CHECK_ has above.
// This is also why this macro should NOT be implemented in terms
// of CHECK_ above. After the PP is finished, this macro should
// expand from e.g.:
//
//   CHECK( x == y )
//
// into:
//
//   if( !(x == y) )
//     rn::die( "/.../source.cpp", 1234, "`x == y` is false." );
//
// Note: It is important that this macro should only evaluate a
// once in case evaluating it either has side effects or is ex-
// pensive. Hopefully the implementation below conforms to this.
#define CHECK( a ) \
  if( !( a ) ) { DIE( "`" TO_STRING( a ) "` is false." ); }

// Only use this variant if it is important that the user see an
// error message describing the problem. Check's that are
// checking for programmer errors should normally not need this,
// since the check failure will be accompanied by the source/line
// location and possibly stack trace for debugging. This macro is
// heavy because of the stream processing that it does, so one
// should prefer the CHECK() macro above by default in most
// cases.
//
// Note: It is important that this macro should NOT evaluate `b`
// unless `a` fails the test. Also, it should consist of a single
// compound statement. Also, it should only evaluate a and b once
// in case they have side effects.
#define CHECK_( a, b )                               \
  if( !( a ) ) {                                     \
    std::ostringstream out;                          \
    out << "CHECK( " << #a << " ) failed: ";         \
    /* NOLINTNEXTLINE(bugprone-macro-parentheses) */ \
    out << b;                                        \
    DIE( out.str() )                                 \
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
    DIE( TO_STRING( b ) " is false." )    \
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
  if( !( a ) ) { DIE( TO_STRING( b ) " is false." ) }

/****************************************************************
**Stack Trace Reporting
*****************************************************************/
// This decides when to enable stack traces in the build.
#ifndef NDEBUG
#define STACK_TRACE_ON
#endif

// Forward declare this so that we can expose a pointer to it but
// hide implementation so that we can backward.hpp.
#ifdef STACK_TRACE_ON
namespace backward {
class StackTrace;
}
#endif

namespace rn {

#ifdef STACK_TRACE_ON
struct StackTrace {
  // Pointer so that we can avoid including backward.hpp here.
  // We don't use unique_ptr here because then it requires us to
  // have a complete type. So this memory will be leaked, but
  // that's ok because we only use this on an error anyway.
  backward::StackTrace* st;
};
#else
struct StackTrace {};
#endif

struct exception_with_bt : public std::runtime_error {
  exception_with_bt( std::string msg, StackTrace st_ )
    : std::runtime_error( msg ), st{st_} {}
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

} // namespace rn
