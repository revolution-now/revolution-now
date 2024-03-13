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

AnimationAction& AnimationBuilder::landview_mod_tile(
    Coord tile, MapSquare const& square ) {
  using landscape_anim_mod =
      AnimationPrimitive::landscape_anim_mod;
  // We can have at most one landscape anim mod per phase, so
  // reuse one if we already have it, otherwise create one.
  AnimationAction& action = [&]() -> auto& {
    CHECK( !seq_.sequence.empty() );
    auto& latest_seq = seq_.sequence.back();
    // As an optimization, try from the back first since that is
    // where it is likely to be, since presumably we will be
    // adding these in bulk.
    for( auto it = latest_seq.rbegin(); it != latest_seq.rend();
         ++it ) {
      AnimationAction& action = *it;
      if( auto mod =
              action.primitive.get_if<landscape_anim_mod>();
          mod.has_value() )
        return action;
    }
    return push( P::landscape_anim_mod{} );
  }();

  UNWRAP_CHECK(
      map, action.primitive.inner_if<landscape_anim_mod>() );
  map[tile] = square;
  return action;
}

AnimationAction& AnimationBuilder::hide_colony( Coord tile ) {
  return push( P::hide_colony{ tile } );
}

AnimationAction& AnimationBuilder::hide_dwelling( Coord tile ) {
  return push( P::hide_dwelling{ tile } );
}

} // namespace rn
