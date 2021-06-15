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

// luapp
#include "cthread.hpp"

// base
#include "base/fmt.hpp"
#include "base/safe-num.hpp"
#include "base/valid.hpp"

namespace lua {

/****************************************************************
** Macros
*****************************************************************/
// This macro is used to define a push function that can only be
// called for a specific type, preventing issues with ambiguious
// overloads due to implicit conversions.
#define LUA_PUSH_FUNC( type )                                   \
  template<typename T>                                          \
  requires( std::is_same_v<std::remove_cvref_t<T>, type> ) void \
  push( cthread L, T const& o )

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
** helpers
*****************************************************************/
int upvalue_index( int upvalue );

// All get( ... ) calls should go here, then this should dispatch
// using ADL. It will look in the ::lua namespace as well as the
// namespace associated wtih T, if any.
template<typename T>
base::maybe<T> get( cthread L, int idx ) noexcept {
  return get( L, idx, static_cast<T*>( nullptr ) );
}

/****************************************************************
** Lua types
*****************************************************************/
enum class type {
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

void to_str( type t, std::string& out );

inline constexpr int kNumLuaTypes = 9;

/****************************************************************
** nil
*****************************************************************/
struct nil_t {
  auto operator<=>( nil_t const& ) const = default;
};

inline constexpr nil_t nil;

void to_str( nil_t, std::string& out );

void push( cthread L, nil_t );
void get( cthread L, int idx, nil_t* ) = delete;

void get( cthread L, nil_t );

/****************************************************************
** value types
*****************************************************************/
using boolean  = base::safe::boolean;
using floating = base::safe::floating<double>;
using integer  = base::safe::integer<long long>;

// This is just a value type.
struct lightuserdata : public base::safe::pointer<void> {
  using Base = base::safe::pointer<void>;
  using Base::Base;
};

void push( cthread L, boolean b );
void push( cthread L, integer i );
void push( cthread L, floating f );
void push( cthread L, lightuserdata lud );
void push( cthread L, std::string_view sv );

base::maybe<bool>          get( cthread L, int idx, bool* );
base::maybe<int>           get( cthread L, int idx, int* );
base::maybe<double>        get( cthread L, int idx, double* );
base::maybe<void*>         get( cthread L, int idx, void** );
base::maybe<boolean>       get( cthread L, int idx, boolean* );
base::maybe<integer>       get( cthread L, int idx, integer* );
base::maybe<floating>      get( cthread L, int idx, floating* );
base::maybe<lightuserdata> get( cthread L, int idx,
                                lightuserdata* );
base::maybe<std::string> get( cthread L, int idx, std::string* );
void get( cthread L, int idx, std::string_view* ) = delete;
void get( cthread L, int idx, char const* )       = delete;

void to_str( lightuserdata const& lud, std::string& out );

bool operator==( nil_t const& l, boolean const& r );
bool operator==( nil_t const& l, lightuserdata const& r );
bool operator==( nil_t const& l, integer const& r );
bool operator==( nil_t const& l, floating const& r );
bool operator==( boolean const& l, lightuserdata const& r );
bool operator==( boolean const& l, integer const& r );
bool operator==( boolean const& l, floating const& r );
bool operator==( lightuserdata const& l, integer const& r );
bool operator==( lightuserdata const& l, floating const& r );
bool operator==( integer const& l, floating const& r );

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

} // namespace lua

/****************************************************************
** {fmt}
*****************************************************************/
TOSTR_TO_FMT( lua::type );
TOSTR_TO_FMT( lua::nil_t );
TOSTR_TO_FMT( lua::lightuserdata );

DEFINE_FORMAT( lua::integer, "{}", o.get() );
