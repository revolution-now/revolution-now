/****************************************************************
**helper.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-05-29.
*
* Description: High-level Lua helper object.
*
*****************************************************************/
#pragma once

// luapp
#include "c-api.hpp"
#include "func-push.hpp"
#include "types.hpp"

// base
#include "base/cc-specific.hpp"
#include "base/error.hpp"
#include "base/meta.hpp"
#include "base/unique-func.hpp"

// {fmt}
#include "fmt/format.h"

// C++ standard library
#include <string_view>
#include <tuple>

namespace lua {

struct helper {
  helper( cthread helper );

  // Expects a function on the top of the stack, and will call it
  // with the given C++ arguments. Returns the number of argu-
  // ments returned by the Lua function.
  template<typename... Args>
  int call( Args&&... args );

  // Expects a function on the top of the stack, and will pcall
  // it with the given C++ arguments. If successful, returns the
  // number of arguments returned by the Lua function.
  template<typename... Args>
  lua_expect<int> pcall( Args&&... args ) noexcept;

private:
  c_api C;
};

template<typename... Args>
int helper::call( Args&&... args ) {
  CHECK( C.stack_size() >= 1 );
  CHECK( C.type_of( -1 ) == type::function );
  // Get size of stack before function was pushed.
  int starting_stack_size = C.stack_size() - 1;

  ( C.push( std::forward<Args>( args ) ), ... );
  C.call( /*nargs=*/sizeof...( Args ),
          /*nresults=*/c_api::multret() );

  int nresults = C.stack_size() - starting_stack_size;
  CHECK_GE( nresults, 0 );
  return nresults;
}

template<typename... Args>
lua_expect<int> helper::pcall( Args&&... args ) noexcept {
  CHECK( C.stack_size() >= 1 );
  CHECK( C.type_of( -1 ) == type::function );
  // Get size of stack before function was pushed.
  int starting_stack_size = C.stack_size() - 1;

  ( C.push( std::forward<Args>( args ) ), ... );
  HAS_VALUE_OR_RET( C.pcall( /*nargs=*/sizeof...( Args ),
                             /*nresults=*/c_api::multret() ) );

  int nresults = C.stack_size() - starting_stack_size;
  CHECK_GE( nresults, 0 );
  return nresults;
}

} // namespace lua
