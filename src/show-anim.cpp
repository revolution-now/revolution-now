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
#include "anim-builder.rds.hpp"
#include "roles.hpp"
#include "society.hpp"
#include "unit-mgr.hpp"
#include "visibility.hpp"

// ss
#include "ss/players.rds.hpp"
#include "ss/ref.hpp"
#include "ss/settings.rds.hpp"
#include "ss/terrain.hpp"

// rds
#include "rds/switch-macro.hpp"

// C++ standard library
#include <unordered_set>

using namespace std;

namespace rn {

namespace {

using ::gfx::e_direction;
using ::gfx::point;

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
bool should_animate_tile( SSConst const& ss, point const tile ) {
  CHECK( ss.terrain.square_exists( tile ) );
  auto const viz_viewer = create_visibility_for(
      ss, player_for_role( ss, e_player_role::viewer ) );
  bool const is_clear =
      viz_viewer->visible( tile ) == e_tile_visibility::clear;
  if( !is_clear ) return false;
  auto const society = society_on_square( ss, tile );
  if( !society.has_value() )
    // Not sure what this would be, but at this point the tile is
    // clear so no reason not to show whatever animation this
    // would be.
    return true;
  SWITCH( *society ) {
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

      if( viz_viewer->player().has_value() ) {
        // We are viewing from one player's perspective, so a
        // foreign power is any other player.
        bool const foreign =
            european.player != *viz_viewer->player();
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
  return true;
}

bool should_animate_seq( SSConst const& ss,
                         AnimationSequence const& seq ) {
  unordered_set<point> tiles;
  for( auto const& phase : seq.sequence ) {
    for( auto const& action : phase ) {
      auto const& primitive = action.primitive;
      SWITCH( primitive ) {
        CASE( delay ) { break; }
        CASE( depixelate_colony ) {
          tiles.insert( depixelate_colony.tile );
          break;
        }
        CASE( depixelate_dwelling ) {
          tiles.insert( depixelate_dwelling.tile );
          break;
        }
        CASE( depixelate_euro_unit ) {
          auto const coord = coord_for_unit_multi_ownership(
              ss, depixelate_euro_unit.unit_id );
          if( coord.has_value() ) tiles.insert( *coord );
          break;
        }
        CASE( depixelate_native_unit ) {
          auto const coord = coord_for_unit_indirect(
              ss.units, depixelate_native_unit.unit_id );
          if( coord.has_value() ) tiles.insert( *coord );
          break;
        }
        CASE( enpixelate_unit ) {
          auto const coord = coord_for_unit_indirect(
              ss.units, enpixelate_unit.unit_id );
          if( coord.has_value() ) tiles.insert( *coord );
          break;
        }
        CASE( ensure_tile_visible ) {
          tiles.insert( ensure_tile_visible.tile );
          break;
        }
        CASE( front_unit ) {
          auto const coord = coord_for_unit_multi_ownership(
              ss, front_unit.unit_id );
          if( coord.has_value() ) tiles.insert( *coord );
          break;
        }
        CASE( hide_colony ) {
          tiles.insert( hide_colony.tile );
          break;
        }
        CASE( hide_dwelling ) {
          tiles.insert( hide_dwelling.tile );
          break;
        }
        CASE( hide_unit ) {
          auto const coord = coord_for_unit_indirect(
              ss.units, hide_unit.unit_id );
          if( coord.has_value() ) tiles.insert( *coord );
          break;
        }
        CASE( landscape_anim_enpixelate ) {
          // TODO: not sure if we should include this.
          break;
        }
        CASE( landscape_anim_replace ) {
          // TODO: not sure if we should include this.
          break;
        }
        CASE( pixelate_euro_unit_to_target ) {
          auto const coord = coord_for_unit_indirect(
              ss.units, pixelate_euro_unit_to_target.unit_id );
          if( coord.has_value() ) tiles.insert( *coord );
          break;
        }
        CASE( pixelate_native_unit_to_target ) {
          auto const coord = coord_for_unit_indirect(
              ss.units, pixelate_native_unit_to_target.unit_id );
          if( coord.has_value() ) tiles.insert( *coord );
          break;
        }
        CASE( play_sound ) { break; }
        CASE( slide_unit ) {
          point const p = coord_for_unit_indirect_or_die(
              ss.units, slide_unit.unit_id );
          point const dst = p.moved( slide_unit.direction );
          // NOTE: dst may potentially not exist here if a unit
          // is sliding off of the map (not yet sure if that will
          // be allowed to happen). But either way, the subse-
          // quent should remove those.
          tiles.insert( p );
          tiles.insert( dst );
          break;
        }
        CASE( talk_unit ) {
          point const p = coord_for_unit_indirect_or_die(
              ss.units, talk_unit.unit_id );
          point const dst = p.moved( talk_unit.direction );
          tiles.insert( p );
          tiles.insert( dst );
          break;
        }
      }
    }
  }

  erase_if( tiles, [&]( point const p ) {
    return !ss.terrain.square_exists( p );
  } );

  for( point const tile : tiles )
    if( should_animate_tile( ss, tile ) ) //
      return true;

  return false;
}

} // namespace rn
