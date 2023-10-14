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
#include "frame-count.hpp"
#include "latch.hpp"
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

// rds
#include "rds/switch-macro.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-chrono.hpp"
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
wait<> pixelation_stage_throttler( co::latch& hold,
                                   double&    stage,
                                   bool negative = false ) {
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
  // Must be last.
  co_await hold.arrive_and_wait();
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
LandViewAnimator::LandViewAnimator(
    SSConst const& ss, SmoothViewport& viewport,
    std::unique_ptr<IVisibility const> const& viz )
  : ss_( ss ), viewport_( viewport ), viz_( viz ) {
  CHECK( viz_ != nullptr );
}

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

maybe<DwellingAnimationState const&>
LandViewAnimator::fog_dwelling_animation( Coord tile ) const {
  auto it = fog_dwelling_animations_.find( tile );
  if( it == fog_dwelling_animations_.end() ) return nothing;
  stack<DwellingAnimationState> const& st = it->second;
  CHECK( !st.empty() );
  return st.top();
}

wait<> LandViewAnimator::unit_depixelation_throttler(
    co::latch& hold, UnitId id,
    maybe<e_unit_type> target_type ) {
  auto popper = add_unit_animation<
      UnitAnimationState::depixelate_euro_unit>( id );
  UnitAnimationState::depixelate_euro_unit& depixelate =
      popper.get();

  depixelate.stage  = 0.0;
  depixelate.target = target_type;
  co_await pixelation_stage_throttler( hold, depixelate.stage );
}

wait<> LandViewAnimator::native_unit_depixelation_throttler(
    co::latch& hold, NativeUnitId id,
    maybe<e_native_unit_type> target_type ) {
  auto popper = add_unit_animation<
      UnitAnimationState::depixelate_native_unit>( id );
  UnitAnimationState::depixelate_native_unit& depixelate =
      popper.get();

  depixelate.stage  = 0.0;
  depixelate.target = target_type;
  co_await pixelation_stage_throttler( hold, depixelate.stage );
}

wait<> LandViewAnimator::unit_enpixelation_throttler(
    co::latch& hold, GenericUnitId id ) {
  auto popper =
      add_unit_animation<UnitAnimationState::enpixelate_unit>(
          id );
  UnitAnimationState::enpixelate_unit& enpixelate = popper.get();

  enpixelate.stage = 1.0;
  co_await pixelation_stage_throttler( hold, enpixelate.stage,
                                       /*negative=*/true );
}

wait<> LandViewAnimator::colony_depixelation_throttler(
    co::latch& hold, Colony const& colony ) {
  auto popper =
      add_colony_animation<ColonyAnimationState::depixelate>(
          colony.id );
  ColonyAnimationState::depixelate& depixelate = popper.get();

  depixelate.stage = 0.0;
  co_await pixelation_stage_throttler( hold, depixelate.stage );
}

wait<> LandViewAnimator::dwelling_depixelation_throttler(
    co::latch& hold, Dwelling const& dwelling ) {
  auto popper =
      add_dwelling_animation<DwellingAnimationState::depixelate>(
          dwelling.id );
  DwellingAnimationState::depixelate& depixelate = popper.get();

  depixelate.stage = 0.0;
  co_await pixelation_stage_throttler( hold, depixelate.stage );
}

wait<> LandViewAnimator::fog_dwelling_depixelation_throttler(
    co::latch& hold, Coord tile ) {
  auto popper = add_fog_dwelling_animation<
      DwellingAnimationState::depixelate>( tile );
  DwellingAnimationState::depixelate& depixelate = popper.get();

  depixelate.stage = 0.0;
  co_await pixelation_stage_throttler( hold, depixelate.stage );
}

wait<> LandViewAnimator::landscape_anim_depixelation_throttler(
    co::latch& hold, map<Coord, MapSquare> const& targets ) {
  CHECK( !landview_anim_buffer_state_.has_value() );
  LandscapeAnimBufferState& state =
      landview_anim_buffer_state_.emplace();
  SCOPE_EXIT { landview_anim_buffer_state_.reset(); };

  // First, get a full set of squares that we'll have to animate;
  // this includes the ones that are changing plus surrounding
  // ones that might change indirectly.
  unordered_map<Coord, FogSquare> fog_squares;
  for( auto const& [tile, _] : targets ) {
    for( e_cdirection const d :
         refl::enum_values<e_cdirection> ) {
      Coord const moved = tile.moved( d );
      if( !ss_.terrain.square_exists( moved ) ) continue;
      switch( viz_->visible( moved ) ) {
        case e_tile_visibility::hidden: {
          // No sense in animating a square that is fully hidden.
          continue;
        }
        case e_tile_visibility::visible_and_clear: {
          // Don't use the fog square here since it may be stale.
          fog_squares[moved].square = viz_->square_at( moved );
          fog_squares[moved].fog_of_war_removed = true;
          break;
        }
        case e_tile_visibility::visible_with_fog: {
          UNWRAP_CHECK( fog_square,
                        viz_->fog_square_at( moved ) );
          fog_squares[moved] = fog_square;
          break;
        }
      }
    }
  }

  // Now inject the target ones that are directly changing.
  for( auto const& [tile, target_square] : targets )
    if( fog_squares.contains( tile ) )
      fog_squares[tile].square = target_square;

  state.needs_rendering = false;
  state.targets         = std::move( fog_squares );
  state.stage           = 1.0;
  {
    // This will cause the renderer to rerender the landscape
    // anim buffer, but only once. This is slightly hacky, but
    // allows ultimately to keep our draw() method const.
    SCOPED_SET_AND_RESTORE( state.needs_rendering, true );
    co_await 1_frames;
  }
  co_await pixelation_stage_throttler( hold, state.stage,
                                       /*negative=*/true );
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

wait<> LandViewAnimator::slide_throttler( co::latch&    hold,
                                          GenericUnitId id,
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
  // Must be last.
  co_await hold.arrive_and_wait();
}

wait<> LandViewAnimator::ensure_visible( Coord const& coord ) {
  if( !ss_.terrain.square_exists( coord ) ) {
    // We could have a situation where a ship is being animated
    // to move off the map edge to sail the high seas, and so
    // someone might end up calling this with such a tile. Even
    // in those cases, we do expect that it should be just one
    // tile off of the edge.
    CHECK(
        coord.is_inside(
            ss_.terrain.world_rect_tiles().with_border_added() ),
        "world map does not contain tile {}", coord );
    co_return;
  }
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
    AnimationAction const& action, co::latch& hold ) {
  AnimationPrimitive const& primitive = action.primitive;
  // Note: in the below, each animation primitive must handle the
  // latch. If the primitive leaves some visual animation state
  // different than when it started then it needs to use
  // arrive_and_wait, while otherwise it can use count_down. If
  // it delegates the animation to another function, it must give
  // the latch to that function so that it can do the above. Ei-
  // ther way, the latch counter must be decremented (through all
  // code paths!) otherwise the composite animation (phase) will
  // never terminate. The following check should enforce that at
  // least for the cases where we don't suspend. It is possible
  // that if we suspend, then another coroutine decrements the
  // latch, then we resume but don't decrement the latch, that
  // this check won't catch it, but it is better than nothing.
  int const latch_count_start = hold.counter();
  SCOPE_EXIT {
    CHECK( hold.counter() < latch_count_start,
           "a call to animation action {} failed to decrement "
           "the latch counter.",
           action );
  };
  // NOTE: Once again, we must tick the latch through every code
  // path in the below switch statement, otherwise animations
  // will hang!
  SWITCH( primitive ) {
    CASE( delay ) {
      co_await delay.duration;
      hold.count_down();
      break;
    }
    CASE( ensure_tile_visible ) {
      co_await ensure_visible( ensure_tile_visible.tile );
      hold.count_down();
      break;
    }
    CASE( play_sound ) {
      // TODO: should we co_await on the length of the sound ef-
      // fect? Maybe that would be a separate animation such as
      // `play_sound_wait`.
      play_sound_effect( play_sound.what );
      hold.count_down();
      break;
    }
    CASE( hide_unit ) {
      auto popper = add_unit_animation<UnitAnimationState::hide>(
          hide_unit.unit_id );
      co_await hold.arrive_and_wait();
      break;
    }
    CASE( front_unit ) {
      auto popper =
          add_unit_animation<UnitAnimationState::front>(
              front_unit.unit_id );
      co_await hold.arrive_and_wait();
      break;
    }
    CASE( slide_unit ) {
      auto& [unit_id, direction] = slide_unit;
      Coord const src =
          coord_for_unit_indirect_or_die( ss_.units, unit_id );
      Coord const dst = src.moved( direction );
      // The destination square may not exist if it is a ship
      // sailing the high seas by moving off of the map edge
      // (which the original game allows).
      bool const dst_exists = ss_.terrain.square_exists( dst );
      // Check visibility.
      bool const should_animate =
          dst_exists ? should_animate_move( *viz_, src, dst )
                     : should_animate_move( *viz_, src, src );
      if( !should_animate ) {
        hold.count_down();
        break;
      }
      co_await slide_throttler( hold, unit_id, direction );
      break;
    }
    CASE( depixelate_euro_unit ) {
      UnitId const unit_id = depixelate_euro_unit.unit_id;
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
      if( viz_->visible( tile ) !=
          e_tile_visibility::visible_and_clear ) {
        hold.count_down();
        break;
      }
      co_await unit_depixelation_throttler( hold, unit_id,
                                            /*target=*/nothing );
      break;
    }
    CASE( depixelate_native_unit ) {
      NativeUnitId const unit_id =
          depixelate_native_unit.unit_id;
      Coord const tile = ss_.units.coord_for( unit_id );
      // Check visibility. This will avoid both scrolling the map
      // and the animation in the case that the unit in question
      // is on a fogged tile, which can happen e.g. if we are de-
      // pixelating a native unit that is under fog when we de-
      // stroy its dwelling.
      if( viz_->visible( tile ) !=
          e_tile_visibility::visible_and_clear ) {
        hold.count_down();
        break;
      }
      co_await native_unit_depixelation_throttler(
          hold, unit_id,
          /*target=*/nothing );
      break;
    }
    CASE( enpixelate_unit ) {
      co_await unit_enpixelation_throttler(
          hold, enpixelate_unit.unit_id );
      break;
    }
    CASE( pixelate_euro_unit_to_target ) {
      auto& [unit_id, target] = pixelate_euro_unit_to_target;
      co_await unit_depixelation_throttler( hold, unit_id,
                                            target );
      break;
    }
    CASE( pixelate_native_unit_to_target ) {
      auto& [unit_id, target] = pixelate_native_unit_to_target;
      co_await native_unit_depixelation_throttler( hold, unit_id,
                                                   target );
      break;
    }
    CASE( depixelate_colony ) {
      Colony const& colony =
          ss_.colonies.colony_for( depixelate_colony.colony_id );
      co_await colony_depixelation_throttler( hold, colony );
      break;
    }
    CASE( depixelate_dwelling ) {
      Dwelling const& dwelling = ss_.natives.dwelling_for(
          depixelate_dwelling.dwelling_id );
      co_await dwelling_depixelation_throttler( hold, dwelling );
      break;
    }
    CASE( depixelate_fog_dwelling ) {
      co_await fog_dwelling_depixelation_throttler(
          hold, depixelate_fog_dwelling.tile );
      break;
    }
    CASE( landscape_anim_enpixelate ) {
      co_await landscape_anim_depixelation_throttler(
          hold, landscape_anim_enpixelate.targets );
      break;
    }
  }
}

wait<> LandViewAnimator::animate_sequence(
    AnimationSequence const& seq ) {
  for( vector<AnimationAction> const& sub_seq : seq.sequence ) {
    vector<wait<>> ws;
    ws.reserve( sub_seq.size() );
    // The latch is to synchronize the animations so that they
    // all end at the same time. If one is shorter than the other
    // and it were to return early, it would revert its final an-
    // imation state while the other animations keep going, which
    // would lead to incorrect visuals. The latch makes it so
    // that each animation, when it gets to the end, holds its
    // state until all of the other animations in the phase have
    // completed (meaning that they've either called count_down
    // or arrive_and_wait on the latch).
    co::latch hold( sub_seq.size() );
    for( AnimationAction const& action : sub_seq )
      ws.push_back( animate_action_primitive( action, hold ) );
    co_await co::all( std::move( ws ) );
  }
}

wait<> LandViewAnimator::animate_sequence_and_hold(
    AnimationSequence const& seq ) {
  if( seq.sequence.empty() ) co_return;
  // Since the phase we animate will be told to hold and not ever
  // return, it only makes sense to animate a single phase.
  CHECK( seq.sequence.size() == 1 );
  vector<AnimationAction> const& sub_seq = seq.sequence[0];

  vector<wait<>> ws;
  ws.reserve( sub_seq.size() );
  // See the corresponding comment in `animate_sequence` for an
  // explanation of the latch; the thing we do differently here
  // is ensure that the counter never hits zero, so i.e. the ani-
  // mation will run to completion, then just hold its final
  // state, never to return. Such animations are intended to be
  // cancelled at some point by the caller.
  co::latch hold( numeric_limits<int>::max() );
  for( AnimationAction const& action : sub_seq )
    ws.push_back( animate_action_primitive( action, hold ) );
  co_await co::all( std::move( ws ) );
}

} // namespace rn
