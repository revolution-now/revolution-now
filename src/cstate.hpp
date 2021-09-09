/****************************************************************
**cstate.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-12-20.
*
* Description: Holds the Colony objects and tracks them.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "colony.hpp"
#include "error.hpp"

// base
#include "base/function-ref.hpp"

// C++ standard library
#include <vector>

namespace rn {

bool                  colony_exists( ColonyId id );
Colony&               colony_from_id( ColonyId id );
std::vector<ColonyId> colonies_all( maybe<e_nation> n = {} );

// Apply a function to all colonies.
void map_colonies( base::function_ref<void( Colony& )> func );

// DO NOT call this directly as it will return a Colony that is
// not yet fully valid, e.g., it will have no units or buildings
// in it.
expect<ColonyId, std::string> cstate_create_colony(
    e_nation nation, Coord const& where, std::string_view name );

// DO NOT call this directly as it will not properly remove units
// or check for errors. Should not be holding any references to
// the colony after this.
void cstate_destroy_colony( ColonyId id );

maybe<ColonyId>       colony_from_coord( Coord const& coord );
maybe<ColonyId>       colony_from_name( std::string_view name );
std::vector<ColonyId> colonies_in_rect( Rect const& rect );

} // namespace rn
