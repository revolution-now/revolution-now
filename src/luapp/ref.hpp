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

// C++ standard library
#include <string>

namespace lua {

/****************************************************************
** reference
*****************************************************************/
struct reference {
  reference() = delete;
  reference( cthread st, int ref ) noexcept;
  ~reference() noexcept;

  void release() noexcept;

  reference( reference const& ) noexcept;
  reference& operator=( reference const& ) noexcept;

  cthread this_cthread() const noexcept;

  // Pushes nil if there is no reference. Note that we don't push
  // onto the Lua state that is held instead the reference ob-
  // ject, since that could correspond to a different thread.
  friend void push( cthread L, reference const& r );

protected:
  cthread L; // not owned.

private:
  int ref_;
};

bool operator==( reference const& lhs, reference const& rhs );

bool operator==( reference const& r, nil_t );
bool operator==( reference const& r, boolean const& b );
bool operator==( reference const& r, lightuserdata const& lud );
bool operator==( reference const& r, integer const& i );
bool operator==( reference const& r, floating const& f );

void to_str( reference const& r, std::string& out );

} // namespace lua
