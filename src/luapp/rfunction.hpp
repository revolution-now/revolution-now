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

// base
#include "base/macros.hpp"

namespace lua {

/****************************************************************
** rfunction
*****************************************************************/
struct rfunction : public any {
  using Base = any;

  using Base::Base;

  friend base::maybe<rfunction> lua_get( cthread L, int idx,
                                         tag<rfunction> );

  template<Pushable... Args>
  any operator()( Args&&... args ) const;

  template<GettableOrVoid R = void, Pushable... Args>
  R call( Args&&... args ) const;

  template<GettableOrVoid R = void, Pushable... Args>
  error_type_for_return_type<R> pcall( Args&&... args ) const;
};

/****************************************************************
** Template implementations.
*****************************************************************/
template<Pushable... Args>
any rfunction::operator()( Args&&... args ) const {
  cthread L = this_cthread();
  push( L, *this );
  return call_lua_unsafe_and_get<any>( L, FWD( args )... );
}

template<GettableOrVoid R, Pushable... Args>
R rfunction::call( Args&&... args ) const {
  cthread L = this_cthread();
  push( L, *this );
  return call_lua_unsafe_and_get<R>( L, FWD( args )... );
}

template<GettableOrVoid R, Pushable... Args>
error_type_for_return_type<R> rfunction::pcall(
    Args&&... args ) const {
  cthread L = this_cthread();
  push( L, *this );
  return call_lua_safe_and_get<R>( L, FWD( args )... );
}

} // namespace lua
