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
#include "anim-builder.rds.hpp"
#include "co-combinator.hpp"
#include "co-wait.hpp"
#include "sound.hpp"
#include "throttler.hpp"
#include "unit-mgr.hpp"
#include "viewport.hpp"

// config
#include "config/natives.hpp"
#include "config/rn.rds.hpp"
#include "config/unit-type.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/colony.rds.hpp"
#include "ss/dwelling.rds.hpp"
#include "ss/natives.hpp"
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
maybe<UnitAnimationState_t const&>
LandViewAnimator::unit_animation( UnitId id ) const {
  auto it = unit_animations_.find( id );
  if( it == unit_animations_.end() ) return nothing;
  stack<UnitAnimationState_t> const& st = it->second;
  CHECK( !st.empty() );
  return st.top();
}

maybe<ColonyAnimationState_t const&>
LandViewAnimator::colony_animation( ColonyId id ) const {
  auto it = colony_animations_.find( id );
  if( it == colony_animations_.end() ) return nothing;
  stack<ColonyAnimationState_t> const& st = it->second;
  CHECK( !st.empty() );
  return st.top();
}

maybe<DwellingAnimationState_t const&>
LandViewAnimator::dwelling_animation( DwellingId id ) const {
  auto it = dwelling_animations_.find( id );
  if( it == dwelling_animations_.end() ) return nothing;
  stack<DwellingAnimationState_t> const& st = it->second;
  CHECK( !st.empty() );
  return st.top();
}

wait<> LandViewAnimator::unit_depixelation_throttler(
    GenericUnitId id, maybe<e_tile> target_tile ) {
  auto popper =
      add_unit_animation<UnitAnimationState::depixelate_unit>(
          id );
  UnitAnimationState::depixelate_unit& depixelate = popper.get();
  depixelate.stage                                = 0.0;
  depixelate.target                               = target_tile;

  AnimThrottler throttle( kAlmostStandardFrame );
  while( depixelate.stage < 1.0 ) {
    co_await throttle();
    depixelate.stage += config_rn.depixelate_per_frame;
  }
  // Need this so that final frame is visible.
  co_await throttle();
}

wait<> LandViewAnimator::unit_enpixelation_throttler(
    GenericUnitId id ) {
  auto popper =
      add_unit_animation<UnitAnimationState::enpixelate_unit>(
          id );
  UnitAnimationState::enpixelate_unit& enpixelate = popper.get();
  enpixelate.stage                                = 1.0;

  AnimThrottler throttle( kAlmostStandardFrame );
  while( enpixelate.stage > 0.0 ) {
    co_await throttle();
    enpixelate.stage -= config_rn.depixelate_per_frame;
  }
  // Need this so that final frame is visible.
  co_await throttle();
}

wait<> LandViewAnimator::colony_depixelation_throttler(
    Colony const& colony ) {
  auto popper =
      add_colony_animation<ColonyAnimationState::depixelate>(
          colony.id );
  ColonyAnimationState::depixelate& depixelate = popper.get();
  depixelate.stage                             = 0.0;

  AnimThrottler throttle( kAlmostStandardFrame );
  while( depixelate.stage < 1.0 ) {
    co_await throttle();
    depixelate.stage += config_rn.depixelate_per_frame;
  }
  // Need this so that final frame is visible.
  co_await throttle();
}

wait<> LandViewAnimator::dwelling_depixelation_throttler(
    Dwelling const& dwelling ) {
  auto popper =
      add_dwelling_animation<DwellingAnimationState::depixelate>(
          dwelling.id );
  DwellingAnimationState::depixelate& depixelate = popper.get();
  depixelate.stage                               = 0.0;

  AnimThrottler throttle( kAlmostStandardFrame );
  while( depixelate.stage < 1.0 ) {
    co_await throttle();
    depixelate.stage += config_rn.depixelate_per_frame;
  }
  // Need this so that final frame is visible.
  co_await throttle();
}

// TODO: this animation needs to be sync'd with the one in the
// mini-map.
wait<> LandViewAnimator::animate_blink(
    UnitId id, bool visible_initially ) {
  using namespace std::literals::chrono_literals;
  auto popper =
      add_unit_animation<UnitAnimationState::blink>( id );
  UnitAnimationState::blink& blink = popper.get();
  blink.visible                    = visible_initially;
  // We use an initial delay so that our initial value of `vis-
  // ible` will be the first to linger.
  AnimThrottler throttle( 500ms, /*initial_delay=*/true );
  while( true ) {
    co_await throttle();
    blink.visible = !blink.visible;
  }
}

wait<> LandViewAnimator::slide_throttler( GenericUnitId id,
                                          e_direction   d ) {
  // TODO: make this a game option.
  double const kMaxVelocity =
      ss_.settings.fast_piece_slide ? .1 : .07;

  auto popper =
      add_unit_animation<UnitAnimationState::slide>( id );
  UnitAnimationState::slide& slide = popper.get();

  slide = UnitAnimationState::slide{
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
  while( slide.percent < 1.0 ) {
    co_await throttle();
    slide.percent_vel.advance( e_push_direction::none );
    slide.percent += slide.percent_vel.to_double();
  }
  // Need this so that final frame is visible.
  co_await throttle();
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

// In this function we can assume that the `primitive` argument
// will outlive this coroutine.
wait<> LandViewAnimator::animate_primitive(
    AnimationPrimitive_t const& primitive ) {
  switch( primitive.to_enum() ) {
    using namespace AnimationPrimitive;
    case e::play_sound: {
      auto& [what] = primitive.get<play_sound>();
      // TODO: should we co_await on the length of the sound ef-
      // fect? Maybe that would be a separate animation such as
      // `play_sound_wait`.
      play_sound_effect( what );
      break;
    }
    case e::hide_unit: {
      auto& [unit_id] = primitive.get<hide_unit>();
      auto popper = add_unit_animation<UnitAnimationState::hide>(
          unit_id );
      // Never resumes.
      co_await wait_promise<>().wait();
      SHOULD_NOT_BE_HERE;
    }
    case e::front_unit: {
      auto& [unit_id] = primitive.get<front_unit>();
      auto popper =
          add_unit_animation<UnitAnimationState::front>(
              unit_id );
      // Never resumes.
      co_await wait_promise<>().wait();
      SHOULD_NOT_BE_HERE;
    }
    case e::slide_unit: {
      auto& [unit_id, direction] = primitive.get<slide_unit>();
      // Ensure that both src and dst squares are visible.
      Coord const src =
          coord_for_unit_indirect_or_die( ss_.units, unit_id );
      Coord const dst = src.moved( direction );
      co_await ensure_visible( src );
      // The destination square may not exist if it is a ship
      // sailing the high seas by moving off of the map edge
      // (which the original game allows).
      if( ss_.terrain.square_exists( dst ) )
        co_await ensure_visible( dst );
      co_await slide_throttler( unit_id, direction );
      break;
    }
    case e::depixelate_unit: {
      auto& [unit_id] = primitive.get<depixelate_unit>();
      co_await ensure_visible_unit( unit_id );
      co_await unit_depixelation_throttler( unit_id,
                                            /*target=*/nothing );
      break;
    }
    case e::enpixelate_unit: {
      auto& [unit_id] = primitive.get<enpixelate_unit>();
      co_await ensure_visible_unit( unit_id );
      co_await unit_enpixelation_throttler( unit_id );
      break;
    }
    case e::depixelate_euro_unit_to_target: {
      auto& [unit_id, target] =
          primitive.get<depixelate_euro_unit_to_target>();
      co_await ensure_visible_unit( unit_id );
      co_await unit_depixelation_throttler(
          unit_id, unit_attr( target ).tile );
      break;
    }
    case e::depixelate_native_unit_to_target: {
      auto& [unit_id, target] =
          primitive.get<depixelate_native_unit_to_target>();
      co_await ensure_visible_unit( unit_id );
      co_await unit_depixelation_throttler(
          unit_id, unit_attr( target ).tile );
      break;
    }
    case e::depixelate_colony: {
      auto& [colony_id] = primitive.get<depixelate_colony>();
      Colony const& colony =
          ss_.colonies.colony_for( colony_id );
      co_await ensure_visible( colony.location );
      co_await colony_depixelation_throttler( colony );
      break;
    }
    case e::depixelate_dwelling: {
      auto& [dwelling_id] = primitive.get<depixelate_dwelling>();
      Dwelling const& dwelling =
          ss_.natives.dwelling_for( dwelling_id );
      Coord const location =
          ss_.natives.coord_for( dwelling_id );
      co_await ensure_visible( location );
      co_await dwelling_depixelation_throttler( dwelling );
      break;
    }
  }
}

wait<> LandViewAnimator::animate_sequence(
    AnimationSequence const& seq ) {
  for( vector<AnimationAction> const& sub_seq : seq.sequence ) {
    vector<wait<>> must_complete;
    vector<wait<>> background;
    must_complete.reserve( sub_seq.size() );
    background.reserve( sub_seq.size() );
    for( AnimationAction const& action : sub_seq ) {
      if( action.background )
        background.push_back(
            animate_primitive( action.primitive ) );
      else
        must_complete.push_back(
            animate_primitive( action.primitive ) );
    }
    co_await co::all( std::move( must_complete ) );
  }
}

} // namespace rn
