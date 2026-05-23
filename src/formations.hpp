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

namespace rn {

/****************************************************************
** Fwd. Decls.
*****************************************************************/
struct IRand;
struct MapMatrix;

enum class e_terrain_formation;

/****************************************************************
** Public API.
*****************************************************************/
// All non-arctic land tiles will be set with forest.
void set_all_forest( MapMatrix& m );

void generate_formation( IRand& rand, MapMatrix& m,
                         e_terrain_formation formation,
                         double density, double growth,
                         int max_length );

} // namespace rn
