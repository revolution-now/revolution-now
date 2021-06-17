/****************************************************************
**thing.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-09.
*
* Description: C++ containers for Lua values/objects.
*
*****************************************************************/
#pragma once

// luapp
#include "any.hpp"
#include "ext.hpp"
#include "indexer.hpp"
#include "ref.hpp"
#include "rfunction.hpp"
#include "rstring.hpp"
#include "rtable.hpp"
#include "rthread.hpp"
#include "ruserdata.hpp"
#include "types.hpp"

// base
#include "base/error.hpp"
#include "base/fmt.hpp"
#include "base/variant.hpp"

// C++ standard library
#include <string>

namespace lua {

/****************************************************************
** thing
*****************************************************************/
// nil_t should be first so that it is selected as the default.
using thing_base =
    base::variant<nil_t, boolean, lightuserdata, integer,
                  floating, rstring, table, rfunction, userdata,
                  rthread>;

struct thing : public thing_base {
  using luapp_internal = void;

  using Base = thing_base;

  using Base::Base;
  using Base::operator=;

  bool operator==( thing const& rhs ) const noexcept;

  bool operator==( std::string_view rhs ) const noexcept;

  template<typename T>
  // clang-format off
  requires( !std::is_same_v<thing, T> &&
             std::is_constructible_v<thing, T const&> )
  bool operator==( T const& rhs ) const noexcept {
    // clang-format on
    return ( *this ) == thing( rhs );
  }

  std::string tostring() const noexcept;

  // Cannot index a thing.  Cast first to a table or userdata:
  //
  //   thing th = ...;
  //   th.as<table>()["key"] = ...;
  //
  template<typename U>
  auto operator[]( U&& idx ) noexcept = delete;

  template<typename T>
  thing( cthread L, T&& o ) noexcept {
    push( L, std::forward<T>( o ) );
    *this = pop( L );
  }

  template<typename IndexT, typename Predecessor>
  thing( indexer<IndexT, Predecessor> const& idxr ) noexcept {
    *this = idxr;
  }

  template<typename IndexT, typename Predecessor>
  thing& operator=(
      indexer<IndexT, Predecessor> const& idxr ) noexcept {
    cthread L = idxr.this_cthread();
    push( L, idxr );
    *this = pop( L );
    return *this;
  }

  thing( any a ) noexcept { *this = a; }

  thing& operator=( any a ) noexcept {
    cthread L = a.this_cthread();
    push( L, a );
    *this = pop( L );
    return *this;
  }

  // Follows Lua's rules, where every value is true except for
  // false and nil.
  operator bool() const noexcept;

  ::lua::type type() const noexcept;

  static thing pop( cthread L ) noexcept;
};

void push_thing( cthread L, thing const& th );

/****************************************************************
** Lua extension points.
*****************************************************************/
LUA_PUSH_FUNC( thing ) { push_thing( L, o ); }
base::maybe<thing> lua_get( cthread L, int idx, tag<thing> );
static_assert( Stackable<thing> );

/****************************************************************
** to_str
*****************************************************************/
void to_str( thing const& th, std::string& out );

} // namespace lua

/****************************************************************
** fmt
*****************************************************************/
TOSTR_TO_FMT( lua::thing );
