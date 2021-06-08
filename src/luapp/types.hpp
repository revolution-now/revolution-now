/****************************************************************
**types.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-05-30.
*
* Description: Types common to all luapp modules.
*
*****************************************************************/
#pragma once

// base
#include "base/valid.hpp"

// {fmt}
#include "fmt/format.h"

struct lua_State;

namespace luapp {

// The type we use for reporting errors raised by lua.
using lua_error_t = std::string;
// We can change the error type in the future, but it must at
// least be constructible from a std::string.
static_assert(
    std::is_constructible_v<lua_error_t, std::string> );

// valid_or
using lua_valid = base::valid_or<lua_error_t>;
lua_valid lua_invalid( lua_error_t err );

// expect
template<typename T>
using lua_expect = base::expect<T, lua_error_t>;

template<typename T>
lua_expect<T> lua_expected( T&& arg ) {
  return base::expected<lua_error_t>( std::forward<T>( arg ) );
}

template<typename T, typename Arg>
lua_expect<T> lua_unexpected( Arg&& arg ) {
  return base::unexpected<T, lua_error_t>(
      std::forward<Arg>( arg ) );
}

enum class e_lua_type {
  nil            = 0,
  boolean        = 1,
  light_userdata = 2,
  number         = 3,
  string         = 4,
  table          = 5,
  function       = 6,
  userdata       = 7,
  thread         = 8
};

inline constexpr int kNumLuaTypes = 9;

struct nil_t {};

inline constexpr nil_t nil;

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

/****************************************************************
** to_str
*****************************************************************/
void to_str( luapp::e_lua_type t, std::string& out );

} // namespace luapp

/****************************************************************
** {fmt}
*****************************************************************/
namespace fmt {

// {fmt} formatter for e_lua_type.
template<>
struct formatter<luapp::e_lua_type>
  : fmt::formatter<std::string> {
  using formatter_base = fmt::formatter<std::string>;
  template<typename FormatContext>
  auto format( luapp::e_lua_type o, FormatContext& ctx ) {
    std::string res;
    to_str( o, res );
    return formatter_base::format( res, ctx );
  }
};

} // namespace fmt
