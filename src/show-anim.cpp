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

bool should_animate_society( SSConst const& ss,
                             maybe<e_player> const viewer,
                             Society const& society ) {
  SWITCH( society ) {
    CASE( european ) {
      // In the below, our goal is to determine whether the euro-
      // pean player on this square is "foreign" or not, and then
      // use the "show foreign moves" game option. However, the
      // definition of what constitutes "foreign" depends on a
      // few different things.
      auto const switch_foreign = [&]( bool const foreign ) {
        if( !foreign ) return true;
        return ss.settings.in_game_options.game_menu_options
            [e_game_menu_option::show_foreign_moves];
      };

      if( viewer.has_value() ) {
        // We are viewing from one player's perspective, so a
        // foreign power is any other player.
        bool const foreign = european.player != *viewer;
        return switch_foreign( foreign );
      } else {
        // We see the entire map. So in that case we need to
        // check the player control. Since if the player has re-
        // vealed the entire map they can still turn off foreign
        // moves and not see the moves of AI players. Another
        // possibility is that there are no human players in the
        // game and it is the natives' turn. In that case it also
        // seems sensible to go based on control. So we'll just
        // assume that the AI players are the ones that the user
        // would consider "foreign".
        UNWRAP_CHECK_T( Player const& european_player,
                        ss.players.players[european.player] );
        bool const foreign =
            european_player.control == e_player_control::ai;
        return switch_foreign( foreign );
      }
    }
    CASE( native ) {
      return ss.settings.in_game_options.game_menu_options
          [e_game_menu_option::show_indian_moves];
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
