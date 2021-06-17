/****************************************************************
**rfunction.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-11.
*
* Description: RAII holder for registry references to Lua
*              functions.
*
*****************************************************************/
#pragma once

// luapp
#include "any.hpp"
#include "call.hpp"
#include "ref.hpp"

// base
#include "base/fmt.hpp"
#include "base/macros.hpp"

namespace lua {

/****************************************************************
** rfunction
*****************************************************************/
struct rfunction : public reference {
  using Base = reference;

  using Base::Base;

  friend base::maybe<rfunction> lua_get( cthread L, int idx,
                                         tag<rfunction> );

  template<Pushable... Args>
  any operator()( Args&&... args );

  template<Gettable R = any, Pushable... Args>
  R call( Args&&... args );

  template<Gettable R = any, Pushable... Args>
  lua_expect<R> pcall( Args&&... args );
};

/****************************************************************
** Template implementations.
*****************************************************************/
template<Pushable... Args>
any rfunction::operator()( Args&&... args ) {
  cthread L = this_cthread();
  push( L, *this );
  return call_lua_unsafe_and_get<any>( L, FWD( args )... );
}

template<Gettable R, Pushable... Args>
R rfunction::call( Args&&... args ) {
  cthread L = this_cthread();
  push( L, *this );
  return call_lua_unsafe_and_get<R>( L, FWD( args )... );
}

template<Gettable R, Pushable... Args>
lua_expect<R> rfunction::pcall( Args&&... args ) {
  cthread L = this_cthread();
  push( L, *this );
  return call_lua_safe_and_get<any>( L, FWD( args )... );
}

} // namespace lua

/****************************************************************
** fmt
*****************************************************************/
TOSTR_TO_FMT( lua::rfunction );
