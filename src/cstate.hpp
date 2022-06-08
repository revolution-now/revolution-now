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
#include "colony-id.hpp"
#include "coord.hpp"
#include "maybe.hpp"

// C++ standard library
#include <vector>

namespace rn {

struct ColoniesState;
struct Colony;

// FIXME: deprecated
bool colony_exists( ColonyId id );

bool colony_exists( ColoniesState const& colonies_state,
                    ColonyId             id );

// TODO: replace usages of this with ColoniesState::colony_for.
Colony& colony_from_id( ColonyId id );

// TODO: replace usages of this with
// ColoniesState::maybe_from_coord.
maybe<ColonyId> colony_from_coord( Coord coord );

} // namespace rn
