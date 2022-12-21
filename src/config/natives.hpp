/****************************************************************
**natives.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-11-22.
*
* Description: Config info for the natives.
*
*****************************************************************/
#pragma once

// Rds
#include "natives.rds.hpp"

namespace rn {

inline auto const& unit_attr( e_native_unit_type type ) {
  return config_natives.unit_types[type];
}

} // namespace rn
