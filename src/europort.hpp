/****************************************************************
**europort.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-07-08.
*
* Description:
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "aliases.hpp"
#include "id.hpp"

namespace rn {

Vec<UnitId> europort_units_on_dock();  // Sorted by arrival.
Vec<UnitId> europort_units_in_port();  // Sorted by arrival.
Vec<UnitId> europort_units_inbound();  // to old world
Vec<UnitId> europort_units_outbound(); // to new world

} // namespace rn
