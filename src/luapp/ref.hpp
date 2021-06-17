/****************************************************************
**ref.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-11.
*
* Description: RAII wrapper around a Lua registry reference.
*
*****************************************************************/
#pragma once

// luapp
#include "cthread.hpp"
#include "ext.hpp"

// base
#include "base/zero.hpp"

// C++ standard library
#include <string>

namespace lua {

/****************************************************************
** reference
*****************************************************************/
struct reference : base::RuleOfZero<reference, int> {
  // Signal that objects of this type should not be treated as
  // any old user object.
  using luapp_internal = void;

  reference() = delete;
  reference( cthread st, int ref ) noexcept;

  cthread this_cthread() const noexcept;

  // Pushes nil if there is no reference. Note that we don't push
  // onto the Lua state that is held instead the reference ob-
  // ject, since that could correspond to a different thread.
  friend void lua_push( cthread L, reference const& r );

private:
  using Base = base::RuleOfZero<reference, int>;
  friend Base;

  // Implement base::RuleOfZero.
  void free_resource();

  // Implement base::RuleOfZero.
  int copy_resource() const;

protected:
  cthread L; // not owned.
};

static_assert( Pushable<reference> );
static_assert( !Gettable<reference> );
static_assert( LuappInternal<reference> );

namespace internal {

// Asks Lua to compare the top to values on the stack and returns
// true if they are equal, and pops them.
bool compare_top_two_and_pop( cthread L );

} // namespace internal

template<typename T>
bool operator==( reference const& r, T const& rhs ) {
  cthread L = r.this_cthread();
  push( L, r );
  push( L, rhs );
  return internal::compare_top_two_and_pop( L );
}

void to_str( reference const& r, std::string& out );

} // namespace lua
