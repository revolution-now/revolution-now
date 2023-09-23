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
#include "co-time.hpp"
#include "co-wait.hpp"
#include "sound.hpp"
#include "throttler.hpp"
#include "unit-mgr.hpp"
#include "viewport.hpp"
#include "visibility.hpp"

// config
#include "config/gfx.rds.hpp"
#include "config/natives.hpp"
#include "config/unit-type.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/colony.rds.hpp"
#include "ss/dwelling.rds.hpp"
#include "ss/natives.hpp"
#include "ss/settings.rds.hpp"
#include "ss/terrain.hpp"
#include "ss/units.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/keyval.hpp"
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

namespace {

// During a pixelation animation (such as e.g. when a unit gets
// defeated in battle) this curve controls the rate at which
// pixels are removed (or added) with time. The "linear" method
// removes them at a constant rate, and the "log" method removes
// them more slowly as the animation progresses, which can make
// it look more "natural.".
//
// The linear method is simplest, in that it would lead to a con-
// stant number of pixels being depixelated per frame. That may
// sound like what we want, but visually it appears to the eye to
// speed up towards the end some how. In order to counter that, a
// logarithmically decreasing rate seems to look right.
//
// Linear: just increase stage linearly with time.
//
// Log: As as time increases, the delta by which we increase the
// stage with each frame diminishes; it follows the below loga-
// rithmic curve. In particular, each time the stage's distance
// to 1.0 halves, we decrease the delta by a fixed amount. That
// looks like the following for an initial_delta of .01:
//
//   .010 |--------
//   .009 |                     -
// d .008 |                               -
// e .007 |                                      -
// l .006 |                                           -
// t .005 |                                              -
// a .004 |                                                -
//   .003 |                                                 --
//   .002 |
//   .001 |
//      0 +---------------------------------------------------
//        0                .5            .75    .87  .93.97.99
//                               stage
//
// where we've also set a minimum so that it finishes. Note that
// this curve can be approximated as a piece-wise step curve as
// follows (this always works well):
//
//   if( stage > .99 ) return initial_delta * .3;
//   if( stage > .98 ) return initial_delta * .4;
//   if( stage > .96 ) return initial_delta * .5;
//   if( stage > .93 ) return initial_delta * .6;
//   if( stage > .87 ) return initial_delta * .7;
//   if( stage > .75 ) return initial_delta * .8;
//   if( stage > .5 )  return initial_delta * .9;
//   return initial_delta;
//
// TODO: move this into a dedicated pixelation module and unit
// test it.
double depixelation_delta_from_stage( double initial_delta,
                                      double stage ) {
  switch( config_gfx.pixelation_curve.curve_type ) {
    case e_pixelation_curve::log: {
      double const min   = initial_delta * .3;
      double const max   = initial_delta;
      double const magic = initial_delta * .106;
      return std::max( magic * log2( 1 - stage ) + max, min );
    }
    case e_pixelation_curve::linear:
      return initial_delta;
  }
}

// TODO: move this into a dedicated pixelation module and unit
// test it.
wait<> pixelation_stage_throttler( double& stage,
                                   bool    negative = false ) {
  double const        sign = negative ? -1.0 : 1.0;
  static double const pixelation_per_frame =
      config_gfx.pixelation_curve.pixelation_per_frame
          [config_gfx.pixelation_curve.curve_type];
  auto not_finished = [&] {
    return negative ? ( stage > 0.0 ) : ( stage < 1.0 );
  };

  AnimThrottler throttle( kAlmostStandardFrame );
  while( not_finished() ) {
    co_await throttle();
    stage += sign * depixelation_delta_from_stage(
                        pixelation_per_frame, stage );
  }
  // Need this so that final frame is visible.
  co_await throttle();
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
maybe<UnitAnimationState const&>
LandViewAnimator::unit_animation( UnitId id ) const {
  auto it = unit_animations_.find( id );
  if( it == unit_animations_.end() ) return nothing;
  stack<UnitAnimationState> const& st = it->second;
  CHECK( !st.empty() );
  return st.top();
}

maybe<ColonyAnimationState const&>
LandViewAnimator::colony_animation( ColonyId id ) const {
  auto it = colony_animations_.find( id );
  if( it == colony_animations_.end() ) return nothing;
  stack<ColonyAnimationState> const& st = it->second;
  CHECK( !st.empty() );
  return st.top();
}

maybe<DwellingAnimationState const&>
LandViewAnimator::dwelling_animation( DwellingId id ) const {
  auto it = dwelling_animations_.find( id );
  if( it == dwelling_animations_.end() ) return nothing;
  stack<DwellingAnimationState> const& st = it->second;
  CHECK( !st.empty() );
  return st.top();
}

wait<> LandViewAnimator::unit_depixelation_throttler(
    GenericUnitId id, maybe<e_tile> target_tile ) {
  auto popper =
      add_unit_animation<UnitAnimationState::depixelate_unit>(
          id );
  UnitAnimationState::depixelate_unit& depixelate = popper.get();

  depixelate.stage  = 0.0;
  depixelate.target = target_tile;
  co_await pixelation_stage_throttler( depixelate.stage );
}

wait<> LandViewAnimator::unit_enpixelation_throttler(
    GenericUnitId id ) {
  auto popper =
      add_unit_animation<UnitAnimationState::enpixelate_unit>(
          id );
  UnitAnimationState::enpixelate_unit& enpixelate = popper.get();

  enpixelate.stage = 1.0;
  co_await pixelation_stage_throttler( enpixelate.stage,
                                       /*negative=*/true );
}

wait<> LandViewAnimator::colony_depixelation_throttler(
    Colony const& colony ) {
  auto popper =
      add_colony_animation<ColonyAnimationState::depixelate>(
          colony.id );
  ColonyAnimationState::depixelate& depixelate = popper.get();

  depixelate.stage = 0.0;
  co_await pixelation_stage_throttler( depixelate.stage );
}

wait<> LandViewAnimator::dwelling_depixelation_throttler(
    Dwelling const& dwelling ) {
  auto popper =
      add_dwelling_animation<DwellingAnimationState::depixelate>(
          dwelling.id );
  DwellingAnimationState::depixelate& depixelate = popper.get();

  depixelate.stage = 0.0;
  co_await pixelation_stage_throttler( depixelate.stage );
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
  double const kMaxVelocity =
      ss_.settings.game_options
              .flags[e_game_flag_option::fast_piece_slide]
          ? .1
          : .07;

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
    // Need to prevent overshooting which can visually manifest.
    slide.percent = std::min( slide.percent, 1.0 );
    CHECK_LE( slide.percent, 1.0 );
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
  UNWRAP_CHECK_MSG(
      coord, coord_for_unit_multi_ownership( ss_, id ),
      "cannot obtain map coordinate for unit ID {}: {}", id,
      ss_.units.euro_unit_for( id ).refl() );
  co_await ensure_visible( coord );
}

// In this function we can assume that the `primitive` argument
// will outlive this coroutine.
wait<> LandViewAnimator::animate_action_primitive(
    AnimationAction const& action ) {
  AnimationPrimitive const& primitive = action.primitive;
  switch( primitive.to_enum() ) {
    using e = AnimationPrimitive::e;
    case e::delay: {
      auto& [duration] =
          primitive.get<AnimationPrimitive::delay>();
      co_await duration;
      break;
    }
    case e::play_sound: {
      auto& [what] =
          primitive.get<AnimationPrimitive::play_sound>();
      // TODO: should we co_await on the length of the sound ef-
      // fect? Maybe that would be a separate animation such as
      // `play_sound_wait`.
      play_sound_effect( what );
      break;
    }
    case e::hide_unit: {
      auto& [unit_id] =
          primitive.get<AnimationPrimitive::hide_unit>();
      auto popper = add_unit_animation<UnitAnimationState::hide>(
          unit_id );
      // Never resumes.
      co_await wait_promise<>().wait();
      SHOULD_NOT_BE_HERE;
    }
    case e::front_unit: {
      auto& [unit_id] =
          primitive.get<AnimationPrimitive::front_unit>();
      auto popper =
          add_unit_animation<UnitAnimationState::front>(
              unit_id );
      // Never resumes.
      co_await wait_promise<>().wait();
      SHOULD_NOT_BE_HERE;
    }
    case e::slide_unit: {
      auto& [unit_id, direction] =
          primitive.get<AnimationPrimitive::slide_unit>();
      // Ensure that both src and dst squares are visible.
      Coord const src =
          coord_for_unit_indirect_or_die( ss_.units, unit_id );
      Coord const dst = src.moved( direction );
      // The destination square may not exist if it is a ship
      // sailing the high seas by moving off of the map edge
      // (which the original game allows).
      bool const dst_exists = ss_.terrain.square_exists( dst );
      // Check visibility.
      bool const should_animate =
          dst_exists ? should_animate_move( viz_, src, dst )
                     : should_animate_move( viz_, src, src );
      if( !should_animate ) break;
      co_await ensure_visible( src );
      if( dst_exists ) co_await ensure_visible( dst );
      co_await slide_throttler( unit_id, direction );
      break;
    }
    case e::depixelate_unit: {
      auto& [unit_id] =
          primitive.get<AnimationPrimitive::depixelate_unit>();
      // We need the multi-ownership version if the unit being
      // depixelated is a unit working in a colony that is being
      // attacked.
      Coord const tile =
          coord_for_unit_multi_ownership_or_die( ss_, unit_id );
      // Check visibility. This will avoid both scrolling the map
      // and the animation in the case that the unit in question
      // is on a fogged tile, which can happen e.g. if we are de-
      // pixelating a native unit that is under fog when we de-
      // stroy its dwelling.
      if( viz_.visible( tile ) !=
          e_tile_visibility::visible_and_clear )
        break;
      co_await ensure_visible_unit( unit_id );
      co_await unit_depixelation_throttler( unit_id,
                                            /*target=*/nothing );
      break;
    }
    case e::enpixelate_unit: {
      auto& [unit_id] =
          primitive.get<AnimationPrimitive::enpixelate_unit>();
      co_await ensure_visible_unit( unit_id );
      co_await unit_enpixelation_throttler( unit_id );
      break;
    }
    case e::pixelate_euro_unit_to_target: {
      auto& [unit_id, target] = primitive.get<
          AnimationPrimitive::pixelate_euro_unit_to_target>();
      co_await ensure_visible_unit( unit_id );
      co_await unit_depixelation_throttler(
          unit_id, unit_attr( target ).tile );
      break;
    }
    case e::pixelate_native_unit_to_target: {
      auto& [unit_id, target] = primitive.get<
          AnimationPrimitive::pixelate_native_unit_to_target>();
      co_await ensure_visible_unit( unit_id );
      co_await unit_depixelation_throttler(
          unit_id, unit_attr( target ).tile );
      break;
    }
    case e::depixelate_colony: {
      auto& [colony_id] =
          primitive.get<AnimationPrimitive::depixelate_colony>();
      Colony const& colony =
          ss_.colonies.colony_for( colony_id );
      co_await ensure_visible( colony.location );
      co_await colony_depixelation_throttler( colony );
      break;
    }
    case e::depixelate_dwelling: {
      auto& [dwelling_id] =
          primitive
              .get<AnimationPrimitive::depixelate_dwelling>();
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
      wait<> w = animate_action_primitive( action );
      if( action.background )
        background.push_back( std::move( w ) );
      else
        must_complete.push_back( std::move( w ) );
    }
    co_await co::all( std::move( must_complete ) );
  }
}

} // namespace rn
