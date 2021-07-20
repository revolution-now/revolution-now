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

/****************************************************************
** function signatures
*****************************************************************/
// This represents the signature of a Lua C API function that in-
// teracts with a Lua state (i.e., takes the Lua state as first
// parameter). Any such API function could interact with the Lua
// state and thus could potentially throw an error (at least most
// of them do). So the code that wraps Lua C API calls to detect
// those errors will use this signature.
//
// Takes args by value since they will only be simple types.
template<typename R, typename... Args>
using LuaApiFunc = R( ::lua_State*, Args... );

// This represents the signature of a Lua C library (extension)
// method, i.e., a C function that is called from Lua.
using LuaCFunction = int( ::lua_State* );

// This needs to agree with Lua's lua_KContext.
using LuaKContext = intptr_t;

using LuaKFunction = int( ::lua_State*, int, LuaKContext );

/****************************************************************
** Concepts
*****************************************************************/
template<typename T>
concept HasCthread = requires( T const& o ) {
  { o.this_cthread() } -> std::same_as<cthread>;
};

} // namespace lua
