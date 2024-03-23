/****************************************************************
**anim-builder.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-01-28.
*
* Description: Representation for animation sequences in land
*              view.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// rds
#include "anim-builder.rds.hpp"

// Revolution Now
#include "maybe.hpp"

// base
#include "base/attributes.hpp"
#include "base/function-ref.hpp"

namespace rn {

/****************************************************************
** AnimationBuilder
*****************************************************************/
struct AnimationBuilder {
  AnimationBuilder();

  void clear();

  AnimationSequence const& result() const& ATTR_LIFETIMEBOUND;

  AnimationSequence result() &&;

  void new_phase();

  AnimationAction& delay( std::chrono::microseconds duration );

  AnimationAction& play_sound( e_sfx what );

  AnimationAction& hide_unit( GenericUnitId unit_id );

  AnimationAction& front_unit( GenericUnitId unit_id );

  AnimationAction& slide_unit( GenericUnitId unit_id,
                               e_direction   direction );

  AnimationAction& depixelate_euro_unit( UnitId unit_id );

  AnimationAction& depixelate_native_unit(
      NativeUnitId unit_id );

  AnimationAction& enpixelate_unit( GenericUnitId unit_id );

  AnimationAction& pixelate_euro_unit_to_target(
      UnitId unit_id, e_unit_type target );

  AnimationAction& pixelate_native_unit_to_target(
      NativeUnitId unit_id, e_native_unit_type target );

  AnimationAction& depixelate_colony( Coord tile );

  AnimationAction& hide_colony( Coord tile );

  AnimationAction& depixelate_dwelling( Coord tile );

  AnimationAction& hide_dwelling( Coord tile );

  AnimationAction& ensure_tile_visible( Coord tile );

  using MapSquareEditFn =
      base::function_ref<void( MapSquare& ) const>;
  inline static auto constexpr kMapSquareEditFnNoop =
      []( MapSquare& ) {};

  // Land View Replacement Buffer.
  // ------------------------------------------------------------
  AnimationAction& landview_replace_set_tile(
      Coord tile, MapSquare const& square );

  // Land View Enpixelation Buffer.
  // ------------------------------------------------------------
  // This function requires an initial value to use in the event
  // that this is the first edit on the tile.
  AnimationAction& landview_enpixelate_edit_tile(
      Coord tile, MapSquare const& initial, MapSquareEditFn op );

  AnimationAction& landview_enpixelate_dwelling_context(
      Coord tile, maybe<Dwelling const&> target );

 private:
  template<typename Primitive>
  AnimationAction& find_or_add_action();

  template<typename Primitive>
  AnimationAction& landview_anim_set_override(
      Coord tile, MapSquare const& def, MapSquareEditFn op );

  template<typename Primitive>
  AnimationAction& landview_anim_override_context(
      Coord tile, maybe<Dwelling const&> override );

  // Add another animation into the current phase.
  AnimationAction& push( auto&& o ) {
    CHECK( !seq_.sequence.empty() );
    seq_.sequence.back().push_back( AnimationAction{
        .primitive = AnimationPrimitive{ std::move( o ) } } );
    return seq_.sequence.back().back();
  }

  AnimationSequence seq_;
};
} // namespace rn
