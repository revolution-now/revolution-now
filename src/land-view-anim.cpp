/****************************************************************
**land-view-anim.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-01-09.
*
* Description: Implements entity animations in the land view.
*
*****************************************************************/
#include "land-view-anim.hpp"

// Revolution Now
#include "anim.hpp"
#include "co-combinator.hpp"
#include "co-wait.hpp"
#include "sound.hpp"
#include "ustate.hpp"
#include "viewport.hpp"

// config
#include "config/natives.hpp"
#include "config/rn.rds.hpp"
#include "config/unit-type.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/colony.rds.hpp"
#include "ss/settings.rds.hpp"
#include "ss/terrain.hpp"
#include "ss/units.hpp"

// base
#include "base/keyval.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

/****************************************************************
** Public API
*****************************************************************/
maybe<UnitAnimation_t const&> LandViewAnimator::unit_animation(
    UnitId id ) const {
  auto it = unit_animations_.find( id );
  if( it == unit_animations_.end() ) return nothing;
  stack<UnitAnimation_t> const& st = it->second;
  CHECK( !st.empty() );
  return st.top();
}

maybe<ColonyAnimation_t const&>
LandViewAnimator::colony_animation( ColonyId id ) const {
  auto it = colony_animations_.find( id );
  if( it == colony_animations_.end() ) return nothing;
  stack<ColonyAnimation_t> const& st = it->second;
  CHECK( !st.empty() );
  return st.top();
}

maybe<DwellingAnimation_t const&>
LandViewAnimator::dwelling_animation( DwellingId id ) const {
  auto it = dwelling_animations_.find( id );
  if( it == dwelling_animations_.end() ) return nothing;
  stack<DwellingAnimation_t> const& st = it->second;
  CHECK( !st.empty() );
  return st.top();
}

wait<> LandViewAnimator::animate_depixelation(
    GenericUnitId id, maybe<e_tile> target_tile ) {
  auto popper =
      add_unit_animation<UnitAnimation::depixelate_unit>( id );
  UnitAnimation::depixelate_unit& depixelate = popper.get();
  depixelate.stage                           = 0.0;
  depixelate.target                          = target_tile;

  AnimThrottler throttle( kAlmostStandardFrame );
  while( depixelate.stage <= 1.0 ) {
    co_await throttle();
    depixelate.stage += config_rn.depixelate_per_frame;
  }
}

wait<> LandViewAnimator::animate_colony_depixelation(
    Colony const& colony ) {
  auto popper =
      add_colony_animation<ColonyAnimation::depixelate>(
          colony.id );
  ColonyAnimation::depixelate& depixelate = popper.get();
  depixelate.stage                        = 0.0;

  AnimThrottler throttle( kAlmostStandardFrame );
  while( depixelate.stage <= 1.0 ) {
    co_await throttle();
    depixelate.stage += config_rn.depixelate_per_frame;
  }
}

// TODO: this animation needs to be sync'd with the one in the
// mini-map.
wait<> LandViewAnimator::animate_blink(
    UnitId id, bool visible_initially ) {
  using namespace std::literals::chrono_literals;
  auto popper = add_unit_animation<UnitAnimation::blink>( id );
  UnitAnimation::blink& blink = popper.get();
  blink.visible               = visible_initially;
  // We use an initial delay so that our initial value of `vis-
  // ible` will be the first to linger.
  AnimThrottler throttle( 500ms, /*initial_delay=*/true );
  while( true ) {
    co_await throttle();
    blink.visible = !blink.visible;
  }
}

wait<> LandViewAnimator::animate_slide( GenericUnitId id,
                                        e_direction   d ) {
  // TODO: make this a game option.
  double const kMaxVelocity =
      ss_.settings.fast_piece_slide ? .1 : .07;

  auto popper = add_unit_animation<UnitAnimation::slide>( id );
  UnitAnimation::slide& slide = popper.get();

  slide = UnitAnimation::slide{
      .direction   = d,
      .percent     = 0.0,
      .percent_vel = DissipativeVelocity{
          /*min_velocity=*/0,            //
          /*max_velocity=*/kMaxVelocity, //
          /*initial_velocity=*/.1,       //
          /*mag_acceleration=*/1,        //
          /*mag_drag_acceleration=*/.002 //
      }                                  //
  };
  AnimThrottler throttle( kAlmostStandardFrame );
  while( slide.percent <= 1.0 ) {
    co_await throttle();
    slide.percent_vel.advance( e_push_direction::none );
    slide.percent += slide.percent_vel.to_double();
  }
}

wait<> LandViewAnimator::ensure_visible( Coord const& coord ) {
  co_await viewport_.ensure_tile_visible_smooth( coord );
}

wait<> LandViewAnimator::ensure_visible_unit(
    GenericUnitId id ) {
  // Need multi-ownership variant because sometimes the unit in
  // question is a worker in a colony, as can happen if we are
  // attacking an undefended colony.
  UNWRAP_CHECK( coord,
                coord_for_unit_multi_ownership( ss_, id ) );
  co_await ensure_visible( coord );
}

wait<> LandViewAnimator::animate_move( UnitId      id,
                                       e_direction direction ) {
  // Ensure that both src and dst squares are visible.
  Coord src = coord_for_unit_indirect_or_die( ss_.units, id );
  Coord dst = src.moved( direction );
  co_await ensure_visible( src );
  // The destination square may not exist if it is a ship sailing
  // the high seas by moving off of the map edge (which the orig-
  // inal game allows).
  if( ss_.terrain.square_exists( dst ) )
    co_await ensure_visible( dst );
  play_sound_effect( e_sfx::move );
  co_await animate_slide( id, direction );
}

wait<> LandViewAnimator::animate_attack(
    GenericUnitId attacker, GenericUnitId defender,
    vector<UnitWithDepixelateTarget_t> const& animations,
    bool                                      attacker_wins ) {
  co_await ensure_visible_unit( defender );
  co_await ensure_visible_unit( attacker );

  UNWRAP_CHECK( attacker_coord,
                ss_.units.maybe_coord_for( attacker ) );
  UNWRAP_CHECK( defender_coord, coord_for_unit_multi_ownership(
                                    ss_, defender ) );
  UNWRAP_CHECK( d,
                attacker_coord.direction_to( defender_coord ) );

  // Give each unit a baseline animation of "front," that way if
  // the below doesn't end up giving one of them an animation
  // then at least it will have this `front` animation which
  // guarantees that it will be visible.
  auto attacker_front_popper =
      add_unit_animation<UnitAnimation::front>( attacker );
  auto defender_front_popper =
      add_unit_animation<UnitAnimation::front>( defender );

  // While the attacker is sliding we want to make sure the de-
  // fender comes to the front in case there are multiple units
  // and/or a colony on the tile.
  play_sound_effect( e_sfx::move );
  co_await animate_slide( attacker, d );

  play_sound_effect( attacker_wins ? e_sfx::attacker_won
                                   : e_sfx::attacker_lost );
  vector<wait<>> waits;
  for( UnitWithDepixelateTarget_t const& anim : animations ) {
    GenericUnitId const id = rn::visit(
        anim, []( auto& o ) { return GenericUnitId{ o.id }; } );
    maybe<e_tile> const target_tile =
        rn::visit( anim, []( auto& o ) {
          return o.target.fmap( []( auto type ) {
            return unit_attr( type ).tile;
          } );
        } );
    // This starts the animation.
    waits.push_back( animate_depixelation( id, target_tile ) );
  }
  co_await co::all( std::move( waits ) );
}

wait<> LandViewAnimator::animate_colony_destruction(
    Colony const& colony ) {
  co_await ensure_visible( colony.location );
  // TODO: Sound effect?
  co_await animate_colony_depixelation( colony );
}

wait<> LandViewAnimator::animate_unit_depixelation(
    UnitWithDepixelateTarget_t const& what ) {
  GenericUnitId const id = rn::visit(
      what, []( auto& o ) { return GenericUnitId{ o.id }; } );
  co_await ensure_visible_unit( id );
  maybe<e_tile> const target_tile =
      rn::visit( what, []( auto& o ) {
        return o.target.fmap(
            []( auto type ) { return unit_attr( type ).tile; } );
      } );
  co_await animate_depixelation( id, target_tile );
}

// FIXME: Would be nice to make this animation a bit more so-
// phisticated.
wait<> LandViewAnimator::animate_colony_capture(
    UnitId attacker_id, UnitId defender_id,
    vector<UnitWithDepixelateTarget_t> const& animations,
    ColonyId                                  colony_id ) {
  co_await animate_attack( attacker_id, defender_id, animations,
                           /*attacker_wins=*/true );
  UNWRAP_CHECK(
      direction,
      ss_.units.coord_for( attacker_id )
          .direction_to(
              ss_.colonies.colony_for( colony_id ).location ) );
  co_await animate_move( attacker_id, direction );
}
} // namespace rn
