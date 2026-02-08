/****************************************************************
**test-map.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2026-01-04.
*
* Description: Generates maps for testing and prints them to the
*              console.
*
*****************************************************************/
#pragma once

namespace rn {

/****************************************************************
** Fwd. Decls.
*****************************************************************/
struct IEngine;
struct GameSetup;

/****************************************************************
** Public API.
*****************************************************************/
void testing_map_gen( IEngine& engine, bool reseed );

void testing_map_gen_stats( IEngine& engine );

void load_testing_game_setup( GameSetup& setup );

} // namespace rn
