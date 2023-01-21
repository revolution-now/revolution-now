/****************************************************************
**land-production.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-14.
*
* Description: Computes what is produced on a land square.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "maybe.hpp"

// ss
#include "ss/colony-enums.rds.hpp"
#include "ss/commodity.rds.hpp"
#include "ss/difficulty.rds.hpp"
#include "ss/fathers.rds.hpp"
#include "ss/unit-type.rds.hpp"

// gfx
#include "gfx/coord.hpp"

namespace rn {

struct MapSquare;
struct Player;
struct TerrainState;

int production_on_square(
    e_outdoor_job job, TerrainState const& terrain_state,
    refl::enum_map<e_founding_father, bool> const& fathers,
    e_unit_type unit_type, Coord where );

int food_production_on_center_square( MapSquare const& square,
                                      e_difficulty difficulty );

// We will call this for various outdoor jobs and then choose one
// based on what produces the most. Note that the rules that de-
// termine production of a commodity in the center square are
// different from on normal squares.
int commodity_production_on_center_square(
    e_outdoor_commons_secondary_job job, MapSquare const& square,
    Player const& player, e_difficulty difficulty );

// If a colony were on this square, which secondary good would it
// choose to produce. This will just run through all of the
// available ones and choose the one that yields the most. If
// none of them yield anything it will return nothing, which can
// happen e.g. on arctic tiles.
//
// In the original game, the commodity produced seems to be what-
// ever yields the most, with some kind of tie-breaking heuris-
// tic. E.g., on a rain forest square the game chooses sugar (and
// it yields one, which is tied with fur and ore). But if the
// rain forest square has minerals in it then the game selects
// ore.
maybe<e_outdoor_commons_secondary_job> choose_secondary_job(
    Player const& player, MapSquare const& square,
    e_difficulty difficulty );

// Given a unit activity this will convert it to an outdoor job,
// if the mapping exists. Basically this can be used to take a
// colonist expertise and find the outdoor job corresponding to
// the job that it is skilled at.
maybe<e_outdoor_job> outdoor_job_for_expertise(
    e_unit_activity activity );

e_unit_activity activity_for_outdoor_job( e_outdoor_job job );

e_commodity commodity_for_outdoor_job( e_outdoor_job job );

} // namespace rn
