/****************************************************************
**flag.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-02.
*
* Description: Fancy bools.
*
*****************************************************************/
#pragma once

#include "config.hpp"

#include <utility>

namespace base {

/****************************************************************
** one_way_flag
*****************************************************************/
// This is a boolean flag that starts off unset, but once it is
// set it can never be reset except if the object is replace
// through assignment.
struct one_way_flag {
  // Sets the flag and returns true if it was changed, which will
  // only be the case the first time it is set.
  bool set() { return !std::exchange( on_, true ); }

  operator bool() const { return on_; }

 private:
  bool on_ = false;
};

} // namespace base
