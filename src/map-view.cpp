/****************************************************************
**map-view.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-07-27.
*
* Description: Logic for handling player interactions in the map
*              view, separate from the plane, for testability.
*
*****************************************************************/
#include "map-view.hpp"

// Revolution Now
#include "game-options.hpp"
#include "roles.hpp"
#include "society.hpp"
#include "unit-mgr.hpp"
#include "visibility.hpp"

// ss
#include "ss/colonies.rds.hpp"
#include "ss/land-view.rds.hpp"
#include "ss/ref.hpp"
#include "ss/settings.rds.hpp"

using namespace std;

namespace rn {

namespace {

using ::base::maybe;
using ::base::nothing;
using ::gfx::point;

} // namespace

/****************************************************************
** Map revealed.
*****************************************************************/
void reveal_entire_map( SS& ss, TS& ts ) {
  ss.land_view.map_revealed = MapRevealed::entire{};
  disable_game_option( ss, ts,
                       e_game_menu_option::show_indian_moves );
  disable_game_option( ss, ts,
                       e_game_menu_option::show_foreign_moves );
  // Redraw.
  update_map_visibility( ts, /*player=*/nothing );
}

/****************************************************************
** Colonies.
*****************************************************************/
maybe<ColonyId> can_open_colony_on_tile( IVisibility const& viz,
                                         point const tile ) {
  auto const viz_colony = viz.colony_at( tile );
  if( !viz_colony.has_value() ) return nothing;
  if( viz_colony->id == 0 ) return nothing;
  bool const compatible_ownership =
      !viz.player().has_value() ||
      *viz.player() == viz_colony->player;
  if( !compatible_ownership ) return nothing;
  return viz_colony->id;
}

/****************************************************************
** Units.
*****************************************************************/
vector<UnitId> can_activate_units_on_tile(
    SSConst const& ss, IVisibility const& viz,
    point const tile ) {
  vector<UnitId> res;
  // Can't activate what we can't see.
  if( viz.visible( tile ) != e_tile_visibility::clear )
    return res;
  // This needs to be "active" and not "viewer" because the
  // purpose of this is to activate units to move, therefore it
  // has to be those units' turn.
  auto const active =
      player_for_role( ss, e_player_role::active );
  if( !active.has_value() ) return res;
  auto const society = society_on_square( ss, tile );
  if( !society.has_value() ) return res;
  auto const european = society->get_if<Society::european>();
  if( !european.has_value() ) return res;
  if( european->player != *active ) return res;
  res = euro_units_from_coord_recursive( ss.units, tile );
  return res;
}

} // namespace rn
