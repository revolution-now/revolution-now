/****************************************************************
**call.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-15.
*
* Description: Functions for calling Lua function via C++.
*
*****************************************************************/
#include "call.hpp"

// luapp
#include "c-api.hpp"
#include "types.hpp"

// {fmt}
#include "fmt/format.h"

using namespace std;

namespace lua {

namespace internal {

lua_expect<int> call_lua_from_cpp(
    cthread L, base::maybe<int> nresults, bool safe,
    base::function_ref<void()> push_args ) {
  c_api C( L );
  CHECK( C.stack_size() >= 1 );
  // Get size of stack before function was pushed.
  int starting_stack_size = C.stack_size() - 1;

  int before_args = C.stack_size();
  push_args();
  int after_args = C.stack_size();

  int num_args = after_args - before_args;

  if( safe ) {
    HAS_VALUE_OR_RET( C.pcall(
        num_args, nresults.value_or( c_api::multret() ) ) );
  } else {
    C.call( num_args, nresults.value_or( c_api::multret() ) );
  }

  int actual_nresults = C.stack_size() - starting_stack_size;
  CHECK_GE( actual_nresults, 0 );
  if( nresults ) { CHECK( nresults == actual_nresults ); }
  return actual_nresults;
}

lua_expect<resume_result> call_lua_resume_from_cpp(
    cthread L_toresume, base::function_ref<void()> push_args ) {
  c_api C_toresume( L_toresume );

  thread_status status = C_toresume.status();
  // First time we are resuming the coroutine?
  if( status == thread_status::ok ) {
    CHECK( C_toresume.stack_size() >= 1 );
    CHECK( C_toresume.type_of( -1 ) == type::function ||
           C_toresume.type_of( -1 ) == type::table );
  }

  int before_args = C_toresume.stack_size();
  push_args();
  int after_args = C_toresume.stack_size();

  int nargs = after_args - before_args;
  DCHECK( nargs >= 0 );
  return C_toresume.resume_or_reset( L_toresume, nargs );
}

void pop_call_results( cthread L, int n ) {
  c_api( L ).pop( n );
}

std::string lua_error_bad_return_values(
    cthread L, int nresults, std::string_view ret_type_name ) {
  std::string msg = fmt::format(
      "native code expected type `{}' as a return value (which "
      "requires {} Lua value{}), but the values returned by Lua "
      "were not convertible to that native type.  The Lua "
      "values received were: [",
      ret_type_name, nresults, nresults > 1 ? "s" : "" );
  for( int i = -nresults; i <= -1; ++i ) {
    msg += type_name( L, -i );
    if( i != -1 ) msg += ", ";
  }
  msg += "].";
  return msg;
}

[[noreturn]] void throw_lua_error_bad_return_values(
    cthread L, int nresults, string_view ret_type_name ) {
  throw_lua_error( L, "{}",
                   lua_error_bad_return_values(
                       L, nresults, ret_type_name ) );
}

void adjust_return_values( cthread L, int nresults_returned,
                           int nresults_needed ) {
  c_api C( L );
  CHECK( C.stack_size() >= nresults_returned );
  int diff = nresults_needed - nresults_returned;

  if( diff > 0 ) {
    for( int i = 0; i < diff; ++i ) C.push( nil );
  } else if( diff < 0 ) {
    C.pop( -diff );
  }
  DCHECK( C.stack_size() >= nresults_needed );
}

lua_valid resetthread( cthread L ) {
  return c_api( L ).resetthread();
}

} // namespace internal

} // namespace lua
