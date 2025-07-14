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
#include "tribe-mgr.hpp"
#include "unit-mgr.hpp"
#include "visibility.hpp"

// ss
#include "ss/players.rds.hpp"
#include "ss/ref.hpp"
#include "ss/settings.rds.hpp"
#include "ss/terrain.hpp"
#include "ss/units.hpp"

// rds
#include "rds/switch-macro.hpp"

// C++ standard library
#include <unordered_map>

using namespace std;

namespace rn {

namespace {

using ::gfx::e_direction;
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

bool should_animate_tile( SSConst const& ss, gfx::point tile ) {
  vector<Society> societies;
  auto const society = society_on_square( ss, tile );
  if( society.has_value() ) societies.push_back( *society );
  return should_animate_tile(
      ss,
      AnimatedTile{ .tile        = tile,
                    .inhabitants = std::move( societies ) } );
}

bool should_animate_seq( SSConst const& ss,
                         AnimationSequence const& seq ) {
  unordered_map<point, vector<Society>> tiles;
  auto const p_viz = create_visibility_for(
      ss, player_for_role( ss, e_player_role::viewer ) );
  auto const& viz = *p_viz;

  auto const add = mp::overload{
    [&]( point const tile ) { tiles[tile]; },
    [&]( point const tile, UnitId const unit_id ) {
      tiles[tile];
      tiles[tile].push_back( Society::european{
        .player = ss.units.unit_for( unit_id ).player_type() } );
    },
    [&]( point const tile, NativeUnitId const unit_id ) {
      tiles[tile];
      tiles[tile].push_back( Society::native{
        .tribe = tribe_type_for_unit(
            ss, ss.units.native_unit_for( unit_id ) ) } );
    },
    [&]( this auto&& self, point const tile,
         GenericUnitId const generic_id ) {
      e_unit_kind const kind = ss.units.unit_kind( generic_id );
      switch( kind ) {
        case e_unit_kind::euro: {
          self( tile,
                ss.units.euro_unit_for( generic_id ).id() );
          break;
        }
        case e_unit_kind::native: {
          self( tile,
                ss.units.native_unit_for( generic_id ).id );
          break;
        }
      }
    },
    [&]( this auto&& self, GenericUnitId const generic_id ) {
      auto const coord =
          coord_for_unit_multi_ownership( ss, generic_id );
      if( coord.has_value() ) self( *coord, generic_id );
    },
    [&]( this auto&& self, GenericUnitId const generic_id,
         e_direction const direction ) {
      auto const src =
          coord_for_unit_multi_ownership( ss, generic_id );
      if( !src.has_value() ) return;
      point const dst = src->moved( direction );
      self( *src, generic_id );
      self( dst, generic_id );
    },
  };

  auto const add_colony = [&]( point const tile ) {
    tiles[tile];
    auto const colony = viz.colony_at( tile );
    if( colony.has_value() )
      tiles[tile].push_back(
          Society::european{ .player = colony->player } );
  };

  auto const add_dwelling = [&]( point const tile ) {
    tiles[tile];
    auto const dwelling = viz.dwelling_at( tile );
    if( dwelling.has_value() )
      tiles[tile].push_back( Society::native{
        .tribe = tribe_type_for_dwelling( ss, *dwelling ) } );
  };

  for( auto const& phase : seq.sequence ) {
    for( auto const& action : phase ) {
      auto const& primitive = action.primitive;
      SWITCH( primitive ) {
        CASE( delay ) { break; }
        CASE( depixelate_colony ) {
          add_colony( depixelate_colony.tile );
          break;
        }
        CASE( depixelate_dwelling ) {
          add_dwelling( depixelate_dwelling.tile );
          break;
        }
        CASE( depixelate_euro_unit ) {
          add( depixelate_euro_unit.unit_id );
          break;
        }
        CASE( depixelate_native_unit ) {
          add( depixelate_native_unit.unit_id );
          break;
        }
        CASE( enpixelate_unit ) {
          add( enpixelate_unit.unit_id );
          break;
        }
        CASE( ensure_tile_visible ) {
          add( ensure_tile_visible.tile );
          break;
        }
        CASE( front_unit ) {
          add( front_unit.unit_id );
          break;
        }
        CASE( hide_colony ) {
          add_colony( hide_colony.tile );
          break;
        }
        CASE( hide_dwelling ) {
          add_dwelling( hide_dwelling.tile );
          break;
        }
        CASE( hide_unit ) {
          add( hide_unit.unit_id );
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
          add( pixelate_euro_unit_to_target.unit_id );
          break;
        }
        CASE( pixelate_native_unit_to_target ) {
          add( pixelate_native_unit_to_target.unit_id );
          break;
        }
        CASE( play_sound ) { break; }
        CASE( slide_unit ) {
          // NOTE: The destination square may potentially not
          // exist here if a unit is sliding off of the map
          // (which can happen when sailing the high seas). But
          // either way, the subsequent should remove those.
          add( slide_unit.unit_id, slide_unit.direction );
          break;
        }
        CASE( talk_unit ) {
          add( talk_unit.unit_id, talk_unit.direction );
          break;
        }
      }
    }
  }

  // Handles a ship sailing off the map for the high seas.
  erase_if( tiles, [&]( auto const& p ) {
    auto const& [tile, societies] = p;
    return !ss.terrain.square_exists( tile );
  } );

  for( auto& [tile, societies] : tiles ) {
    AnimatedTile const anim_tile{
      .tile = tile, .inhabitants = std::move( societies ) };
    if( should_animate_tile( ss, anim_tile ) ) //
      return true;
  }

  return false;
}

} // namespace rn
