/****************************************************************
**anim-builder.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-01-28.
*
* Description: Representation for animation sequences in land
*              view.
*
*****************************************************************/
#include "anim-builder.hpp"

// Revolution Now
#include "roles.hpp"
#include "tribe-mgr.hpp"
#include "unit-mgr.hpp"
#include "visibility.hpp"

// ss
#include "ss/colony.rds.hpp"
#include "ss/ref.hpp"
#include "ss/terrain.hpp"
#include "ss/units.hpp"

// rds
#include "rds/switch-macro.hpp"

using namespace std;

namespace rn {

namespace {

using P = ::rn::AnimationPrimitive;

using ::gfx::point;

} // namespace

/****************************************************************
** AnimationBuilder
*****************************************************************/
AnimationBuilder::AnimationBuilder() { new_phase(); }

void AnimationBuilder::clear() {
  // We should do it this way so that we get initialized with a
  // new value using the constructor to preserve invariants.
  *this = {};
}

AnimationSequence const& AnimationBuilder::result() const& {
  return seq_;
}

AnimationSequence AnimationBuilder::result() && {
  // This should be NRVO'd.
  AnimationSequence res = std::move( seq_ );
  // If we don't call clear here then we won't preserve invari-
  // ants, namely that we always have at least one phase.
  clear();
  return res;
}

void AnimationBuilder::new_phase() {
  if( seq_.sequence.size() == 1 && seq_.sequence[0].empty() )
    // This allows us to either include or emit the
    // builder.new_phase call for the first phase.
    return;
  seq_.sequence.emplace_back();
}

AnimationAction& AnimationBuilder::delay(
    chrono::microseconds duration ) {
  return push( P::delay{ .duration = duration } );
}

AnimationAction& AnimationBuilder::play_sound( e_sfx what ) {
  return push( P::play_sound{ .what = what } );
}

AnimationAction& AnimationBuilder::hide_unit(
    GenericUnitId unit_id ) {
  return push( P::hide_unit{ .unit_id = unit_id } );
}

AnimationAction& AnimationBuilder::front_unit(
    GenericUnitId unit_id ) {
  return push( P::front_unit{ .unit_id = unit_id } );
}

AnimationAction& AnimationBuilder::translocate_unit(
    GenericUnitId unit_id, e_direction const direction ) {
  return push( P::translocate_unit{ .unit_id   = unit_id,
                                    .direction = direction } );
}

AnimationAction& AnimationBuilder::slide_unit(
    GenericUnitId unit_id, e_direction direction ) {
  return push( P::slide_unit{ .unit_id   = unit_id,
                              .direction = direction } );
}

AnimationAction& AnimationBuilder::talk_unit(
    GenericUnitId unit_id, e_direction direction ) {
  return push( P::talk_unit{ .unit_id   = unit_id,
                             .direction = direction } );
}

AnimationAction& AnimationBuilder::depixelate_euro_unit(
    UnitId unit_id ) {
  return push( P::depixelate_euro_unit{ .unit_id = unit_id } );
}

AnimationAction& AnimationBuilder::depixelate_native_unit(
    NativeUnitId unit_id ) {
  return push( P::depixelate_native_unit{ .unit_id = unit_id } );
}

AnimationAction& AnimationBuilder::enpixelate_unit(
    GenericUnitId unit_id ) {
  return push( P::enpixelate_unit{ .unit_id = unit_id } );
}

AnimationAction& AnimationBuilder::pixelate_euro_unit_to_target(
    UnitId unit_id, e_unit_type target ) {
  return push( P::pixelate_euro_unit_to_target{
    .unit_id = unit_id, .target = target } );
}

AnimationAction&
AnimationBuilder::pixelate_native_unit_to_target(
    NativeUnitId unit_id, e_native_unit_type target ) {
  return push( P::pixelate_native_unit_to_target{
    .unit_id = unit_id, .target = target } );
}

AnimationAction& AnimationBuilder::depixelate_colony(
    Coord tile ) {
  return push( P::depixelate_colony{ .tile = tile } );
}

AnimationAction& AnimationBuilder::depixelate_dwelling(
    Coord tile ) {
  return push( P::depixelate_dwelling{ .tile = tile } );
}

AnimationAction& AnimationBuilder::ensure_tile_visible(
    Coord tile ) {
  return push( P::ensure_tile_visible{ .tile = tile } );
}

// We can have at most one of each type of landscape anim per
// phase, so reuse one if we already have it, otherwise create
// one. As an optimization, try from the back first since that is
// where it is likely to be, since presumably we will be adding
// these in bulk.
template<typename Primitive>
AnimationAction& AnimationBuilder::find_or_add_action() {
  CHECK( !seq_.sequence.empty() );
  auto& latest_seq = seq_.sequence.back();
  for( auto it = latest_seq.rbegin(); it != latest_seq.rend();
       ++it ) {
    AnimationAction& action = *it;
    if( auto mod = action.primitive.get_if<Primitive>();
        mod.has_value() )
      return action;
  }
  return push( Primitive{} );
}

template<typename Primitive>
AnimationAction& AnimationBuilder::landview_anim_set_override(
    Coord tile, MapSquare const& initial, MapSquareEditFn op ) {
  AnimationAction& action = find_or_add_action<Primitive>();
  UNWRAP_CHECK( overrides,
                action.primitive.inner_if<Primitive>() );
  if( !overrides.squares.contains( tile ) )
    overrides.squares[tile] = initial;
  op( overrides.squares[tile] );
  return action;
}

template<typename Primitive>
AnimationAction&
AnimationBuilder::landview_anim_override_context(
    Coord tile, maybe<Dwelling const&> override ) {
  AnimationAction& action = find_or_add_action<Primitive>();
  UNWRAP_CHECK( overrides,
                action.primitive.inner_if<Primitive>() );
  overrides.dwellings[tile] = override;
  return action;
}

AnimationAction& AnimationBuilder::landview_replace_set_tile(
    Coord tile, MapSquare const& square ) {
  using Primitive = AnimationPrimitive::landscape_anim_replace;
  return landview_anim_set_override<Primitive>(
      tile, MapSquare{},
      [&]( MapSquare& target ) { target = square; } );
}

AnimationAction& AnimationBuilder::landview_enpixelate_edit_tile(
    Coord tile, MapSquare const& initial, MapSquareEditFn op ) {
  using Primitive =
      AnimationPrimitive::landscape_anim_enpixelate;
  return landview_anim_set_override<Primitive>( tile, initial,
                                                op );
}

AnimationAction&
AnimationBuilder::landview_enpixelate_dwelling_context(
    Coord tile, maybe<Dwelling const&> target ) {
  using Primitive =
      AnimationPrimitive::landscape_anim_enpixelate;
  return landview_anim_override_context<Primitive>( tile,
                                                    target );
}

AnimationAction& AnimationBuilder::hide_colony( Coord tile ) {
  return push( P::hide_colony{ tile } );
}

AnimationAction& AnimationBuilder::hide_dwelling( Coord tile ) {
  return push( P::hide_dwelling{ tile } );
}

/****************************************************************
** Public API
*****************************************************************/
AnimationContents animated_contents(
    SSConst const& ss, AnimationSequence const& seq ) {
  AnimationContents res;
  unordered_map<point, vector<Society>> tiles;
  auto const p_viz = create_visibility_for(
      ss, player_for_role( ss, e_player_role::viewer ) );
  auto const& viz = *p_viz;

  auto const add = mp::overload{
    [&]( point const tile ) {
      tiles[tile];
      auto const colony = viz.colony_at( tile );
      if( colony.has_value() )
        tiles[tile].push_back(
            Society::european{ .player = colony->player } );
      auto const dwelling = viz.dwelling_at( tile );
      if( dwelling.has_value() )
        tiles[tile].push_back( Society::native{
          .tribe = tribe_type_for_dwelling( ss, *dwelling ) } );
    },
    [&]( this auto&& self, point const tile,
         UnitId const unit_id ) {
      self( tile );
      tiles[tile].push_back( Society::european{
        .player = ss.units.unit_for( unit_id ).player_type() } );
    },
    [&]( this auto&& self, point const tile,
         NativeUnitId const unit_id ) {
      self( tile );
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
      UNWRAP_CHECK_T(
          point const src,
          coord_for_unit_multi_ownership( ss, generic_id ) );
      point const dst = src.moved( direction );
      self( src, generic_id );
      self( dst, generic_id );
    },
  };

  for( auto const& phase : seq.sequence ) {
    for( auto const& action : phase ) {
      auto const& primitive = action.primitive;
      SWITCH( primitive ) {
        CASE( delay ) { break; }
        CASE( depixelate_colony ) {
          add( depixelate_colony.tile );
          break;
        }
        CASE( depixelate_dwelling ) {
          add( depixelate_dwelling.tile );
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
          // This one is slightly questionable as to whether we
          // need to add this here... perhaps time will tell.
          add( ensure_tile_visible.tile );
          break;
        }
        CASE( front_unit ) {
          add( front_unit.unit_id );
          break;
        }
        CASE( translocate_unit ) {
          add( translocate_unit.unit_id,
               translocate_unit.direction );
          break;
        }
        CASE( hide_colony ) {
          add( hide_colony.tile );
          break;
        }
        CASE( hide_dwelling ) {
          add( hide_dwelling.tile );
          break;
        }
        CASE( hide_unit ) {
          add( hide_unit.unit_id );
          break;
        }
        CASE( landscape_anim_enpixelate ) {
          // TBD as to whether we need to include this.
          break;
        }
        CASE( landscape_anim_replace ) {
          // TBD as to whether we need to include this.
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

  for( auto& [tile, societies] : tiles )
    res.tiles.push_back( AnimatedTile{
      .tile = tile, .inhabitants = std::move( societies ) } );

  sort( res.tiles.begin(), res.tiles.end(),
        []( AnimatedTile const& l, AnimatedTile const& r ) {
          if( l.tile.y != r.tile.y ) return l.tile.y < r.tile.y;
          return l.tile.x < r.tile.x;
        } );
  return res;
}

} // namespace rn
