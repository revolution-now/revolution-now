/****************************************************************
**state.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-05-29.
*
* Description: High-level Lua state object.
*
*****************************************************************/
#pragma once

// C++ standard library
#include <memory>
#include <string_view>

struct lua_State;

namespace luapp {

struct c_api;

struct state {
  state();
  // TODO: this constructor can be removed (as well as the lua.h
  // include) after migration away from sol2.
  state( ::lua_State* state );

  void tables( std::string_view path ) noexcept;

  // TODO: use this as a model for loading a piece of code once
  // into the registry. Can remove eventually.
  void tables_slow( std::string_view path ) noexcept;
  int  tables_func_ref = noref();

  c_api& api() noexcept;

private:
  state( state const& ) = delete;
  state( state&& )      = delete;
  state& operator=( state const& ) = delete;
  state& operator=( state&& ) = delete;

  static int noref();

  std::unique_ptr<c_api> C;
};

} // namespace luapp
