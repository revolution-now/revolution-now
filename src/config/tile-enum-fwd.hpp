/****************************************************************
**tile-enum-fwd.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-02-13.
*
* Description: Forward declaration of e_tile for use in headers.
*
*****************************************************************/
#pragma once

namespace rn {

// So that we don't have to include the full tile enum in head-
// ers, which significantly reduces compile time when that enum
// is changed. Forward declaring enums is apparently allowed so
// long as we forward declare with the correct underlying type.
// The default should be fine since that's how we declare the
// generated Rds enums; but if it is wrong then we will get an
// error in the associated translation unit where the real enum
// is included. So there should be no risk for ODR violations.
enum class e_tile;

} // namespace rn
