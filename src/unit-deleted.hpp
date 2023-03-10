/****************************************************************
**unit-deleted.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-05.
*
* Description: Helper type to signal statically when a unit may
*              have been deleted after a function call in order
*              to help callers avoid use-after-free.
*
*****************************************************************/
#pragma once

namespace rn {

// A dummy type to help remind the caller that the unit may have
// disappeared as a result of the call. This works because maybe
// types are [[nodiscard]].
struct UnitDeleted final {};

} // namespace rn
