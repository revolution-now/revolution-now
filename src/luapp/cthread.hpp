/****************************************************************
**cthread.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-11.
*
* Description: Type to represent L.
*
*****************************************************************/
#pragma once

// base
#include "base/safe-num.hpp"

struct lua_State;

namespace lua {

/****************************************************************
** lua_State
*****************************************************************/
// Currently, this serves the purpose of having a representation
// of the C Lua state in the lua namespace that can implicitely
// convert to lua_State in order to automatically enable ADL in
// that namespace in any function that takes it. We also take
// this opportunity to rename lua_State to something containing
// the word "thread", since it is more accurately a thread (one
// could argue that it should have been called lua_Thread).
//
// Note that this should be passed around by value, unlike L
// which is passed by pointer.
struct cthread : public base::safe::pointer<::lua_State> {
  using Base = base::safe::pointer<::lua_State>;
  using Base::Base;
};

// These must be bit-wise exactly the same.
static_assert( sizeof( cthread ) == sizeof( ::lua_State* ) );

} // namespace lua
