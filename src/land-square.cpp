/****************************************************************
**land-square.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-02-01.
*
* Description: Represents a single square of land.
*
*****************************************************************/
#include "land-square.hpp"

// Revolution Now
//#include "dummy.hpp"

using namespace std;

namespace rn {

namespace {
//
} // namespace

expect<> LandSquare::check_invariants_safe() const {
  return xp_success_t{};
}

/****************************************************************
** Public API
*****************************************************************/
// ...

} // namespace rn
