/****************************************************************
**colony-evolve.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-04.
*
* Description: Evolves one colony one turn.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Rds
#include "colony-evolve.rds.hpp"

namespace rn {

struct Colony;
struct SS;
struct TS;

// Evolve the colony by one turn. This is not a coroutine for a
// few reasons: 1) ease of testability, 2) we want it to also be
// used for the AI players, 3) we want to be able to have a way
// to evolve a colony (e.g. for cheat mode) where we can control
// what is shown to the user.
ColonyEvolution evolve_colony_one_turn( SS& ss, TS& ts,
                                        Colony& colony );

} // namespace rn
