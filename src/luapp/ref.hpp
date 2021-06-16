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
#include "types.hpp"

// base
#include "base/zero.hpp"

// C++ standard library
#include <string>

namespace lua {

/****************************************************************
** reference
*****************************************************************/
struct reference : public base::RuleOfZero<reference, int> {
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

bool operator==( reference const& lhs, reference const& rhs );

bool operator==( reference const& r, nil_t );
bool operator==( reference const& r, boolean const& b );
bool operator==( reference const& r, lightuserdata const& lud );
bool operator==( reference const& r, integer const& i );
bool operator==( reference const& r, floating const& f );

void to_str( reference const& r, std::string& out );

} // namespace lua
