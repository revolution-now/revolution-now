/****************************************************************
**formations.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2026-05-17.
*
* Description: Generates land overlay formations.
*
*****************************************************************/
#pragma once

// config
#include "config/range-helpers.rds.hpp"

// ss
#include "ss/terrain-enums.rds.hpp"

// refl
#include "refl/enum-map.hpp"

namespace rn {

/****************************************************************
** Fwd. Decls.
*****************************************************************/
struct IRand;
struct MapMatrix;

enum class e_biome;
enum class e_terrain_formation;

/****************************************************************
** Public API.
*****************************************************************/
// All non-arctic land tiles will be set with forest.
void set_all_forest( MapMatrix& m );

void generate_formation(
    IRand& rand, MapMatrix& m, e_terrain_formation formation,
    refl::enum_map<e_biome, config::DoublePercent> const&
        densities,
    double growth, int max_length );

} // namespace rn
