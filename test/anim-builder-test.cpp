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
  builder.depixelate_colony( Coord{ .x = 7, .y = 8 } );
  builder.depixelate_dwelling( Coord{ .x = 9, .y = 10 } );
  builder.ensure_tile_visible( Coord{ .x = 1, .y = 3 } );
  builder.depixelate_native_unit( NativeUnitId{ 10 } );
  // First use the default square and no-op function to test
  // adding the square in on the first edit.
  builder.landview_enpixelate_edit_tile(
      Coord{ .x = 3, .y = 5 }, MapSquare{},
      AnimationBuilder::kMapSquareEditFnNoop );
  builder.landview_enpixelate_edit_tile(
      Coord{ .x = 4, .y = 6 }, MapSquare{},
      AnimationBuilder::kMapSquareEditFnNoop );
  // Now test an edit.
  builder.landview_enpixelate_edit_tile(
      Coord{ .x = 4, .y = 6 }, MapSquare{},
      []( auto& square ) { square.road = true; } );
  builder.hide_colony( { .x = 111, .y = 222 } );
  builder.hide_dwelling( { .x = 222, .y = 333 } );

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
          {
              { .primitive =
                    P::pixelate_native_unit_to_target{
                        .unit_id = NativeUnitId{ 7 },
                        .target  = e_native_unit_type::
                            mounted_brave } },
              { .primitive =
                    P::depixelate_colony{
                        .tile = Coord{ .x = 7, .y = 8 } } },
              { .primitive =
                    P::depixelate_dwelling{
                        .tile = Coord{ .x = 9, .y = 10 } } },
              { .primitive =
                    P::ensure_tile_visible{
                        .tile = { .x = 1, .y = 3 } } },
              { .primitive =
                    P::depixelate_native_unit{
                        .unit_id = NativeUnitId{ 10 } } },
              { .primitive =
                    P::landscape_anim_enpixelate{
                        .overrides =
                            { .squares =
                                  {
                                      { Coord{ .x = 3, .y = 5 },
                                        MapSquare{} },
                                      { Coord{ .x = 4, .y = 6 },
                                        MapSquare{
                                            .road = true } },
                                  } } } },
              { .primitive =
                    P::hide_colony{
                        .tile = { .x = 111, .y = 222 } } },
              { .primitive =
                    P::hide_dwelling{
                        .tile = { .x = 222, .y = 333 } } },
          } } };

  REQUIRE( res == expected );
}

TEST_CASE( "[anim-builder] first new_phase optional" ) {
  SECTION( "don't call new_phase" ) {
    AnimationBuilder builder;
    REQUIRE( builder.result() ==
             AnimationSequence{ .sequence = { {} } } );
  }

  SECTION( "call new_phase once" ) {
    AnimationBuilder builder;
    builder.new_phase();
    REQUIRE( builder.result() ==
             AnimationSequence{ .sequence = { {} } } );
  }

  SECTION( "call new_phase twice" ) {
    AnimationBuilder builder;
    builder.new_phase();
    builder.new_phase();
    REQUIRE( builder.result() ==
             AnimationSequence{ .sequence = { {} } } );
  }
}

TEST_CASE( "[anim-builder] emptiness and clearing" ) {
  AnimationBuilder builder;
  REQUIRE_FALSE( builder.result().sequence.empty() );
  REQUIRE( builder.result().sequence.back().empty() );

  builder.delay( chrono::seconds{ 1 } );
  REQUIRE_FALSE( builder.result().sequence.empty() );
  REQUIRE_FALSE( builder.result().sequence.back().empty() );

  auto copied = builder.result();
  REQUIRE_FALSE( builder.result().sequence.empty() );
  REQUIRE_FALSE( builder.result().sequence.back().empty() );

  builder.clear();
  REQUIRE_FALSE( builder.result().sequence.empty() );
  REQUIRE( builder.result().sequence.back().empty() );

  builder.delay( chrono::seconds{ 1 } );
  REQUIRE_FALSE( builder.result().sequence.empty() );
  REQUIRE_FALSE( builder.result().sequence.back().empty() );

  auto moved = std::move( builder ).result();
  REQUIRE_FALSE( builder.result().sequence.empty() );
  REQUIRE( builder.result().sequence.back().empty() );
}

TEST_CASE( "[anim-builder] landview_replace_set_tile" ) {
  AnimationBuilder builder;

  AnimationSequence expected{ .sequence = { {} } };
  REQUIRE( builder.result() == expected );

  builder.delay( chrono::milliseconds{ 5 } );
  expected = {
      .sequence = { { { .primitive = AnimationPrimitive::delay{
                            .duration = chrono::milliseconds{
                                5 } } } } } };
  REQUIRE( builder.result() == expected );

  builder.landview_replace_set_tile( { .x = 1, .y = 2 },
                                     { .road = true } );
  expected = {
      .sequence = {
          { { .primitive =
                  AnimationPrimitive::delay{
                      .duration = chrono::milliseconds{ 5 } } },
            { .primitive =
                  AnimationPrimitive::landscape_anim_replace{
                      .overrides = {
                          .squares = {
                              { { .x = 1, .y = 2 },
                                { .road = true } } } } } } } } };
  REQUIRE( builder.result() == expected );

  builder.landview_replace_set_tile( { .x = 2, .y = 3 },
                                     { .irrigation = true } );
  expected = {
      .sequence = {
          { { .primitive =
                  AnimationPrimitive::delay{
                      .duration = chrono::milliseconds{ 5 } } },
            { .primitive =
                  AnimationPrimitive::landscape_anim_replace{
                      .overrides = {
                          .squares = {
                              { { .x = 1, .y = 2 },
                                { .road = true } },
                              { { .x = 2, .y = 3 },
                                { .irrigation =
                                      true } } } } } } } } };
  REQUIRE( builder.result() == expected );

  builder.ensure_tile_visible( { .x = 7, .y = 8 } );
  expected = {
      .sequence = {
          { { .primitive =
                  AnimationPrimitive::delay{
                      .duration = chrono::milliseconds{ 5 } } },
            { .primitive =
                  AnimationPrimitive::landscape_anim_replace{
                      .overrides =
                          { .squares =
                                { { { .x = 1, .y = 2 },
                                    { .road = true } },
                                  { { .x = 2, .y = 3 },
                                    { .irrigation =
                                          true } } } } } },
            { .primitive =
                  AnimationPrimitive::ensure_tile_visible{
                      .tile = { .x = 7, .y = 8 } } } } } };
  REQUIRE( builder.result() == expected );

  builder.landview_replace_set_tile(
      { .x = 3, .y = 4 }, { .lost_city_rumor = true } );
  expected = {
      .sequence = {
          { { .primitive =
                  AnimationPrimitive::delay{
                      .duration = chrono::milliseconds{ 5 } } },
            { .primitive =
                  AnimationPrimitive::landscape_anim_replace{
                      .overrides =
                          { .squares =
                                { { { .x = 1, .y = 2 },
                                    { .road = true } },
                                  { { .x = 2, .y = 3 },
                                    { .irrigation = true } },
                                  { { .x = 3, .y = 4 },
                                    { .lost_city_rumor =
                                          true } } } } } },
            { .primitive =
                  AnimationPrimitive::ensure_tile_visible{
                      .tile = { .x = 7, .y = 8 } } } } } };
  REQUIRE( builder.result() == expected );

  builder.clear();
  expected = { .sequence = { {} } };
  REQUIRE( builder.result() == expected );

  // Now try with the landview_replace_set_tile as the first one.
  builder.landview_replace_set_tile( { .x = 1, .y = 2 },
                                     { .road = true } );
  expected = {
      .sequence = {
          { { .primitive =
                  AnimationPrimitive::landscape_anim_replace{
                      .overrides = {
                          .squares = {
                              { { .x = 1, .y = 2 },
                                { .road = true } } } } } } } } };
  REQUIRE( builder.result() == expected );

  builder.landview_replace_set_tile( { .x = 2, .y = 3 },
                                     { .irrigation = true } );
  expected = {
      .sequence = {
          { { .primitive =
                  AnimationPrimitive::landscape_anim_replace{
                      .overrides = {
                          .squares = {
                              { { .x = 1, .y = 2 },
                                { .road = true } },
                              { { .x = 2, .y = 3 },
                                { .irrigation =
                                      true } } } } } } } } };
  REQUIRE( builder.result() == expected );
}

TEST_CASE( "[anim-builder] landview_enpixelate_edit_tile" ) {
  AnimationBuilder builder;

  AnimationSequence expected{ .sequence = { {} } };
  REQUIRE( builder.result() == expected );

  builder.delay( chrono::milliseconds{ 5 } );
  expected = {
      .sequence = { { { .primitive = AnimationPrimitive::delay{
                            .duration = chrono::milliseconds{
                                5 } } } } } };
  REQUIRE( builder.result() == expected );

  builder.landview_enpixelate_edit_tile(
      { .x = 1, .y = 2 }, MapSquare{ .irrigation = true },
      []( MapSquare& square ) { square.road = true; } );
  expected = {
      .sequence = {
          { { .primitive =
                  AnimationPrimitive::delay{
                      .duration = chrono::milliseconds{ 5 } } },
            { .primitive =
                  AnimationPrimitive::landscape_anim_enpixelate{
                      .overrides = {
                          .squares = {
                              { { .x = 1, .y = 2 },
                                { .irrigation = true,
                                  .road = true } } } } } } } } };
  REQUIRE( builder.result() == expected );

  builder.ensure_tile_visible( { .x = 7, .y = 8 } );
  expected = {
      .sequence = {
          { { .primitive =
                  AnimationPrimitive::delay{
                      .duration = chrono::milliseconds{ 5 } } },
            { .primitive =
                  AnimationPrimitive::landscape_anim_enpixelate{
                      .overrides =
                          { .squares =
                                { { { .x = 1, .y = 2 },
                                    { .irrigation = true,
                                      .road = true } } } } } },
            { .primitive =
                  AnimationPrimitive::ensure_tile_visible{
                      .tile = { .x = 7, .y = 8 } } } } } };
  REQUIRE( builder.result() == expected );

  builder.landview_enpixelate_edit_tile(
      { .x = 1, .y = 2 }, MapSquare{}, []( MapSquare& square ) {
        square.overlay = e_land_overlay::hills;
      } );
  expected = {
      .sequence = {
          { { .primitive =
                  AnimationPrimitive::delay{
                      .duration = chrono::milliseconds{ 5 } } },
            { .primitive =
                  AnimationPrimitive::landscape_anim_enpixelate{
                      .overrides =
                          { .squares =
                                { { { .x = 1, .y = 2 },
                                    { .overlay =
                                          e_land_overlay::hills,
                                      .irrigation = true,
                                      .road = true } } } } } },
            { .primitive =
                  AnimationPrimitive::ensure_tile_visible{
                      .tile = { .x = 7, .y = 8 } } } } } };
  REQUIRE( builder.result() == expected );

  builder.landview_enpixelate_edit_tile(
      { .x = 2, .y = 2 }, MapSquare{}, []( MapSquare& square ) {
        square.overlay = e_land_overlay::hills;
      } );
  expected = {
      .sequence = {
          { { .primitive =
                  AnimationPrimitive::delay{
                      .duration = chrono::milliseconds{ 5 } } },
            { .primitive =
                  AnimationPrimitive::landscape_anim_enpixelate{
                      .overrides =
                          { .squares =
                                { { { .x = 1, .y = 2 },
                                    { .overlay =
                                          e_land_overlay::hills,
                                      .irrigation = true,
                                      .road       = true } },
                                  { { .x = 2, .y = 2 },
                                    { .overlay = e_land_overlay::
                                          hills } } } } } },
            { .primitive =
                  AnimationPrimitive::ensure_tile_visible{
                      .tile = { .x = 7, .y = 8 } } } } } };
  REQUIRE( builder.result() == expected );
}

TEST_CASE(
    "[anim-builder] landview_enpixelate_dwelling_context" ) {
  AnimationBuilder builder;

  AnimationSequence expected{ .sequence = { {} } };
  REQUIRE( builder.result() == expected );

  builder.delay( chrono::milliseconds{ 5 } );
  expected = {
      .sequence = { { { .primitive = AnimationPrimitive::delay{
                            .duration = chrono::milliseconds{
                                5 } } } } } };
  REQUIRE( builder.result() == expected );

  builder.landview_enpixelate_dwelling_context(
      { .x = 1, .y = 2 }, nothing );
  expected = {
      .sequence = {
          { { .primitive =
                  AnimationPrimitive::delay{
                      .duration = chrono::milliseconds{ 5 } } },
            { .primitive =
                  AnimationPrimitive::landscape_anim_enpixelate{
                      .overrides = {
                          .dwellings = {
                              { { .x = 1, .y = 2 }, nothing },
                          } } } } } } };
  REQUIRE( builder.result() == expected );

  builder.ensure_tile_visible( { .x = 7, .y = 8 } );
  expected = {
      .sequence = {
          { { .primitive =
                  AnimationPrimitive::delay{
                      .duration = chrono::milliseconds{ 5 } } },
            { .primitive =
                  AnimationPrimitive::landscape_anim_enpixelate{
                      .overrides =
                          { .dwellings = { { { .x = 1, .y = 2 },
                                             nothing } } } } },
            { .primitive =
                  AnimationPrimitive::ensure_tile_visible{
                      .tile = { .x = 7, .y = 8 } } } } } };
  REQUIRE( builder.result() == expected );

  builder.landview_enpixelate_dwelling_context(
      { .x = 2, .y = 2 }, Dwelling{ .is_capital = true } );
  expected = {
      .sequence = {
          { { .primitive =
                  AnimationPrimitive::delay{
                      .duration = chrono::milliseconds{ 5 } } },
            { .primitive =
                  AnimationPrimitive::landscape_anim_enpixelate{
                      .overrides =
                          { .dwellings =
                                { { { .x = 1, .y = 2 },
                                    nothing },
                                  { { .x = 2, .y = 2 },
                                    Dwelling{
                                        .is_capital =
                                            true } } } } } },
            { .primitive =
                  AnimationPrimitive::ensure_tile_visible{
                      .tile = { .x = 7, .y = 8 } } } } } };
  REQUIRE( builder.result() == expected );
}

} // namespace
} // namespace rn
