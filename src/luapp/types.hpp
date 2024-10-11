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
#include "ext.hpp"

// base
#include "base/maybe.hpp"
#include "base/safe-num.hpp"

namespace lua {

/****************************************************************
** helpers
*****************************************************************/
int upvalue_index( int upvalue );

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

void to_str( type t, std::string& out, base::tag<type> );

inline constexpr int kNumLuaTypes = 9;

char const* type_name( cthread L, int idx ) noexcept;
type        type_of_idx( cthread L, int idx ) noexcept;
void        pop_stack( cthread L, int n ) noexcept;

// clang-format off
template<typename T>
requires( Pushable<T> )
type type_of( cthread L, T&& o ) noexcept {
  // clang-format on
  lua::push( L, o );
  type res = type_of_idx( L, -1 );
  pop_stack( L, nvalues_for<T>() );
  return res;
}

// clang-format off
template<typename T>
requires( HasCthread<T> && Pushable<T> )
type type_of( T&& o ) noexcept {
  // clang-format on
  cthread L = o.this_cthread();
  return type_of( L, std::forward<T>( o ) );
}

/****************************************************************
** nil
*****************************************************************/
struct nil_t {
  auto operator<=>( nil_t const& ) const = default;
};

inline constexpr nil_t nil;

void to_str( nil_t, std::string& out, base::tag<nil_t> );

void lua_push( cthread L, nil_t );
void lua_get( cthread L, int idx, tag<nil_t> ) = delete;

/****************************************************************
** void
*****************************************************************/
template<>
struct type_traits<void> {
  static constexpr int nvalues = 0;
};

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

  // We need to get rid of this from the base method because oth-
  // erwise the implicit conversion to void* causes {fmt} to
  // refuse to format it (even though it can format void point-
  // ers). It tests if the type (lightuserdata) can be converted
  // to void* (which lightuserdata and maybe<lightuserdata> can
  // be, and thus it probably thinks that it (lightuserdata) is
  // some kind of non-void pointer, which it does not support
  // formatting.
  operator void*() const noexcept = delete;
};

bool operator==( lightuserdata const& l,
                 lightuserdata const& r );
bool operator!=( lightuserdata const& l,
                 lightuserdata const& r );

void to_str( lightuserdata const& lud, std::string& out,
             base::tag<lightuserdata> );

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
** extension point: push
*****************************************************************/
void lua_push( cthread L, boolean b );
void lua_push( cthread L, integer i );
void lua_push( cthread L, floating f );
void lua_push( cthread L, lightuserdata lud );
void lua_push( cthread L, std::string_view sv );

template<>
struct type_traits<std::string_view> {
  using storage_type = std::string;
};

/****************************************************************
** extension point: lua_get
*****************************************************************/
base::maybe<bool>    lua_get( cthread L, int idx, tag<bool> );
base::maybe<int>     lua_get( cthread L, int idx, tag<int> );
base::maybe<double>  lua_get( cthread L, int idx, tag<double> );
base::maybe<void*>   lua_get( cthread L, int idx, tag<void*> );
base::maybe<boolean> lua_get( cthread L, int idx, tag<boolean> );
base::maybe<integer> lua_get( cthread L, int idx, tag<integer> );
base::maybe<floating>      lua_get( cthread L, int idx,
                                    tag<floating> );
base::maybe<lightuserdata> lua_get( cthread L, int idx,
                                    tag<lightuserdata> );
base::maybe<std::string>   lua_get( cthread L, int idx,
                                    tag<std::string> );
void                       lua_get( cthread L, int idx,
                                    tag<std::string_view> ) = delete;
void lua_get( cthread L, int idx, tag<char const*> ) = delete;

} // namespace lua
