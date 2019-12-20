/****************************************************************
**colony-state.hpp
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
#include "aliases.hpp"
#include "colony.hpp"
#include "errors.hpp"
#include "sg-macros.hpp"

// function_ref
#include "tl/function_ref.hpp"

// C++ standard library
#include <vector>

namespace rn {

DECLARE_SAVEGAME_SERIALIZERS( Colony );

// Note this will return a Colony that is not yet fully valid,
// e.g., it will have no units or buildings in it.
expect<ColonyId> create_colony( e_nation         nation,
                                Coord const&     where,
                                std::string_view name );

bool          colony_exists( ColonyId id );
Colony&       colony_from_id( ColonyId id );
Vec<ColonyId> colonies_all( Opt<e_nation> n = {} );

// Apply a function to all colonies.
void map_colonies( tl::function_ref<void( Colony& )> func );

// Should not be holding any references to the colony after this.
void destroy_colony( ColonyId id );

/****************************************************************
** Map Ownership
*****************************************************************/
FlatSet<ColonyId> const& colonies_from_coord( Coord const& c );
Vec<ColonyId>            colonies_in_rect( Rect const& rect );

} // namespace rn
