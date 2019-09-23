/****************************************************************
**lua-ext.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-09-19.
*
* Description: Sol2 Lua type customizations.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "aliases.hpp"
#include "coord.hpp"
#include "id.hpp"
#include "typed-int.hpp"

LUA_TYPED_INT( ::rn::X );
LUA_TYPED_INT( ::rn::Y );
LUA_TYPED_INT( ::rn::W );
LUA_TYPED_INT( ::rn::H );

LUA_TYPED_INT( ::rn::UnitId );

namespace rn {

/****************************************************************
** Coord
*****************************************************************/
template<typename Handler>
inline bool sol_lua_check( sol::types<::rn::Coord>, lua_State* L,
                           int index, Handler&& handler,
                           sol::stack::record& tracking ) {
  int  absolute_index = lua_absindex( L, index );
  bool success        = sol::stack::check<sol::table>(
      L, absolute_index, handler );
  tracking.use( 1 );
  if( !success ) return false;
  auto table = sol::stack::get<sol::table>( L, absolute_index );
  success    = ( table["x"].get_type() == sol::type::number ) &&
            ( table["y"].get_type() == sol::type::number );
  if( !success ) return false;
  return success;
}

inline ::rn::Coord sol_lua_get( sol::types<::rn::Coord>,
                                lua_State* L, int index,
                                sol::stack::record& tracking ) {
  int  absolute_index = lua_absindex( L, index );
  auto table = sol::stack::get<sol::table>( L, absolute_index );
  ::rn::Coord coord{table["x"].get<X>(), table["y"].get<Y>()};
  tracking.use( 1 );
  return coord;
}

inline int sol_lua_push( sol::types<::rn::Coord>, lua_State* L,
                         ::rn::Coord const& coord ) {
  sol::state_view st( L );

  auto table = sol::lua_value( st, {} ).as<sol::table>();
  table["x"] = coord.x;
  table["y"] = coord.y;
  int amount = sol::stack::push( L, table );

  /* amount will be 1: int pushes 1 item. */
  return amount;
}

} // namespace rn
