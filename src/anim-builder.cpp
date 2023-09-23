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

using namespace std;

namespace rn {

namespace {

using P = ::rn::AnimationPrimitive;

} // namespace

/****************************************************************
** AnimationBuilder
*****************************************************************/
AnimationBuilder::AnimationBuilder() { new_phase(); }

AnimationSequence const& AnimationBuilder::result() {
  return std::move( seq_ );
}

void AnimationBuilder::new_phase() {
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
  AnimationAction& action =
      push( P::hide_unit{ .unit_id = unit_id } );
  // The "hide" animation must always be a background animation
  // because it never ends.
  action.background = true;
  return action;
}

AnimationAction& AnimationBuilder::front_unit(
    GenericUnitId unit_id ) {
  AnimationAction& action =
      push( P::front_unit{ .unit_id = unit_id } );
  action.background = true;
  return action;
}

AnimationAction& AnimationBuilder::front_unit_non_background(
    GenericUnitId unit_id ) {
  AnimationAction& action =
      push( P::front_unit{ .unit_id = unit_id } );
  action.background = false;
  return action;
}

AnimationAction& AnimationBuilder::slide_unit(
    GenericUnitId unit_id, e_direction direction ) {
  return push( P::slide_unit{ .unit_id   = unit_id,
                              .direction = direction } );
}

AnimationAction& AnimationBuilder::depixelate_unit(
    GenericUnitId unit_id ) {
  return push( P::depixelate_unit{ .unit_id = unit_id } );
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
    ColonyId colony_id ) {
  return push( P::depixelate_colony{ .colony_id = colony_id } );
}

AnimationAction& AnimationBuilder::depixelate_dwelling(
    DwellingId dwelling_id ) {
  return push(
      P::depixelate_dwelling{ .dwelling_id = dwelling_id } );
}

} // namespace rn
