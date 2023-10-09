/****************************************************************
**anim-builder.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-01-30.
*
* Description: Unit tests for the src/anim-builder.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/anim-builder.hpp"

// Must be last.
#include "test/catch-common.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-chrono.hpp"
#include "base/to-str-ext-std.hpp"

namespace rn {
namespace {

using namespace std;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[anim-builder] builders" ) {
  AnimationBuilder builder;

  builder.delay( chrono::seconds{ 1 } );
  builder.play_sound( e_sfx::attacker_won );
  builder.hide_unit( GenericUnitId{ 1 } );
  builder.front_unit( GenericUnitId{ 2 } );
  builder.slide_unit( GenericUnitId{ 3 }, e_direction::sw );
  builder.new_phase();
  builder.depixelate_euro_unit( UnitId{ 4 } );
  builder.enpixelate_unit( GenericUnitId{ 5 } );
  builder.pixelate_euro_unit_to_target( UnitId{ 6 },
                                        e_unit_type::cavalry );
  builder.new_phase();
  builder.pixelate_native_unit_to_target(
      NativeUnitId{ 7 }, e_native_unit_type::mounted_brave );
  builder.depixelate_colony( ColonyId{ 8 } );
  builder.depixelate_dwelling( DwellingId{ 9 } );
  builder.depixelate_fog_dwelling( Coord{ .x = 1, .y = 2 } );
  builder.ensure_tile_visible( Coord{ .x = 1, .y = 3 } );
  builder.depixelate_native_unit( NativeUnitId{ 10 } );

  AnimationSequence const& res = builder.result();

  using P = AnimationPrimitive;
  AnimationSequence const expected{
      .sequence = {
          /*phase 0*/ {
              { .primitive =
                    P::delay{ .duration =
                                  chrono::seconds{ 1 } } },
              { .primitive =
                    P::play_sound{ .what =
                                       e_sfx::attacker_won } },
              { .primitive =
                    P::hide_unit{ .unit_id =
                                      GenericUnitId{ 1 } } },
              { .primitive =
                    P::front_unit{ .unit_id =
                                       GenericUnitId{ 2 } } },
              { .primitive =
                    P::slide_unit{
                        .unit_id   = GenericUnitId{ 3 },
                        .direction = e_direction::sw } } },
          /*phase 1*/
          { { .primitive =
                  P::depixelate_euro_unit{ .unit_id =
                                               UnitId{ 4 } } },
            { .primitive =
                  P::enpixelate_unit{ .unit_id =
                                          GenericUnitId{ 5 } } },
            { .primitive =
                  P::pixelate_euro_unit_to_target{
                      .unit_id = UnitId{ 6 },
                      .target  = e_unit_type::cavalry } } },
          /*phase 2*/
          { { .primitive =
                  P::pixelate_native_unit_to_target{
                      .unit_id = NativeUnitId{ 7 },
                      .target =
                          e_native_unit_type::mounted_brave } },
            { .primitive =
                  P::depixelate_colony{ .colony_id =
                                            ColonyId{ 8 } } },
            { .primitive =
                  P::depixelate_dwelling{
                      .dwelling_id = DwellingId{ 9 } } },
            { .primitive =
                  P::depixelate_fog_dwelling{
                      .tile = { .x = 1, .y = 2 } } },
            { .primitive =
                  P::ensure_tile_visible{
                      .tile = { .x = 1, .y = 3 } } },
            { .primitive = P::depixelate_native_unit{
                  .unit_id = NativeUnitId{ 10 } } } } } };

  REQUIRE( res == expected );
}

} // namespace
} // namespace rn
