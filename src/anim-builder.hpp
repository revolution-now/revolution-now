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

// base
#include "base/attributes.hpp"

namespace rn {

/****************************************************************
** AnimationBuilder
*****************************************************************/
struct AnimationBuilder {
  AnimationBuilder();

  AnimationSequence const& result() ATTR_LIFETIMEBOUND;

  void new_phase();

  AnimationAction& delay( std::chrono::microseconds duration );

  AnimationAction& play_sound( e_sfx what );

  AnimationAction& hide_unit( GenericUnitId unit_id );

  AnimationAction& front_unit( GenericUnitId unit_id );

  AnimationAction& slide_unit( GenericUnitId unit_id,
                               e_direction   direction );

  AnimationAction& depixelate_unit( GenericUnitId unit_id );

  AnimationAction& enpixelate_unit( GenericUnitId unit_id );

  AnimationAction& pixelate_euro_unit_to_target(
      UnitId unit_id, e_unit_type target );

  AnimationAction& pixelate_native_unit_to_target(
      NativeUnitId unit_id, e_native_unit_type target );

  AnimationAction& depixelate_colony( ColonyId colony_id );

  AnimationAction& depixelate_dwelling( DwellingId dwelling_id );

  AnimationAction& depixelate_fog_dwelling( Coord tile );

  AnimationAction& ensure_tile_visible( Coord tile );

 private:
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
