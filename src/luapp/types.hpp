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
#include "base/fmt.hpp"
#include "base/safe-num.hpp"
#include "base/valid.hpp"

struct lua_State;

namespace luapp {

/****************************************************************
** expect/valid
*****************************************************************/
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

/****************************************************************
** Lua types
*****************************************************************/
enum class e_lua_type {
  nil           = 0,
  boolean       = 1,
  lightuserdata = 2,
  number        = 3,
  string        = 4,
  table         = 5,
  function      = 6,
  userdata      = 7,
  thread        = 8
};

void to_str( luapp::e_lua_type t, std::string& out );

inline constexpr int kNumLuaTypes = 9;

/****************************************************************
** nil
*****************************************************************/
struct nil_t {
  auto operator<=>( nil_t const& ) const = default;
};

inline constexpr nil_t nil;

void to_str( luapp::nil_t, std::string& out );

/****************************************************************
** Numeric types
*****************************************************************/
using boolean  = base::safe::boolean;
using floating = base::safe::floating<double>;
using void_p   = base::safe::void_p;

// Derive from it so that we can add an implicit conversion to
// floating, which we need in order to be able to easily convert
// integrals to floats when comparing the two like Lua would do.
struct integer : public base::safe::integer<long long> {
  using Base = base::safe::integer<long long>;

  using Base::Base;
  using Base::operator<=>;
  using Base::operator long long;
  using Base::get;

  operator floating() const noexcept {
    return static_cast<double>( get() );
  }
};

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

} // namespace luapp

/****************************************************************
** {fmt}
*****************************************************************/
TOSTR_TO_FMT( luapp::e_lua_type );
TOSTR_TO_FMT( luapp::nil_t );

DEFINE_FORMAT( luapp::integer, "{}", o.get() );
