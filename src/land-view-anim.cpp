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
#include "co-wait.hpp"

// config
#include "config/rn.rds.hpp"

// ss
#include "ss/colony.rds.hpp"
#include "ss/settings.rds.hpp"

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

} // namespace rn
