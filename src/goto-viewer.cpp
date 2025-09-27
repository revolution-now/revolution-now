/****************************************************************
**goto-viewer.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-09-27.
*
* Description: Implementations of IGotoMapViewer.
*
*****************************************************************/
#include "goto-viewer.hpp"

// Revolution Now
#include "roles.hpp"
#include "visibility.hpp"

// config
#include "config/unit-type.rds.hpp"

// ss
#include "ss/nation.hpp"
#include "ss/unit.hpp"

using namespace std;

namespace rn {

namespace {

using ::base::maybe;
using ::gfx::point;

maybe<e_player> viz_player( SSConst const& ss,
                            Unit const& unit ) {
  if( auto const viewer =
          player_for_role( ss, e_player_role::viewer );
      !viewer.has_value() )
    // If the entire map is currently visible then we allow the
    // unit to use that, regardless of player.
    return viewer;
  // The entire map is not visible, so use the one of the unit
  // that is actually moving.
  return unit.player_type();
}

} // namespace

/****************************************************************
** GotoMapViewer
*****************************************************************/
GotoMapViewer::GotoMapViewer( SSConst const& ss,
                              Unit const& unit )
  : viz_( create_visibility_for( ss, viz_player( ss, unit ) ) ),
    is_ship_( unit.desc().ship ) {
  CHECK( viz_ ); // should never fail.
}

GotoMapViewer::~GotoMapViewer() = default;

bool GotoMapViewer::can_enter_tile( point const p ) const {
  if( !viz_->on_map( p ) ) return false;
  switch( viz_->visible( p ) ) {
    case e_tile_visibility::hidden:
      return true;
    case e_tile_visibility::fogged:
    case e_tile_visibility::clear:
      break;
  }
  MapSquare const& square = viz_->square_at( p );
  switch( square.surface ) {
    case e_surface::land:
      return !is_ship_;
    case e_surface::water:
      return is_ship_;
  }
}

} // namespace rn
