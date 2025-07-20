/****************************************************************
**show-anim.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-06-29.
*
* Description: Decides whether an event should be animated.
*
*****************************************************************/
#include "show-anim.hpp"

// Revolution Now
#include "anim-builder.hpp"
#include "roles.hpp"
#include "visibility.hpp"

// ss
#include "ss/players.rds.hpp"
#include "ss/ref.hpp"
#include "ss/settings.rds.hpp"
#include "ss/terrain.hpp"

// rds
#include "rds/switch-macro.hpp"

using namespace std;

namespace rn {

namespace {

using ::gfx::point;

bool show_moves_for_society( SSConst const& ss,
                             Society const& society ) {
  auto const& options =
      ss.settings.in_game_options.game_menu_options;
  SWITCH( society ) {
    using enum e_game_menu_option;
    CASE( european ) { return options[show_foreign_moves]; }
    CASE( native ) { return options[show_indian_moves]; }
  }
}

bool should_animate_society( SSConst const& ss,
                             maybe<e_player> const viewer,
                             Society const& society ) {
  if( show_moves_for_society( ss, society ) ) return true;
  // Since the relevant game option flag does not allow us to au-
  // tomatically show the moves, we will now try to determine if
  // the society is "friendly" or not. However, the definition of
  // what constitutes "friendly" depends on a few things.
  SWITCH( society ) {
    CASE( native ) { return false; }
    CASE( european ) {
      if( viewer.has_value() )
        // We are viewing from one player's perspective, so a
        // foreign power is any other player.
        return european.player == *viewer;
      // We see the entire map. So in that case we need to check
      // the player control. Since if the player has revealed the
      // entire map they can still turn off foreign moves and not
      // see the moves of AI players. Another possibility is that
      // there are no human players in the game and it is the na-
      // tives' turn. In that case it also seems sensible to go
      // based on control. So just assume that the AI players are
      // the ones that the viewer would consider "foreign".
      UNWRAP_CHECK_T( Player const& european_player,
                      ss.players.players[european.player] );
      return european_player.control == e_player_control::human;
    }
  }
}

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
bool should_animate_tile( SSConst const& ss,
                          AnimatedTile const& anim_tile ) {
  point const tile = anim_tile.tile;
  CHECK( ss.terrain.square_exists( tile ) );
  auto const viz_viewer = create_visibility_for(
      ss, player_for_role( ss, e_player_role::viewer ) );
  bool const is_clear =
      viz_viewer->visible( tile ) == e_tile_visibility::clear;
  if( !is_clear ) return false;
  for( Society const& society : anim_tile.inhabitants )
    if( should_animate_society( ss, viz_viewer->player(),
                                society ) )
      return true;
  return false;
}

bool should_animate_seq( SSConst const& ss,
                         AnimationSequence const& seq ) {
  AnimationContents const contents =
      animated_contents( ss, seq );
  for( AnimatedTile const& anim_tile : contents.tiles )
    if( should_animate_tile( ss, anim_tile ) ) //
      return true;
  return false;
}

} // namespace rn
