/****************************************************************
**any.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-17.
*
* Description: RAII wrapper around registry reference to any Lua
*              type.
*
*****************************************************************/
#pragma once

// luapp
#include "cthread.hpp"
#include "ext.hpp"

// base
#include "base/fmt.hpp"
#include "base/zero.hpp"

// C++ standard library
#include <string>

namespace lua {

/****************************************************************
** any
*****************************************************************/
struct any : base::zero<any, int> {
  // Signal that objects of this type should not be treated as
  // any old user object.
  using luapp_internal = void;

  any() = delete;
  any( cthread st, int ref ) noexcept;

  cthread this_cthread() const noexcept;

  // Pushes nil if there is no reference. Note that we don't push
  // onto the Lua state that is held inside the any object, since
  // that could correspond to a different thread.
  friend void             lua_push( cthread L, any const& r );
  friend base::maybe<any> lua_get( cthread L, int idx,
                                   tag<any> );

private:
  using Base = base::zero<any, int>;
  friend Base;

  // Implement base::zero.
  void free_resource();

  // Implement base::zero.
  int copy_resource() const;

protected:
  cthread L; // not owned.
};

static_assert( Stackable<any> );
static_assert( LuappInternal<any> );

namespace internal {

// Asks Lua to compare the top to values on the stack and returns
// true if they are equal, and pops them.
bool compare_top_two_and_pop( cthread L );

} // namespace internal

template<typename T>
bool operator==( any const& r, T const& rhs ) {
  cthread L = r.this_cthread();
  push( L, r );
  push( L, rhs );
  return internal::compare_top_two_and_pop( L );
}

void to_str( any const& r, std::string& out );

} // namespace lua

/****************************************************************
** fmt
*****************************************************************/
TOSTR_TO_FMT( lua::any );
