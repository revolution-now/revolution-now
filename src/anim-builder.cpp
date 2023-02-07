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

namespace P = ::rn::AnimationPrimitive;

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

void AnimationBuilder::play_sound( e_sfx what ) {
  push( P::play_sound{ .what = what } );
}

void AnimationBuilder::hide_unit( GenericUnitId unit_id ) {
  AnimationAction& action =
      push( P::hide_unit{ .unit_id = unit_id } );
  // The "hide" animation must always be a background animation
  // because it never ends.
  action.background = true;
}

void AnimationBuilder::front_unit( GenericUnitId unit_id ) {
  AnimationAction& action =
      push( P::front_unit{ .unit_id = unit_id } );
  action.background = true;
}

void AnimationBuilder::front_unit_non_background(
    GenericUnitId unit_id ) {
  AnimationAction& action =
      push( P::front_unit{ .unit_id = unit_id } );
  action.background = false;
}

void AnimationBuilder::slide_unit( GenericUnitId unit_id,
                                   e_direction   direction ) {
  push( P::slide_unit{ .unit_id   = unit_id,
                       .direction = direction } );
}

void AnimationBuilder::depixelate_unit( GenericUnitId unit_id ) {
  push( P::depixelate_unit{ .unit_id = unit_id } );
}

void AnimationBuilder::enpixelate_unit( GenericUnitId unit_id ) {
  push( P::enpixelate_unit{ .unit_id = unit_id } );
}

void AnimationBuilder::depixelate_euro_unit_to_target(
    UnitId unit_id, e_unit_type target ) {
  push( P::depixelate_euro_unit_to_target{ .unit_id = unit_id,
                                           .target  = target } );
}

void AnimationBuilder::depixelate_native_unit_to_target(
    NativeUnitId unit_id, e_native_unit_type target ) {
  push( P::depixelate_native_unit_to_target{
      .unit_id = unit_id, .target = target } );
}

void AnimationBuilder::depixelate_colony( ColonyId colony_id ) {
  push( P::depixelate_colony{ .colony_id = colony_id } );
}

void AnimationBuilder::depixelate_dwelling(
    DwellingId dwelling_id ) {
  push( P::depixelate_dwelling{ .dwelling_id = dwelling_id } );
}

} // namespace rn
