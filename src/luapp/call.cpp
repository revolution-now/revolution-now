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
  CHECK( C.type_of( -1 ) == type::function );
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

void pop_call_results( cthread L, int n ) {
  c_api( L ).pop( n );
}

[[noreturn]] void throw_lua_error_bad_return_values(
    cthread L, int nresults, string_view ret_type_name ) {
  std::string msg = fmt::format(
      "native code expected type `{}' as a return value (which "
      "requires {} Lua value{}), but the values returned by Lua "
      "were not convertible to that native type.  The Lua "
      "values received were: [",
      ret_type_name, nresults, nresults > 1 ? "s" : "" );
  for( int i = 1; i <= nresults; ++i ) {
    msg += type_name( L, -i );
    if( i != nresults ) msg += ", ";
  }
  msg += "].";
  throw_lua_error( L, "{}", msg );
}

} // namespace internal

} // namespace lua
