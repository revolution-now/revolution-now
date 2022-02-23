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
#include "expect.hpp"

// base
#include "base/function-ref.hpp"

// C++ standard library
#include <vector>

namespace rn {

bool colony_exists( ColonyId id );
// TODO: replace usages of this with ColoniesState::colony_for.
Colony& colony_from_id( ColonyId id );
// TODO: replace usages of this with ColoniesState::all.
std::vector<ColonyId> colonies_all();
std::vector<ColonyId> colonies_all( e_nation n );

// Apply a function to all colonies.
void map_colonies( base::function_ref<void( Colony& )> func );

// DO NOT call this directly as it will return a Colony that is
// not yet fully valid, e.g., it will have no units or buildings
// in it.
ColonyId create_colony( e_nation nation, Coord const& where,
                        std::string_view name );

// TODO: replace usages of this with
// ColoniesState::maybe_from_coord.
maybe<ColonyId> colony_from_coord( Coord const& coord );
// TODO: replace usages of this with
// ColoniesState::maybe_from_name.
maybe<ColonyId> colony_from_name( std::string_view name );
// TODO: replace usages of this with ColoniesState::from_rect.
std::vector<ColonyId> colonies_in_rect( Rect const& rect );

} // namespace rn
