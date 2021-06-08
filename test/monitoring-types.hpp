/****************************************************************
**monitoring-types.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-08.
*
* Description: Some types used for tracking during unit tests.
*
*****************************************************************/
#pragma once

// Must be last.
#include "test/catch-common.hpp"

namespace testing {

/****************************************************************
** Tracker
*****************************************************************/
// Tracks number of constructions and destructions.
struct Tracker {
  static inline int constructed      = 0;
  static inline int destructed       = 0;
  static inline int copied           = 0;
  static inline int move_constructed = 0;
  static inline int move_assigned    = 0;
  static void       reset() {
    constructed = destructed = copied = move_constructed =
        move_assigned                 = 0;
  }

  Tracker() noexcept { ++constructed; }
  Tracker( Tracker const& ) noexcept { ++copied; }
  Tracker( Tracker&& ) noexcept { ++move_constructed; }
  ~Tracker() noexcept { ++destructed; }

  Tracker& operator=( Tracker const& ) = delete;
  Tracker& operator                    =( Tracker&& ) noexcept {
    ++move_assigned;
    return *this;
  }
};

} // namespace testing

DEFINE_FORMAT_( testing::Tracker, "Tracker" );
FMT_TO_CATCH( testing::Tracker );
