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

AnimationAction& AnimationBuilder::slide_unit(
    GenericUnitId unit_id, e_direction direction ) {
  return push( P::slide_unit{ .unit_id   = unit_id,
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

AnimationAction& AnimationBuilder::enpixelate_landview_tiles(
    std::map<Coord, MapSquare> targets ) {
  return push( P::landscape_anim_enpixelate{
      .targets = std::move( targets ) } );
}

AnimationAction& AnimationBuilder::landview_mod_tiles(
    std::map<Coord, MapSquare> modded ) {
  return push(
      P::landscape_anim_mod{ .modded = std::move( modded ) } );
}

} // namespace rn
