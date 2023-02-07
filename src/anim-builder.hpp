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

  void play_sound( e_sfx what );

  void hide_unit( GenericUnitId unit_id );

  void front_unit( GenericUnitId unit_id );

  void front_unit_non_background( GenericUnitId unit_id );

  void slide_unit( GenericUnitId unit_id,
                   e_direction   direction );

  void depixelate_unit( GenericUnitId unit_id );

  void enpixelate_unit( GenericUnitId unit_id );

  void depixelate_euro_unit_to_target( UnitId      unit_id,
                                       e_unit_type target );

  void depixelate_native_unit_to_target(
      NativeUnitId unit_id, e_native_unit_type target );

  void depixelate_colony( ColonyId colony_id );

  void depixelate_dwelling( DwellingId dwelling_id );

 private:
  // Add another animation into the current phase.
  AnimationAction& push( auto&& o ) {
    CHECK( !seq_.sequence.empty() );
    seq_.sequence.back().push_back( AnimationAction{
        .primitive = AnimationPrimitive_t{ std::move( o ) } } );
    return seq_.sequence.back().back();
  }

  AnimationSequence seq_;
};
} // namespace rn
