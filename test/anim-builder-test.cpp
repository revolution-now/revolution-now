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

// Testing.
#include "test/fake/world.hpp"

// Must be last.
#include "test/catch-common.hpp"

// ss
#include "src/ss/native-unit.rds.hpp"
#include "src/ss/ref.hpp"
#include "src/ss/terrain.hpp"
#include "src/ss/unit-composition.hpp"
#include "src/ss/unit.hpp"
#include "src/ss/units.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-chrono.hpp"
#include "base/to-str-ext-std.hpp"

namespace rn {
namespace {

using namespace std;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct world : testing::World {
  world() {
    add_default_player();
    create_default_map();
  }

  void create_default_map() {
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
    vector<MapSquare> tiles{
      L, L, L, //
      L, L, L, //
      L, L, L, //
    };
    build_map( std::move( tiles ), 3 );
  }
};

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
  builder.talk_unit( GenericUnitId{ 6 }, e_direction::sw );
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
  builder.translocate_unit( GenericUnitId{ 6 },
                            e_direction::se );

  AnimationSequence const& res = builder.result();

  using P = AnimationPrimitive;
  AnimationSequence const expected{
    .sequence = {
      /*phase 0*/ {
        { .primitive =
              P::delay{ .duration = chrono::seconds{ 1 } } },
        { .primitive =
              P::play_sound{ .what = e_sfx::attacker_won } },
        { .primitive =
              P::hide_unit{ .unit_id = GenericUnitId{ 1 } } },
        { .primitive =
              P::front_unit{ .unit_id = GenericUnitId{ 2 } } },
        { .primitive =
              P::slide_unit{ .unit_id   = GenericUnitId{ 3 },
                             .direction = e_direction::sw } },
        { .primitive =
              P::talk_unit{ .unit_id   = GenericUnitId{ 6 },
                            .direction = e_direction::sw } },
      },
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
                .target  = e_native_unit_type::mounted_brave } },
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
                .overrides = { .squares =
                                   {
                                     { Coord{ .x = 3, .y = 5 },
                                       MapSquare{} },
                                     { Coord{ .x = 4, .y = 6 },
                                       MapSquare{ .road =
                                                      true } },
                                   } } } },
        { .primitive =
              P::hide_colony{ .tile = { .x = 111, .y = 222 } } },
        { .primitive =
              P::hide_dwelling{
                .tile = { .x = 222, .y = 333 } } },
        { .primitive =
              P::translocate_unit{
                .unit_id   = GenericUnitId{ 6 },
                .direction = e_direction::se } },
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
    .sequence = {
      { { .primitive = AnimationPrimitive::delay{
            .duration = chrono::milliseconds{ 5 } } } } } };
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
                    { { .x = 1, .y = 2 }, { .road = true } },
                    { { .x = 2, .y = 3 },
                      { .irrigation = true } } } } } } } } };
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
                    { .squares = { { { .x = 1, .y = 2 },
                                     { .road = true } },
                                   { { .x = 2, .y = 3 },
                                     { .irrigation =
                                           true } } } } } },
        { .primitive = AnimationPrimitive::ensure_tile_visible{
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
                    { .squares = { { { .x = 1, .y = 2 },
                                     { .road = true } },
                                   { { .x = 2, .y = 3 },
                                     { .irrigation = true } },
                                   { { .x = 3, .y = 4 },
                                     { .lost_city_rumor =
                                           true } } } } } },
        { .primitive = AnimationPrimitive::ensure_tile_visible{
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
                    { { .x = 1, .y = 2 }, { .road = true } },
                    { { .x = 2, .y = 3 },
                      { .irrigation = true } } } } } } } } };
  REQUIRE( builder.result() == expected );
}

TEST_CASE( "[anim-builder] landview_enpixelate_edit_tile" ) {
  AnimationBuilder builder;

  AnimationSequence expected{ .sequence = { {} } };
  REQUIRE( builder.result() == expected );

  builder.delay( chrono::milliseconds{ 5 } );
  expected = {
    .sequence = {
      { { .primitive = AnimationPrimitive::delay{
            .duration = chrono::milliseconds{ 5 } } } } } };
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
                        .road       = true } } } } } } } } };
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
                    { .squares = { { { .x = 1, .y = 2 },
                                     { .irrigation = true,
                                       .road = true } } } } } },
        { .primitive = AnimationPrimitive::ensure_tile_visible{
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
                    { .squares = { { { .x = 1, .y = 2 },
                                     { .overlay =
                                           e_land_overlay::hills,
                                       .irrigation = true,
                                       .road = true } } } } } },
        { .primitive = AnimationPrimitive::ensure_tile_visible{
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
                              { .overlay = e_land_overlay::hills,
                                .irrigation = true,
                                .road       = true } },
                            { { .x = 2, .y = 2 },
                              { .overlay = e_land_overlay::
                                    hills } } } } } },
        { .primitive = AnimationPrimitive::ensure_tile_visible{
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
    .sequence = {
      { { .primitive = AnimationPrimitive::delay{
            .duration = chrono::milliseconds{ 5 } } } } } };
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
                .overrides = { .dwellings = {
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
        { .primitive = AnimationPrimitive::ensure_tile_visible{
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
                          { { { .x = 1, .y = 2 }, nothing },
                            { { .x = 2, .y = 2 },
                              Dwelling{ .is_capital =
                                            true } } } } } },
        { .primitive = AnimationPrimitive::ensure_tile_visible{
            .tile = { .x = 7, .y = 8 } } } } } };
  REQUIRE( builder.result() == expected );
}

TEST_CASE( "[anim-builder] animated_contents" ) {
  world w;
  AnimationSequence seq;
  AnimationContents expected;

  e_player const player = w.default_player_type();

  auto const f = [&] [[clang::noinline]] {
    return animated_contents( w.ss(), seq );
  };

  auto const set_prim = [&]( AnimationPrimitive const& prim ) {
    seq = {
      .sequence = { { AnimationAction{ .primitive = prim } } } };
  };

  UnitId const unit_id = w.add_unit_on_map( e_unit_type::soldier,
                                            { .x = 1, .y = 1 } )
                             .id();
  UnitId const edge_unit_id =
      w.add_unit_on_map( e_unit_type::soldier,
                         { .x = 0, .y = 1 } )
          .id();
  Colony const& colony = w.add_colony( { .x = 1, .y = 0 } );
  Dwelling const& dwelling =
      w.add_dwelling( { .x = 2, .y = 1 }, e_tribe::sioux );
  NativeUnitId const native_unit_id =
      w.add_native_unit_on_map( e_native_unit_type::brave,
                                { .x = 2, .y = 2 }, dwelling.id )
          .id;

  SECTION( "delay" ) {
    AnimationPrimitive::delay const prim{ .duration = {} };
    set_prim( prim );
    expected = { .tiles = {} };
    REQUIRE( f() == expected );
  }

  SECTION( "ensure_tile_visible" ) {
    AnimationPrimitive::ensure_tile_visible const prim{
      .tile = { .x = 1, .y = 2 } };
    set_prim( prim );
    expected = {
      .tiles = { AnimatedTile{ .tile        = { .x = 1, .y = 2 },
                               .inhabitants = {} } } };
    REQUIRE( f() == expected );
  }

  SECTION( "play_sound" ) {
    AnimationPrimitive::play_sound const prim{ .what = {} };
    set_prim( prim );
    expected = { .tiles = {} };
    REQUIRE( f() == expected );
  }

  SECTION( "hide_unit" ) {
    SECTION( "european" ) {
      AnimationPrimitive::hide_unit const prim{ .unit_id =
                                                    unit_id };
      set_prim( prim );
      expected = { .tiles = { AnimatedTile{
                     .tile        = { .x = 1, .y = 1 },
                     .inhabitants = { Society::european{
                       .player = player } } } } };
      REQUIRE( f() == expected );
    }
    SECTION( "native" ) {
      AnimationPrimitive::hide_unit const prim{
        .unit_id = native_unit_id };
      set_prim( prim );
      expected = { .tiles = { AnimatedTile{
                     .tile        = { .x = 2, .y = 2 },
                     .inhabitants = { Society::native{
                       .tribe = e_tribe::sioux } } } } };
      REQUIRE( f() == expected );
    }
  }

  SECTION( "front_unit" ) {
    SECTION( "european" ) {
      AnimationPrimitive::front_unit const prim{ .unit_id =
                                                     unit_id };
      set_prim( prim );
      expected = { .tiles = { AnimatedTile{
                     .tile        = { .x = 1, .y = 1 },
                     .inhabitants = { Society::european{
                       .player = player } } } } };
      REQUIRE( f() == expected );
    }
    SECTION( "native" ) {
      AnimationPrimitive::front_unit const prim{
        .unit_id = native_unit_id };
      set_prim( prim );
      expected = { .tiles = { AnimatedTile{
                     .tile        = { .x = 2, .y = 2 },
                     .inhabitants = { Society::native{
                       .tribe = e_tribe::sioux } } } } };
      REQUIRE( f() == expected );
    }
  }

  SECTION( "slide_unit" ) {
    SECTION( "european" ) {
      AnimationPrimitive::slide_unit const prim{
        .unit_id = unit_id, .direction = e_direction::se };
      set_prim( prim );
      expected = {
        .tiles = {
          AnimatedTile{ .tile        = { .x = 1, .y = 1 },
                        .inhabitants = { Society::european{
                          .player = player } } },
          // NOTE: even though we are sliding to a tile con-
          // taining a native unit, we don't include that native
          // unit in the dst tile's list of societies because the
          // native unit is not being animated in this animation
          // sequence.
          AnimatedTile{ .tile        = { .x = 2, .y = 2 },
                        .inhabitants = { Society::european{
                          .player = player } } } } };
      REQUIRE( f() == expected );
    }
    SECTION( "native" ) {
      AnimationPrimitive::slide_unit const prim{
        .unit_id   = native_unit_id,
        .direction = e_direction::nw };
      set_prim( prim );
      expected = {
        .tiles = {
          // NOTE: even though we are sliding to a tile con-
          // taining a european unit, we don't include that euro-
          // pean unit in the dst tile's list of societies be-
          // cause the european unit is not being animated in
          // this animation sequence.
          AnimatedTile{ .tile        = { .x = 1, .y = 1 },
                        .inhabitants = { Society::native{
                          .tribe = e_tribe::sioux } } },
          AnimatedTile{ .tile        = { .x = 2, .y = 2 },
                        .inhabitants = { Society::native{
                          .tribe = e_tribe::sioux } } } } };
      REQUIRE( f() == expected );
    }
  }

  SECTION( "slide_unit/dst off map" ) {
    e_direction const d = e_direction::w;
    AnimationPrimitive::slide_unit const prim{
      .unit_id = edge_unit_id, .direction = d };
    set_prim( prim );
    BASE_CHECK( !w.ss().terrain.square_exists(
        w.ss()
            .units.coord_for( edge_unit_id )
            .to_gfx()
            .moved( d ) ) );
    expected = { .tiles = { AnimatedTile{
                   .tile        = { .x = 0, .y = 1 },
                   .inhabitants = { Society::european{
                     .player = player } } } } };
    REQUIRE( f() == expected );
  }

  SECTION( "translocate_unit" ) {
    SECTION( "european" ) {
      AnimationPrimitive::translocate_unit const prim{
        .unit_id = unit_id, .direction = e_direction::se };
      set_prim( prim );
      expected = {
        .tiles = {
          AnimatedTile{ .tile        = { .x = 1, .y = 1 },
                        .inhabitants = { Society::european{
                          .player = player } } },
          // NOTE: even though we are moving to a tile containing
          // a native unit, we don't include that native unit in
          // the dst tile's list of societies because the native
          // unit is not being animated in this animation se-
          // quence.
          AnimatedTile{ .tile        = { .x = 2, .y = 2 },
                        .inhabitants = { Society::european{
                          .player = player } } } } };
      REQUIRE( f() == expected );
    }
    SECTION( "native" ) {
      AnimationPrimitive::translocate_unit const prim{
        .unit_id   = native_unit_id,
        .direction = e_direction::nw };
      set_prim( prim );
      expected = {
        .tiles = {
          // NOTE: even though we are moving to a tile containing
          // a european unit, we don't include that european unit
          // in the dst tile's list of societies because the eu-
          // ropean unit is not being animated in this animation
          // sequence.
          AnimatedTile{ .tile        = { .x = 1, .y = 1 },
                        .inhabitants = { Society::native{
                          .tribe = e_tribe::sioux } } },
          AnimatedTile{ .tile        = { .x = 2, .y = 2 },
                        .inhabitants = { Society::native{
                          .tribe = e_tribe::sioux } } } } };
      REQUIRE( f() == expected );
    }
  }

  SECTION( "translocate_unit/dst off map" ) {
    e_direction const d = e_direction::w;
    AnimationPrimitive::translocate_unit const prim{
      .unit_id = edge_unit_id, .direction = d };
    set_prim( prim );
    BASE_CHECK( !w.ss().terrain.square_exists(
        w.ss()
            .units.coord_for( edge_unit_id )
            .to_gfx()
            .moved( d ) ) );
    expected = { .tiles = { AnimatedTile{
                   .tile        = { .x = 0, .y = 1 },
                   .inhabitants = { Society::european{
                     .player = player } } } } };
    REQUIRE( f() == expected );
  }

  SECTION( "talk_unit" ) {
    SECTION( "european" ) {
      AnimationPrimitive::talk_unit const prim{
        .unit_id = unit_id, .direction = e_direction::se };
      set_prim( prim );
      expected = {
        .tiles = {
          AnimatedTile{ .tile        = { .x = 1, .y = 1 },
                        .inhabitants = { Society::european{
                          .player = player } } },
          // NOTE: even though we are sliding to a tile con-
          // taining a native unit, we don't include that native
          // unit in the dst tile's list of societies because the
          // native unit is not being animated in this animation
          // sequence.
          AnimatedTile{ .tile        = { .x = 2, .y = 2 },
                        .inhabitants = { Society::european{
                          .player = player } } } } };
      REQUIRE( f() == expected );
    }
    SECTION( "native" ) {
      AnimationPrimitive::talk_unit const prim{
        .unit_id   = native_unit_id,
        .direction = e_direction::nw };
      set_prim( prim );
      expected = {
        .tiles = {
          // NOTE: even though we are sliding to a tile con-
          // taining a european unit, we don't include that euro-
          // pean unit in the dst tile's list of societies be-
          // cause the european unit is not being animated in
          // this animation sequence.
          AnimatedTile{ .tile        = { .x = 1, .y = 1 },
                        .inhabitants = { Society::native{
                          .tribe = e_tribe::sioux } } },
          AnimatedTile{ .tile        = { .x = 2, .y = 2 },
                        .inhabitants = { Society::native{
                          .tribe = e_tribe::sioux } } } } };
      REQUIRE( f() == expected );
    }
  }

  SECTION( "depixelate_euro_unit" ) {
    AnimationPrimitive::depixelate_euro_unit const prim{
      .unit_id = unit_id };
    set_prim( prim );
    expected = { .tiles = { AnimatedTile{
                   .tile        = { .x = 1, .y = 1 },
                   .inhabitants = { Society::european{
                     .player = player } } } } };
    REQUIRE( f() == expected );
  }

  SECTION( "depixelate_native_unit" ) {
    AnimationPrimitive::depixelate_native_unit const prim{
      .unit_id = native_unit_id };
    set_prim( prim );
    expected = { .tiles = { AnimatedTile{
                   .tile        = { .x = 2, .y = 2 },
                   .inhabitants = { Society::native{
                     .tribe = e_tribe::sioux } } } } };
    REQUIRE( f() == expected );
  }

  SECTION( "enpixelate_unit" ) {
    SECTION( "european" ) {
      AnimationPrimitive::enpixelate_unit const prim{
        .unit_id = unit_id };
      set_prim( prim );
      expected = { .tiles = { AnimatedTile{
                     .tile        = { .x = 1, .y = 1 },
                     .inhabitants = { Society::european{
                       .player = player } } } } };
      REQUIRE( f() == expected );
    }
    SECTION( "native" ) {
      AnimationPrimitive::enpixelate_unit const prim{
        .unit_id = native_unit_id };
      set_prim( prim );
      expected = { .tiles = { AnimatedTile{
                     .tile        = { .x = 2, .y = 2 },
                     .inhabitants = { Society::native{
                       .tribe = e_tribe::sioux } } } } };
      REQUIRE( f() == expected );
    }
  }

  SECTION( "pixelate_euro_unit_to_target" ) {
    AnimationPrimitive::pixelate_euro_unit_to_target const prim{
      .unit_id = unit_id,
      .target  = e_unit_type::free_colonist,
    };
    set_prim( prim );
    expected = { .tiles = { AnimatedTile{
                   .tile        = { .x = 1, .y = 1 },
                   .inhabitants = { Society::european{
                     .player = player } } } } };
    REQUIRE( f() == expected );
  }

  SECTION( "pixelate_native_unit_to_target" ) {
    AnimationPrimitive::pixelate_native_unit_to_target const
        prim{ .unit_id = native_unit_id,
              .target  = e_native_unit_type::brave };
    set_prim( prim );
    expected = { .tiles = { AnimatedTile{
                   .tile        = { .x = 2, .y = 2 },
                   .inhabitants = { Society::native{
                     .tribe = e_tribe::sioux } } } } };
    REQUIRE( f() == expected );
  }

  SECTION( "depixelate_colony" ) {
    AnimationPrimitive::depixelate_colony const prim{
      .tile = colony.location };
    set_prim( prim );
    expected = { .tiles = { AnimatedTile{
                   .tile        = { .x = 1, .y = 0 },
                   .inhabitants = { Society::european{
                     .player = player } } } } };
    REQUIRE( f() == expected );
  }

  SECTION( "hide_colony (empty)" ) {
    AnimationPrimitive::hide_colony const prim{
      .tile = { .x = 0, .y = 0 } };
    set_prim( prim );
    expected = {
      .tiles = { AnimatedTile{ .tile = { .x = 0, .y = 0 } } } };
    REQUIRE( f() == expected );
  }

  SECTION( "hide_colony" ) {
    AnimationPrimitive::hide_colony const prim{
      .tile = colony.location };
    set_prim( prim );
    expected = { .tiles = { AnimatedTile{
                   .tile        = { .x = 1, .y = 0 },
                   .inhabitants = { Society::european{
                     .player = player } } } } };
    REQUIRE( f() == expected );
  }

  SECTION( "depixelate_dwelling" ) {
    AnimationPrimitive::depixelate_dwelling const prim{
      .tile = { .x = 2, .y = 1 } };
    set_prim( prim );
    expected = { .tiles = { AnimatedTile{
                   .tile        = { .x = 2, .y = 1 },
                   .inhabitants = { Society::native{
                     .tribe = e_tribe::sioux } } } } };
    REQUIRE( f() == expected );
  }

  SECTION( "hide_dwelling" ) {
    AnimationPrimitive::hide_dwelling const prim{
      .tile = { .x = 2, .y = 1 } };
    set_prim( prim );
    expected = { .tiles = { AnimatedTile{
                   .tile        = { .x = 2, .y = 1 },
                   .inhabitants = { Society::native{
                     .tribe = e_tribe::sioux } } } } };
    REQUIRE( f() == expected );
  }

  SECTION( "hide_dwelling (empty)" ) {
    AnimationPrimitive::hide_dwelling const prim{
      .tile = { .x = 0, .y = 0 } };
    set_prim( prim );
    expected = {
      .tiles = { AnimatedTile{ .tile = { .x = 0, .y = 0 } } } };
    REQUIRE( f() == expected );
  }

  SECTION( "landscape_anim_enpixelate" ) {
    AnimationPrimitive::landscape_anim_enpixelate const prim{
      .overrides = {
        .squares = { { Coord{ .x = 1, .y = 1 }, {} } } } };
    set_prim( prim );
    expected = {};
    REQUIRE( f() == expected );
  }

  SECTION( "landscape_anim_replace" ) {
    AnimationPrimitive::landscape_anim_replace const prim{
      .overrides = {
        .squares = { { Coord{ .x = 1, .y = 1 }, {} } } } };
    set_prim( prim );
    expected = {};
    REQUIRE( f() == expected );
  }

  SECTION( "combined: slide+depixelate" ) {
    AnimationPrimitive::slide_unit const prim1{
      .unit_id = unit_id, .direction = e_direction::se };
    AnimationPrimitive::depixelate_native_unit const prim2{
      .unit_id = native_unit_id };

    seq = { .sequence = {
              { AnimationAction{ .primitive = prim1 },
                AnimationAction{ .primitive = prim2 } } } };

    expected = {
      .tiles = {
        AnimatedTile{ .tile        = { .x = 1, .y = 1 },
                      .inhabitants = { Society::european{
                        .player = player } } },
        AnimatedTile{
          .tile        = { .x = 2, .y = 2 },
          .inhabitants = {
            Society::european{ .player = player },
            Society::native{ .tribe = e_tribe::sioux } } } } };
    REQUIRE( f() == expected );
  }
}

} // namespace
} // namespace rn
