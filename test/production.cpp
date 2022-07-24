/****************************************************************
**production.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-06.
*
* Description: Unit tests for the src/production.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/production.hpp"

// Testing.
#include "test/fake/world.hpp"

// Revolution Now
#include "colony-mgr.hpp"
#include "map-square.hpp"

// ss
#include "src/ss/player.rds.hpp"
#include "src/ss/ref.hpp"
#include "src/ss/settings.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() { add_player( e_nation::dutch ); }

  static inline Coord kGrasslandTile{ .x = 1, .y = 1 };
  static inline Coord kPrairieTile{ .x = 4, .y = 1 };
  static inline Coord kConiferTile{ .x = 3, .y = 1 };
  static inline Coord kArcticTile{ .x = 7, .y = 1 };
  static inline Coord kDesertTile{ .x = 10, .y = 1 };

  void create_default_map() {
    MapSquare const _ = make_ocean();
    MapSquare const G = make_grassland();
    MapSquare const C = make_terrain( e_terrain::conifer );
    MapSquare const P = make_terrain( e_terrain::prairie );
    MapSquare const A = make_terrain( e_terrain::arctic );
    MapSquare const D = make_terrain( e_terrain::desert );
    // clang-format off
    vector<MapSquare> tiles{
      _, G, _, _, P, _, A, A, A, D, D, D,
      G, G, G, C, P, P, A, A, A, G, D, D,
      _, G, G, P, P, _, A, A, A, D, D, D,
    };
    // clang-format on
    build_map( std::move( tiles ), 12 );

    CHECK( effective_terrain( square( kGrasslandTile ) ) ==
           e_terrain::grassland );
    CHECK( effective_terrain( square( kPrairieTile ) ) ==
           e_terrain::prairie );
    CHECK( effective_terrain( square( kConiferTile ) ) ==
           e_terrain::conifer );
    CHECK( effective_terrain( square( kArcticTile ) ) ==
           e_terrain::arctic );
    CHECK( effective_terrain( square( kDesertTile ) ) ==
           e_terrain::desert );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[production] production_for_slot" ) {
  ColonyProduction pr;

  pr.tools_muskets.product_produced_theoretical  = 1;
  pr.tools_muskets.raw_produced                  = 2;
  pr.sugar_rum.product_produced_theoretical      = 3;
  pr.cotton_cloth.product_produced_theoretical   = 4;
  pr.fur_coats.product_produced_theoretical      = 5;
  pr.tobacco_cigars.product_produced_theoretical = 6;
  pr.lumber_hammers.product_produced_theoretical = 7;
  pr.bells                                       = 8;
  pr.food_horses.horses_produced_theoretical     = 12;
  pr.crosses                                     = 15;

  REQUIRE( production_for_slot(
               pr, e_colony_building_slot::muskets ) == 1 );
  REQUIRE( production_for_slot(
               pr, e_colony_building_slot::tools ) == 2 );
  REQUIRE( production_for_slot(
               pr, e_colony_building_slot::rum ) == 3 );
  REQUIRE( production_for_slot(
               pr, e_colony_building_slot::cloth ) == 4 );
  REQUIRE( production_for_slot(
               pr, e_colony_building_slot::coats ) == 5 );
  REQUIRE( production_for_slot(
               pr, e_colony_building_slot::cigars ) == 6 );
  REQUIRE( production_for_slot(
               pr, e_colony_building_slot::hammers ) == 7 );
  REQUIRE( production_for_slot(
               pr, e_colony_building_slot::town_hall ) == 8 );
  REQUIRE( production_for_slot(
               pr, e_colony_building_slot::newspapers ) ==
           nothing );
  REQUIRE( production_for_slot(
               pr, e_colony_building_slot::schools ) ==
           nothing );
  REQUIRE( production_for_slot(
               pr, e_colony_building_slot::offshore ) ==
           nothing );
  REQUIRE( production_for_slot(
               pr, e_colony_building_slot::horses ) == 12 );
  REQUIRE( production_for_slot(
               pr, e_colony_building_slot::wall ) == nothing );
  REQUIRE( production_for_slot(
               pr, e_colony_building_slot::warehouses ) ==
           nothing );
  REQUIRE( production_for_slot(
               pr, e_colony_building_slot::crosses ) == 15 );
  REQUIRE( production_for_slot(
               pr, e_colony_building_slot::custom_house ) ==
           nothing );
}

TEST_CASE( "[production] crosses" ) {
  World W;
  W.create_default_map();
  Colony& colony = W.add_colony( Coord{ .x = 1, .y = 1 } );
  Player& player = W.dutch();

  auto crosses = [&] {
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    return pr.crosses;
  };

  // Baseline.
  REQUIRE( crosses() == 1 );

  // With church.
  colony.buildings[e_colony_building::church] = true;
  REQUIRE( crosses() == 2 );

  // With free colonist working in church.
  W.add_unit_indoors( colony.id, e_indoor_job::crosses );
  REQUIRE( crosses() == 2 + 3 );

  // With free colonist and firebrand preacher working in church.
  W.add_unit_indoors( colony.id, e_indoor_job::crosses,
                      e_unit_type::firebrand_preacher );
  REQUIRE( crosses() == 2 + 3 + 6 );

  // With free colonist and firebrand preacher working in cathe-
  // dral.
  colony.buildings[e_colony_building::cathedral] = true;
  REQUIRE( crosses() == 3 + ( 3 + 6 ) * 2 );

  // Taking away the church should have no effect because the
  // cathedral should override it.
  colony.buildings[e_colony_building::church] = false;
  REQUIRE( crosses() == 3 + ( 3 + 6 ) * 2 );

  // With free colonist and firebrand preacher working in cathe-
  // dral with William Penn.
  player.fathers.has[e_founding_father::william_penn] = true;
  // Should be a 50% increase (rounded up) applied only to the
  // units' production and not the base production.
  REQUIRE( crosses() ==
           ( 3 + ( 3 + 6 ) * 2 ) + ( ( 3 + 6 ) * 2 ) / 2 );

  // All colonies constructed in this test should be valid.
  REQUIRE( W.validate_colonies() == base::valid );
}

// This test case emphasizes lumber production for many combina-
// tions of unit types and tile types. There is another test case
// that deals with conversion of lumber to hammers and warehouse
// capacity.
TEST_CASE(
    "[production] lumber production by colonist and tile "
    "type" ) {
  World W;
  W.create_default_map();
  Colony&    colony = W.add_colony( Coord{ .x = 1, .y = 1 } );
  gfx::point P{ .x = 0, .y = 1 };

  auto lum = [&] {
    return production_for_colony( W.ss(), colony )
        .lumber_hammers;
  };

  SECTION( "petty_criminal" ) {
    W.add_unit_outdoors( colony.id, e_direction::w,
                         e_outdoor_job::lumber,
                         e_unit_type::petty_criminal );
    SECTION( "grassland" ) {
      REQUIRE( lum() == RawMaterialAndProduct{} );

      // With road.
      W.add_road( P );
      REQUIRE( lum() == RawMaterialAndProduct{} );

      // With minor river.
      W.add_minor_river( P );
      REQUIRE( lum() == RawMaterialAndProduct{} );

      // With major river.
      W.add_major_river( P );
      REQUIRE( lum() == RawMaterialAndProduct{} );
    }
    SECTION( "conifer forest" ) {
      W.add_forest( P );
      REQUIRE( lum() == RawMaterialAndProduct{
                            .raw_produced          = 6,
                            .raw_delta_theoretical = 6,
                            .raw_delta_final       = 6,
                        } );

      // With road.
      W.add_road( P );
      REQUIRE( lum() == RawMaterialAndProduct{
                            .raw_produced          = 8,
                            .raw_delta_theoretical = 8,
                            .raw_delta_final       = 8,
                        } );

      // With minor river.
      W.add_minor_river( P );
      REQUIRE( lum() == RawMaterialAndProduct{
                            .raw_produced          = 10,
                            .raw_delta_theoretical = 10,
                            .raw_delta_final       = 10,
                        } );

      // With major river.
      W.add_major_river( P );
      REQUIRE( lum() == RawMaterialAndProduct{
                            .raw_produced          = 12,
                            .raw_delta_theoretical = 12,
                            .raw_delta_final       = 12,
                        } );
    }
    SECTION( "mountains" ) {
      W.add_mountains( P );
      REQUIRE( lum() == RawMaterialAndProduct{} );

      // With road.
      W.add_road( P );
      REQUIRE( lum() == RawMaterialAndProduct{} );

      // With minor river.
      W.add_minor_river( P );
      REQUIRE( lum() == RawMaterialAndProduct{} );

      // With major river.
      W.add_major_river( P );
      REQUIRE( lum() == RawMaterialAndProduct{} );
    }
  }

  SECTION( "native_convert" ) {
    W.add_unit_outdoors( colony.id, e_direction::w,
                         e_outdoor_job::lumber,
                         e_unit_type::native_convert );
    SECTION( "grassland" ) {
      REQUIRE( lum() == RawMaterialAndProduct{} );

      // With road.
      W.add_road( P );
      REQUIRE( lum() == RawMaterialAndProduct{} );

      // With minor river.
      W.add_minor_river( P );
      REQUIRE( lum() == RawMaterialAndProduct{} );

      // With major river.
      W.add_major_river( P );
      REQUIRE( lum() == RawMaterialAndProduct{} );
    }
    SECTION( "conifer forest" ) {
      W.add_forest( P );
      REQUIRE( lum() == RawMaterialAndProduct{
                            .raw_produced          = 6,
                            .raw_delta_theoretical = 6,
                            .raw_delta_final       = 6,
                        } );

      // With road.
      W.add_road( P );
      REQUIRE( lum() == RawMaterialAndProduct{
                            .raw_produced          = 8,
                            .raw_delta_theoretical = 8,
                            .raw_delta_final       = 8,
                        } );

      // With minor river.
      W.add_minor_river( P );
      REQUIRE( lum() == RawMaterialAndProduct{
                            .raw_produced          = 10,
                            .raw_delta_theoretical = 10,
                            .raw_delta_final       = 10,
                        } );

      // With major river.
      W.add_major_river( P );
      REQUIRE( lum() == RawMaterialAndProduct{
                            .raw_produced          = 12,
                            .raw_delta_theoretical = 12,
                            .raw_delta_final       = 12,
                        } );
    }
    SECTION( "mountains" ) {
      W.add_mountains( P );
      REQUIRE( lum() == RawMaterialAndProduct{} );

      // With road.
      W.add_road( P );
      REQUIRE( lum() == RawMaterialAndProduct{} );

      // With minor river.
      W.add_minor_river( P );
      REQUIRE( lum() == RawMaterialAndProduct{} );

      // With major river.
      W.add_major_river( P );
      REQUIRE( lum() == RawMaterialAndProduct{} );
    }
  }

  SECTION( "free_colonist" ) {
    W.add_unit_outdoors( colony.id, e_direction::w,
                         e_outdoor_job::lumber,
                         e_unit_type::free_colonist );
    SECTION( "grassland" ) {
      REQUIRE( lum() == RawMaterialAndProduct{} );

      // With road.
      W.add_road( P );
      REQUIRE( lum() == RawMaterialAndProduct{} );

      // With minor river.
      W.add_minor_river( P );
      REQUIRE( lum() == RawMaterialAndProduct{} );

      // With major river.
      W.add_major_river( P );
      REQUIRE( lum() == RawMaterialAndProduct{} );
    }
    SECTION( "conifer forest" ) {
      W.add_forest( P );
      REQUIRE( lum() == RawMaterialAndProduct{
                            .raw_produced          = 6,
                            .raw_delta_theoretical = 6,
                            .raw_delta_final       = 6,
                        } );

      // With road.
      W.add_road( P );
      REQUIRE( lum() == RawMaterialAndProduct{
                            .raw_produced          = 8,
                            .raw_delta_theoretical = 8,
                            .raw_delta_final       = 8,
                        } );

      // With minor river.
      W.add_minor_river( P );
      REQUIRE( lum() == RawMaterialAndProduct{
                            .raw_produced          = 10,
                            .raw_delta_theoretical = 10,
                            .raw_delta_final       = 10,
                        } );

      // With major river.
      W.add_major_river( P );
      REQUIRE( lum() == RawMaterialAndProduct{
                            .raw_produced          = 12,
                            .raw_delta_theoretical = 12,
                            .raw_delta_final       = 12,
                        } );
    }
    SECTION( "mountains" ) {
      W.add_mountains( P );
      REQUIRE( lum() == RawMaterialAndProduct{} );

      // With road.
      W.add_road( P );
      REQUIRE( lum() == RawMaterialAndProduct{} );

      // With minor river.
      W.add_minor_river( P );
      REQUIRE( lum() == RawMaterialAndProduct{} );

      // With major river.
      W.add_major_river( P );
      REQUIRE( lum() == RawMaterialAndProduct{} );
    }
  }

  SECTION( "expert_lumberjack" ) {
    W.add_unit_outdoors( colony.id, e_direction::w,
                         e_outdoor_job::lumber,
                         e_unit_type::expert_lumberjack );
    SECTION( "grassland" ) {
      REQUIRE( lum() == RawMaterialAndProduct{} );

      // With road.
      W.add_road( P );
      REQUIRE( lum() == RawMaterialAndProduct{} );

      // With minor river.
      W.add_minor_river( P );
      REQUIRE( lum() == RawMaterialAndProduct{} );

      // With major river.
      W.add_major_river( P );
      REQUIRE( lum() == RawMaterialAndProduct{} );
    }
    SECTION( "conifer forest" ) {
      W.add_forest( P );
      REQUIRE( lum() == RawMaterialAndProduct{
                            .raw_produced          = 12,
                            .raw_delta_theoretical = 12,
                            .raw_delta_final       = 12,
                        } );

      // With road.
      W.add_road( P );
      REQUIRE( lum() == RawMaterialAndProduct{
                            .raw_produced          = 16,
                            .raw_delta_theoretical = 16,
                            .raw_delta_final       = 16,
                        } );

      // With minor river.
      W.add_minor_river( P );
      REQUIRE( lum() == RawMaterialAndProduct{
                            .raw_produced          = 20,
                            .raw_delta_theoretical = 20,
                            .raw_delta_final       = 20,
                        } );

      // With major river.
      W.add_major_river( P );
      REQUIRE( lum() == RawMaterialAndProduct{
                            .raw_produced          = 24,
                            .raw_delta_theoretical = 24,
                            .raw_delta_final       = 24,
                        } );
    }
    SECTION( "mountains" ) {
      W.add_mountains( P );
      REQUIRE( lum() == RawMaterialAndProduct{} );

      // With road.
      W.add_road( P );
      REQUIRE( lum() == RawMaterialAndProduct{} );

      // With minor river.
      W.add_minor_river( P );
      REQUIRE( lum() == RawMaterialAndProduct{} );

      // With major river.
      W.add_major_river( P );
      REQUIRE( lum() == RawMaterialAndProduct{} );
    }
  }
}

TEST_CASE( "[production] lumber+hammers [discoverer]" ) {
  World W;
  W.create_default_map();

  W.square( W.kGrasslandTile - Delta{ .w = 1 } ).overlay =
      e_land_overlay::forest;
  // Adding a forest to the colony square will also help us to
  // enforce that there is no lumber produced on the center
  // square.
  W.square( W.kGrasslandTile ).overlay = e_land_overlay::forest;

  using SP  = SquareProduction;
  using LP  = refl::enum_map<e_direction, SP>;
  using RMP = RawMaterialAndProduct;

  W.settings().difficulty = e_difficulty::discoverer;

  SECTION( "no lumberjack" ) {
    Colony&          colony = W.add_colony( W.kGrasslandTile );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE( pr.lumber_hammers ==
             RMP{
                 .raw_produced                 = 0,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 0,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
  }

  SECTION( "one lumberjack" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    W.add_unit_outdoors( colony.id, e_direction::w,
                         e_outdoor_job::lumber,
                         e_unit_type::free_colonist );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::w, SP{ .what = e_outdoor_job::lumber,
                                  .quantity = 6 } } } );
    REQUIRE( pr.lumber_hammers ==
             RMP{
                 .raw_produced                 = 6,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 6,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 6,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
  }

  SECTION( "one lumberjack, almost full warehouse" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.commodities[e_commodity::lumber] = 98;
    W.add_unit_outdoors( colony.id, e_direction::w,
                         e_outdoor_job::lumber,
                         e_unit_type::free_colonist );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::w, SP{ .what = e_outdoor_job::lumber,
                                  .quantity = 6 } } } );
    REQUIRE( pr.lumber_hammers ==
             RMP{
                 .raw_produced                 = 6,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 6,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 2,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
  }

  SECTION( "one expert lumberjack, full warehouse" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.commodities[e_commodity::lumber] = 100;
    W.add_unit_outdoors( colony.id, e_direction::w,
                         e_outdoor_job::lumber,
                         e_unit_type::expert_lumberjack );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::w, SP{ .what = e_outdoor_job::lumber,
                                  .quantity = 12 } } } );
    REQUIRE( pr.lumber_hammers ==
             RMP{
                 .raw_produced                 = 12,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 12,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
  }

  SECTION( "one lumberjack, over warehouse" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.commodities[e_commodity::lumber] = 150;
    W.add_unit_outdoors( colony.id, e_direction::w,
                         e_outdoor_job::lumber,
                         e_unit_type::free_colonist );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::w, SP{ .what = e_outdoor_job::lumber,
                                  .quantity = 6 } } } );
    REQUIRE( pr.lumber_hammers ==
             RMP{
                 .raw_produced                 = 6,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 6,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
  }

  SECTION( "no lumberjack/with carpenter" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.commodities[e_commodity::lumber] = 0;
    W.add_unit_indoors( colony.id, e_indoor_job::hammers,
                        e_unit_type::free_colonist );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.lumber_hammers ==
             RMP{
                 .raw_produced                 = 0,
                 .raw_consumed_theoretical     = 3,
                 .raw_delta_theoretical        = -3,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 3,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
  }

  SECTION( "no lumberjack/with carpenter, lumber in store" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.commodities[e_commodity::lumber] = 1;
    W.add_unit_indoors( colony.id, e_indoor_job::hammers,
                        e_unit_type::free_colonist );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.lumber_hammers ==
             RMP{
                 .raw_produced                 = 0,
                 .raw_consumed_theoretical     = 3,
                 .raw_delta_theoretical        = -3,
                 .raw_consumed_actual          = 1,
                 .raw_delta_final              = -1,
                 .product_produced_theoretical = 3,
                 .product_produced_actual      = 1,
                 .product_delta_final          = 1,
             } );
  }

  SECTION(
      "no lumberjack/with master carpenter, lumber in store" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.commodities[e_commodity::lumber] = 50;
    W.add_unit_indoors( colony.id, e_indoor_job::hammers,
                        e_unit_type::master_carpenter );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.lumber_hammers ==
             RMP{
                 .raw_produced                 = 0,
                 .raw_consumed_theoretical     = 6,
                 .raw_delta_theoretical        = -6,
                 .raw_consumed_actual          = 6,
                 .raw_delta_final              = -6,
                 .product_produced_theoretical = 6,
                 .product_produced_actual      = 6,
                 .product_delta_final          = 6,
             } );
  }

  SECTION(
      "one lumberjack/with master carpenter, no lumber in "
      "store" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.commodities[e_commodity::lumber] = 0;
    W.add_unit_outdoors( colony.id, e_direction::w,
                         e_outdoor_job::lumber,
                         e_unit_type::free_colonist );
    W.add_unit_indoors( colony.id, e_indoor_job::hammers,
                        e_unit_type::master_carpenter );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::w, SP{ .what = e_outdoor_job::lumber,
                                  .quantity = 6 } } } );
    REQUIRE( pr.lumber_hammers ==
             RMP{
                 .raw_produced                 = 6,
                 .raw_consumed_theoretical     = 6,
                 .raw_delta_theoretical        = 0,
                 .raw_consumed_actual          = 6,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 6,
                 .product_produced_actual      = 6,
                 .product_delta_final          = 6,
             } );
  }

  SECTION(
      "one expert lumberjack, with master carpenter, some "
      "lumber in store, lumber mill" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.commodities[e_commodity::lumber]          = 50;
    colony.buildings[e_colony_building::lumber_mill] = true;
    W.add_unit_outdoors( colony.id, e_direction::w,
                         e_outdoor_job::lumber,
                         e_unit_type::expert_lumberjack );
    W.add_unit_indoors( colony.id, e_indoor_job::hammers,
                        e_unit_type::master_carpenter );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::w, SP{ .what = e_outdoor_job::lumber,
                                  .quantity = 12 } } } );
    REQUIRE( pr.lumber_hammers ==
             RMP{
                 .raw_produced                 = 12,
                 .raw_consumed_theoretical     = 12,
                 .raw_delta_theoretical        = 0,
                 .raw_consumed_actual          = 12,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 12,
                 .product_produced_actual      = 12,
                 .product_delta_final          = 12,
             } );
  }

  SECTION(
      "one expert lumberjack, with two master carpenters, some "
      "lumber in store, lumber mill" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.commodities[e_commodity::lumber]          = 50;
    colony.buildings[e_colony_building::lumber_mill] = true;
    W.add_unit_outdoors( colony.id, e_direction::w,
                         e_outdoor_job::lumber,
                         e_unit_type::expert_lumberjack );
    W.add_unit_indoors( colony.id, e_indoor_job::hammers,
                        e_unit_type::master_carpenter );
    W.add_unit_indoors( colony.id, e_indoor_job::hammers,
                        e_unit_type::master_carpenter );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::w, SP{ .what = e_outdoor_job::lumber,
                                  .quantity = 12 } } } );
    REQUIRE( pr.lumber_hammers ==
             RMP{
                 .raw_produced                 = 12,
                 .raw_consumed_theoretical     = 24,
                 .raw_delta_theoretical        = -12,
                 .raw_consumed_actual          = 24,
                 .raw_delta_final              = -12,
                 .product_produced_theoretical = 24,
                 .product_produced_actual      = 24,
                 .product_delta_final          = 24,
             } );
  }
}

TEST_CASE( "[production] silver [discoverer]" ) {
  World W;
  W.create_default_map();
  MapSquare& has_silver      = W.square( World::kGrasslandTile -
                                         Delta{ .w = 1, .h = 0 } );
  has_silver.overlay         = e_land_overlay::mountains;
  has_silver.ground_resource = e_natural_resource::silver;
  MapSquare& has_silver2     = W.square( World::kGrasslandTile +
                                         Delta{ .w = 0, .h = 1 } );
  has_silver2.overlay        = e_land_overlay::mountains;
  has_silver2.ground_resource =
      e_natural_resource::silver_depleted;

  using SP  = SquareProduction;
  using LP  = refl::enum_map<e_direction, SP>;
  using RMP = RawMaterialAndProduct;

  W.settings().difficulty = e_difficulty::discoverer;

  SECTION( "silver miner, warehouse empty" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    W.add_unit_outdoors( colony.id, e_direction::w,
                         e_outdoor_job::silver,
                         e_unit_type::expert_silver_miner );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::w, SP{ .what = e_outdoor_job::silver,
                                  .quantity = 6 } } } );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::tobacco, .quantity = 4 } );
    int const ex = 6;
    REQUIRE( pr.silver == RMP{
                              .raw_produced                 = ex,
                              .raw_consumed_theoretical     = 0,
                              .raw_delta_theoretical        = ex,
                              .raw_consumed_actual          = 0,
                              .raw_delta_final              = ex,
                              .product_produced_theoretical = 0,
                              .product_produced_actual      = 0,
                              .product_delta_final          = 0,
                          } );
  }

  SECTION( "two silver miners, warehouse empty" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    W.add_unit_outdoors( colony.id, e_direction::w,
                         e_outdoor_job::silver,
                         e_unit_type::expert_silver_miner );
    W.add_unit_outdoors( colony.id, e_direction::s,
                         e_outdoor_job::silver,
                         e_unit_type::free_colonist );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::w, SP{ .what = e_outdoor_job::silver,
                                  .quantity = 6 } },
            { e_direction::s, SP{ .what = e_outdoor_job::silver,
                                  .quantity = 1 } } } );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::tobacco, .quantity = 4 } );
    int const ex = 7;
    REQUIRE( pr.silver == RMP{
                              .raw_produced                 = ex,
                              .raw_consumed_theoretical     = 0,
                              .raw_delta_theoretical        = ex,
                              .raw_consumed_actual          = 0,
                              .raw_delta_final              = ex,
                              .product_produced_theoretical = 0,
                              .product_produced_actual      = 0,
                              .product_delta_final          = 0,
                          } );
  }

  SECTION( "silver miner, warehouse partial" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.commodities[e_commodity::silver] = 50;
    W.add_unit_outdoors( colony.id, e_direction::w,
                         e_outdoor_job::silver,
                         e_unit_type::expert_silver_miner );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::w, SP{ .what = e_outdoor_job::silver,
                                  .quantity = 6 } } } );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::tobacco, .quantity = 4 } );
    int const ex = 6;
    REQUIRE( pr.silver == RMP{
                              .raw_produced                 = ex,
                              .raw_consumed_theoretical     = 0,
                              .raw_delta_theoretical        = ex,
                              .raw_consumed_actual          = 0,
                              .raw_delta_final              = ex,
                              .product_produced_theoretical = 0,
                              .product_produced_actual      = 0,
                              .product_delta_final          = 0,
                          } );
  }

  SECTION( "silver miner, warehouse almost full" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.commodities[e_commodity::silver] = 98;
    W.add_unit_outdoors( colony.id, e_direction::w,
                         e_outdoor_job::silver,
                         e_unit_type::expert_silver_miner );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::w, SP{ .what = e_outdoor_job::silver,
                                  .quantity = 6 } } } );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::tobacco, .quantity = 4 } );
    REQUIRE( pr.silver == RMP{
                              .raw_produced                 = 6,
                              .raw_consumed_theoretical     = 0,
                              .raw_delta_theoretical        = 6,
                              .raw_consumed_actual          = 0,
                              .raw_delta_final              = 2,
                              .product_produced_theoretical = 0,
                              .product_produced_actual      = 0,
                              .product_delta_final          = 0,
                          } );
  }

  SECTION( "silver miner, warehouse full" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.commodities[e_commodity::silver] = 100;
    W.add_unit_outdoors( colony.id, e_direction::w,
                         e_outdoor_job::silver,
                         e_unit_type::expert_silver_miner );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::w, SP{ .what = e_outdoor_job::silver,
                                  .quantity = 6 } } } );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::tobacco, .quantity = 4 } );
    REQUIRE( pr.silver == RMP{
                              .raw_produced                 = 6,
                              .raw_consumed_theoretical     = 0,
                              .raw_delta_theoretical        = 6,
                              .raw_consumed_actual          = 0,
                              .raw_delta_final              = 0,
                              .product_produced_theoretical = 0,
                              .product_produced_actual      = 0,
                              .product_delta_final          = 0,
                          } );
  }

  SECTION( "silver miner, warehouse over" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.commodities[e_commodity::silver] = 150;
    W.add_unit_outdoors( colony.id, e_direction::w,
                         e_outdoor_job::silver,
                         e_unit_type::expert_silver_miner );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::w, SP{ .what = e_outdoor_job::silver,
                                  .quantity = 6 } } } );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::tobacco, .quantity = 4 } );
    REQUIRE( pr.silver == RMP{
                              .raw_produced                 = 6,
                              .raw_consumed_theoretical     = 0,
                              .raw_delta_theoretical        = 6,
                              .raw_consumed_actual          = 0,
                              .raw_delta_final              = 0,
                              .product_produced_theoretical = 0,
                              .product_produced_actual      = 0,
                              .product_delta_final          = 0,
                          } );
  }
}

TEST_CASE( "[production] tobacco/cigar [discoverer]" ) {
  World W;
  W.create_default_map();

  using SP  = SquareProduction;
  using LP  = refl::enum_map<e_direction, SP>;
  using RMP = RawMaterialAndProduct;

  W.settings().difficulty = e_difficulty::discoverer;

  SECTION( "center square tobacco only" ) {
    Colony&          colony = W.add_colony( W.kGrasslandTile );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::tobacco, .quantity = 4 } );
    REQUIRE( pr.tobacco_cigars ==
             RMP{
                 .raw_produced                 = 4,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 4,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 4,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
  }

  SECTION(
      "center square tobacco only, almost full warehouse" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.commodities[e_commodity::tobacco] = 98;
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::tobacco, .quantity = 4 } );
    REQUIRE( pr.tobacco_cigars ==
             RMP{
                 .raw_produced                 = 4,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 4,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 2,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
  }

  SECTION( "center square tobacco only, full warehouse" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.commodities[e_commodity::tobacco] = 100;
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::tobacco, .quantity = 4 } );
    REQUIRE( pr.tobacco_cigars ==
             RMP{
                 .raw_produced                 = 4,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 4,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
  }

  SECTION( "center square tobacco only, over warehouse" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.commodities[e_commodity::tobacco] = 150;
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::tobacco, .quantity = 4 } );
    REQUIRE( pr.tobacco_cigars ==
             RMP{
                 .raw_produced                 = 4,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 4,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
  }

  SECTION( "one tobacco planter" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    W.add_unit_outdoors( colony.id, e_direction::w,
                         e_outdoor_job::tobacco );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::w, SP{ .what = e_outdoor_job::tobacco,
                                  .quantity = 3 } } } );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::tobacco, .quantity = 4 } );
    REQUIRE( pr.tobacco_cigars ==
             RMP{
                 .raw_produced                 = 7,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 7,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 7,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
  }

  SECTION( "no tobacco center square" ) {
    Colony&          colony = W.add_colony( W.kPrairieTile );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::cotton, .quantity = 4 } );
    REQUIRE( pr.tobacco_cigars ==
             RMP{
                 .raw_produced                 = 0,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 0,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
  }

  SECTION( "no tobacco center square/with rum distiller" ) {
    Colony& colony = W.add_colony( W.kPrairieTile );
    W.add_unit_indoors( colony.id, e_indoor_job::rum,
                        e_unit_type::master_rum_distiller );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::cotton, .quantity = 4 } );
    REQUIRE( pr.tobacco_cigars ==
             RMP{
                 .raw_produced                 = 0,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 0,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
  }

  SECTION(
      "no tobacco center square/with tobacconist, tobacco in "
      "store" ) {
    Colony& colony = W.add_colony( W.kPrairieTile );
    colony.commodities[e_commodity::tobacco] = 50;
    W.add_unit_indoors( colony.id, e_indoor_job::cigars,
                        e_unit_type::free_colonist );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::cotton, .quantity = 4 } );
    REQUIRE( pr.tobacco_cigars ==
             RMP{
                 .raw_produced                 = 0,
                 .raw_consumed_theoretical     = 3,
                 .raw_delta_theoretical        = -3,
                 .raw_consumed_actual          = 3,
                 .raw_delta_final              = -3,
                 .product_produced_theoretical = 3,
                 .product_produced_actual      = 3,
                 .product_delta_final          = 3,
             } );
  }

  SECTION(
      "no tobacco center square/with master tobacconist, some "
      "tobacco in store" ) {
    Colony& colony = W.add_colony( W.kPrairieTile );
    colony.commodities[e_commodity::tobacco] = 3;
    W.add_unit_indoors( colony.id, e_indoor_job::cigars,
                        e_unit_type::master_tobacconist );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::cotton, .quantity = 4 } );
    REQUIRE( pr.tobacco_cigars ==
             RMP{
                 .raw_produced                 = 0,
                 .raw_consumed_theoretical     = 6,
                 .raw_delta_theoretical        = -6,
                 .raw_consumed_actual          = 3,
                 .raw_delta_final              = -3,
                 .product_produced_theoretical = 6,
                 .product_produced_actual      = 3,
                 .product_delta_final          = 3,
             } );
  }

  SECTION(
      "no tobacco center square/with tobacconist, some tobacco "
      "in store" ) {
    Colony& colony = W.add_colony( W.kPrairieTile );
    colony.commodities[e_commodity::tobacco] = 3;
    W.add_unit_indoors( colony.id, e_indoor_job::cigars,
                        e_unit_type::free_colonist );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::cotton, .quantity = 4 } );
    REQUIRE( pr.tobacco_cigars ==
             RMP{
                 .raw_produced                 = 0,
                 .raw_consumed_theoretical     = 3,
                 .raw_delta_theoretical        = -3,
                 .raw_consumed_actual          = 3,
                 .raw_delta_final              = -3,
                 .product_produced_theoretical = 3,
                 .product_produced_actual      = 3,
                 .product_delta_final          = 3,
             } );
  }

  SECTION(
      "no tobacco center square/with master tobacconist, "
      "tobacco in store" ) {
    Colony& colony = W.add_colony( W.kPrairieTile );
    colony.commodities[e_commodity::tobacco] = 50;
    W.add_unit_indoors( colony.id, e_indoor_job::cigars,
                        e_unit_type::master_tobacconist );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::cotton, .quantity = 4 } );
    REQUIRE( pr.tobacco_cigars ==
             RMP{
                 .raw_produced                 = 0,
                 .raw_consumed_theoretical     = 6,
                 .raw_delta_theoretical        = -6,
                 .raw_consumed_actual          = 6,
                 .raw_delta_final              = -6,
                 .product_produced_theoretical = 6,
                 .product_produced_actual      = 6,
                 .product_delta_final          = 6,
             } );
  }

  SECTION(
      "no tobacco center square/with master tobacconist, "
      "tobacco in store, almost full warehouse for cigars" ) {
    Colony& colony = W.add_colony( W.kPrairieTile );
    colony.commodities[e_commodity::tobacco] = 50;
    colony.commodities[e_commodity::cigars]  = 98;
    W.add_unit_indoors( colony.id, e_indoor_job::cigars,
                        e_unit_type::master_tobacconist );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::cotton, .quantity = 4 } );
    REQUIRE( pr.tobacco_cigars ==
             RMP{
                 .raw_produced             = 0,
                 .raw_consumed_theoretical = 6,
                 .raw_delta_theoretical    = -6,
                 // No backpressure.
                 .raw_consumed_actual          = 6,
                 .raw_delta_final              = -6,
                 .product_produced_theoretical = 6,
                 .product_produced_actual      = 6,
                 .product_delta_final          = 2,
             } );
  }

  SECTION(
      "no tobacco center square/with master tobacconist, "
      "tobacco in store, full warehouse for cigars" ) {
    Colony& colony = W.add_colony( W.kPrairieTile );
    colony.commodities[e_commodity::tobacco] = 50;
    colony.commodities[e_commodity::cigars]  = 100;
    W.add_unit_indoors( colony.id, e_indoor_job::cigars,
                        e_unit_type::master_tobacconist );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::cotton, .quantity = 4 } );
    REQUIRE( pr.tobacco_cigars ==
             RMP{
                 .raw_produced             = 0,
                 .raw_consumed_theoretical = 6,
                 .raw_delta_theoretical    = -6,
                 // No backpressure.
                 .raw_consumed_actual          = 6,
                 .raw_delta_final              = -6,
                 .product_produced_theoretical = 6,
                 .product_produced_actual      = 6,
                 .product_delta_final          = 0,
             } );
  }

  SECTION(
      "no tobacco center square/with master tobacconist, "
      "tobacco in store, over warehouse for cigars" ) {
    Colony& colony = W.add_colony( W.kPrairieTile );
    colony.commodities[e_commodity::tobacco] = 50;
    colony.commodities[e_commodity::cigars]  = 150;
    W.add_unit_indoors( colony.id, e_indoor_job::cigars,
                        e_unit_type::master_tobacconist );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::cotton, .quantity = 4 } );
    REQUIRE( pr.tobacco_cigars ==
             RMP{
                 .raw_produced             = 0,
                 .raw_consumed_theoretical = 6,
                 .raw_delta_theoretical    = -6,
                 // No backpressure.
                 .raw_consumed_actual          = 6,
                 .raw_delta_final              = -6,
                 .product_produced_theoretical = 6,
                 .product_produced_actual      = 6,
                 .product_delta_final          = 0,
             } );
  }

  SECTION(
      "tobacco center square/with master tobacconist, "
      "no tobacco in store, no cigars in store" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.commodities[e_commodity::tobacco] = 0;
    colony.commodities[e_commodity::cigars]  = 0;
    W.add_unit_indoors( colony.id, e_indoor_job::cigars,
                        e_unit_type::master_tobacconist );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::tobacco, .quantity = 4 } );
    REQUIRE( pr.tobacco_cigars ==
             RMP{
                 .raw_produced                 = 4,
                 .raw_consumed_theoretical     = 6,
                 .raw_delta_theoretical        = -2,
                 .raw_consumed_actual          = 4,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 6,
                 .product_produced_actual      = 4,
                 .product_delta_final          = 4,
             } );
  }

  SECTION(
      "tobacco center square/with master tobacconist, "
      "no tobacco in store, full cigar warehouse" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.commodities[e_commodity::tobacco] = 0;
    colony.commodities[e_commodity::cigars]  = 100;
    W.add_unit_indoors( colony.id, e_indoor_job::cigars,
                        e_unit_type::master_tobacconist );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::tobacco, .quantity = 4 } );
    REQUIRE( pr.tobacco_cigars ==
             RMP{
                 .raw_produced                 = 4,
                 .raw_consumed_theoretical     = 6,
                 .raw_delta_theoretical        = -2,
                 .raw_consumed_actual          = 4,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 6,
                 .product_produced_actual      = 4,
                 .product_delta_final          = 0,
             } );
  }

  SECTION(
      "tobacco center square, one tobacco planter, with master "
      "tobacconist, no tobacco in store, no cigars in store, "
      "cigar shop" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.buildings[e_colony_building::tobacconists_shop] =
        true;
    colony.commodities[e_commodity::tobacco] = 0;
    colony.commodities[e_commodity::cigars]  = 0;
    W.add_unit_outdoors( colony.id, e_direction::w,
                         e_outdoor_job::tobacco,
                         e_unit_type::free_colonist );
    W.add_unit_indoors( colony.id, e_indoor_job::cigars,
                        e_unit_type::master_tobacconist );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::w, SP{ .what = e_outdoor_job::tobacco,
                                  .quantity = 3 } } } );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::tobacco, .quantity = 4 } );
    REQUIRE( pr.tobacco_cigars ==
             RMP{
                 .raw_produced                 = 7,
                 .raw_consumed_theoretical     = 12,
                 .raw_delta_theoretical        = -5,
                 .raw_consumed_actual          = 7,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 12,
                 .product_produced_actual      = 7,
                 .product_delta_final          = 7,
             } );
  }

  SECTION(
      "tobacco center square, one tobacco planter, with master "
      "tobacconist, some tobacco in store, no cigars in store, "
      "cigar shop" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.buildings[e_colony_building::tobacconists_shop] =
        true;
    colony.commodities[e_commodity::tobacco] = 50;
    colony.commodities[e_commodity::cigars]  = 0;
    W.add_unit_outdoors( colony.id, e_direction::w,
                         e_outdoor_job::tobacco,
                         e_unit_type::free_colonist );
    W.add_unit_indoors( colony.id, e_indoor_job::cigars,
                        e_unit_type::master_tobacconist );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::w, SP{ .what = e_outdoor_job::tobacco,
                                  .quantity = 3 } } } );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::tobacco, .quantity = 4 } );
    REQUIRE( pr.tobacco_cigars ==
             RMP{
                 .raw_produced                 = 7,
                 .raw_consumed_theoretical     = 12,
                 .raw_delta_theoretical        = -5,
                 .raw_consumed_actual          = 12,
                 .raw_delta_final              = -5,
                 .product_produced_theoretical = 12,
                 .product_produced_actual      = 12,
                 .product_delta_final          = 12,
             } );
  }

  SECTION(
      "tobacco center square, one tobacco planter, with master "
      "tobacconist, no tobacco in store, no cigars in store, "
      "cigar factory" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.buildings[e_colony_building::cigar_factory] = true;
    colony.commodities[e_commodity::tobacco]           = 0;
    colony.commodities[e_commodity::cigars]            = 0;
    W.add_unit_outdoors( colony.id, e_direction::w,
                         e_outdoor_job::tobacco,
                         e_unit_type::free_colonist );
    W.add_unit_indoors( colony.id, e_indoor_job::cigars,
                        e_unit_type::master_tobacconist );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::w, SP{ .what = e_outdoor_job::tobacco,
                                  .quantity = 3 } } } );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::tobacco, .quantity = 4 } );
    REQUIRE( pr.tobacco_cigars ==
             RMP{
                 .raw_produced                 = 7,
                 .raw_consumed_theoretical     = 12,
                 .raw_delta_theoretical        = -5,
                 .raw_consumed_actual          = 7,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 18,
                 .product_produced_actual      = 11,
                 .product_delta_final          = 11,
             } );
  }

  SECTION(
      "tobacco center square, one tobacco planter, with master "
      "tobacconist, some tobacco in store, no cigars in store, "
      "cigar factory" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.buildings[e_colony_building::cigar_factory] = true;
    colony.commodities[e_commodity::tobacco]           = 2;
    colony.commodities[e_commodity::cigars]            = 0;
    W.add_unit_outdoors( colony.id, e_direction::w,
                         e_outdoor_job::tobacco,
                         e_unit_type::free_colonist );
    W.add_unit_indoors( colony.id, e_indoor_job::cigars,
                        e_unit_type::master_tobacconist );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::w, SP{ .what = e_outdoor_job::tobacco,
                                  .quantity = 3 } } } );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::tobacco, .quantity = 4 } );
    REQUIRE( pr.tobacco_cigars ==
             RMP{
                 .raw_produced                 = 7,
                 .raw_consumed_theoretical     = 12,
                 .raw_delta_theoretical        = -5,
                 .raw_consumed_actual          = 9,
                 .raw_delta_final              = -2,
                 .product_produced_theoretical = 18,
                 .product_produced_actual      = 14,
                 .product_delta_final          = 14,
             } );
  }

  SECTION(
      "tobacco center square, one expert tobacco planter, with "
      "master tobacconist, no tobacco in store, no cigars in "
      "store, cigar factory" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.buildings[e_colony_building::cigar_factory] = true;
    colony.commodities[e_commodity::tobacco]           = 0;
    colony.commodities[e_commodity::cigars]            = 0;
    W.add_unit_outdoors( colony.id, e_direction::w,
                         e_outdoor_job::tobacco,
                         e_unit_type::expert_tobacco_planter );
    W.add_unit_indoors( colony.id, e_indoor_job::cigars,
                        e_unit_type::master_tobacconist );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::w, SP{ .what = e_outdoor_job::tobacco,
                                  .quantity = 6 } } } );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::tobacco, .quantity = 4 } );
    REQUIRE( pr.tobacco_cigars ==
             RMP{
                 .raw_produced                 = 10,
                 .raw_consumed_theoretical     = 12,
                 .raw_delta_theoretical        = -2,
                 .raw_consumed_actual          = 10,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 18,
                 .product_produced_actual      = 15,
                 .product_delta_final          = 15,
             } );
  }

  SECTION(
      "tobacco center square, one expert tobacco planter, one "
      "tobacco planter, with master tobacconist, no tobacco in "
      "store, no cigars in store, cigar factory" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.buildings[e_colony_building::cigar_factory] = true;
    colony.commodities[e_commodity::tobacco]           = 0;
    colony.commodities[e_commodity::cigars]            = 0;
    W.add_unit_outdoors( colony.id, e_direction::w,
                         e_outdoor_job::tobacco,
                         e_unit_type::expert_tobacco_planter );
    W.add_unit_outdoors( colony.id, e_direction::e,
                         e_outdoor_job::tobacco,
                         e_unit_type::petty_criminal );
    W.add_unit_indoors( colony.id, e_indoor_job::cigars,
                        e_unit_type::master_tobacconist );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::w, SP{ .what = e_outdoor_job::tobacco,
                                  .quantity = 6 } },
            { e_direction::e, SP{ .what = e_outdoor_job::tobacco,
                                  .quantity = 3 } } } );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::tobacco, .quantity = 4 } );
    REQUIRE( pr.tobacco_cigars ==
             RMP{
                 .raw_produced                 = 13,
                 .raw_consumed_theoretical     = 12,
                 .raw_delta_theoretical        = 1,
                 .raw_consumed_actual          = 12,
                 .raw_delta_final              = 1,
                 .product_produced_theoretical = 18,
                 .product_produced_actual      = 18,
                 .product_delta_final          = 18,
             } );
  }

  SECTION(
      "tobacco center square, one expert tobacco planter, one "
      "tobacco planter, with master tobacconist, full tobacco "
      "in store, no cigars in store, cigar factory" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.buildings[e_colony_building::cigar_factory] = true;
    colony.commodities[e_commodity::tobacco]           = 100;
    colony.commodities[e_commodity::cigars]            = 0;
    W.add_unit_outdoors( colony.id, e_direction::w,
                         e_outdoor_job::tobacco,
                         e_unit_type::expert_tobacco_planter );
    W.add_unit_outdoors( colony.id, e_direction::e,
                         e_outdoor_job::tobacco,
                         e_unit_type::petty_criminal );
    W.add_unit_indoors( colony.id, e_indoor_job::cigars,
                        e_unit_type::master_tobacconist );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::w, SP{ .what = e_outdoor_job::tobacco,
                                  .quantity = 6 } },
            { e_direction::e, SP{ .what = e_outdoor_job::tobacco,
                                  .quantity = 3 } } } );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::tobacco, .quantity = 4 } );
    REQUIRE( pr.tobacco_cigars ==
             RMP{
                 .raw_produced                 = 13,
                 .raw_consumed_theoretical     = 12,
                 .raw_delta_theoretical        = 1,
                 .raw_consumed_actual          = 12,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 18,
                 .product_produced_actual      = 18,
                 .product_delta_final          = 18,
             } );
  }

  SECTION(
      "tobacco center square, one expert tobacco planter, one "
      "tobacco planter, two master tobacconists, full tobacco "
      "in store, no cigars in store, cigar factory" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.buildings[e_colony_building::cigar_factory] = true;
    colony.commodities[e_commodity::tobacco]           = 100;
    colony.commodities[e_commodity::cigars]            = 0;
    W.add_unit_outdoors( colony.id, e_direction::w,
                         e_outdoor_job::tobacco,
                         e_unit_type::expert_tobacco_planter );
    W.add_unit_outdoors( colony.id, e_direction::e,
                         e_outdoor_job::tobacco,
                         e_unit_type::petty_criminal );
    W.add_unit_indoors( colony.id, e_indoor_job::cigars,
                        e_unit_type::master_tobacconist );
    W.add_unit_indoors( colony.id, e_indoor_job::cigars,
                        e_unit_type::master_tobacconist );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::w, SP{ .what = e_outdoor_job::tobacco,
                                  .quantity = 6 } },
            { e_direction::e, SP{ .what = e_outdoor_job::tobacco,
                                  .quantity = 3 } } } );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::tobacco, .quantity = 4 } );
    REQUIRE( pr.tobacco_cigars ==
             RMP{
                 .raw_produced                 = 13,
                 .raw_consumed_theoretical     = 24,
                 .raw_delta_theoretical        = -11,
                 .raw_consumed_actual          = 24,
                 .raw_delta_final              = -11,
                 .product_produced_theoretical = 36,
                 .product_produced_actual      = 36,
                 .product_delta_final          = 36,
             } );
  }

  SECTION(
      "tobacco center square, one expert tobacco planter, one "
      "tobacco planter, two master tobacconists, some tobacco "
      "in store, no cigars in store, cigar factory" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.buildings[e_colony_building::cigar_factory] = true;
    colony.commodities[e_commodity::tobacco]           = 2;
    colony.commodities[e_commodity::cigars]            = 0;
    W.add_unit_outdoors( colony.id, e_direction::w,
                         e_outdoor_job::tobacco,
                         e_unit_type::expert_tobacco_planter );
    W.add_unit_outdoors( colony.id, e_direction::e,
                         e_outdoor_job::tobacco,
                         e_unit_type::petty_criminal );
    W.add_unit_indoors( colony.id, e_indoor_job::cigars,
                        e_unit_type::master_tobacconist );
    W.add_unit_indoors( colony.id, e_indoor_job::cigars,
                        e_unit_type::master_tobacconist );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::w, SP{ .what = e_outdoor_job::tobacco,
                                  .quantity = 6 } },
            { e_direction::e, SP{ .what = e_outdoor_job::tobacco,
                                  .quantity = 3 } } } );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::tobacco, .quantity = 4 } );
    REQUIRE( pr.tobacco_cigars ==
             RMP{
                 .raw_produced                 = 13,
                 .raw_consumed_theoretical     = 24,
                 .raw_delta_theoretical        = -11,
                 .raw_consumed_actual          = 15,
                 .raw_delta_final              = -2,
                 .product_produced_theoretical = 36,
                 .product_produced_actual      = 23,
                 .product_delta_final          = 23,
             } );
  }

  SECTION(
      "tobacco center square, one expert tobacco planter, one "
      "tobacco planter, two master tobacconists, some tobacco "
      "in store, cigars almost full, cigar factory" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.buildings[e_colony_building::cigar_factory] = true;
    colony.commodities[e_commodity::tobacco]           = 2;
    colony.commodities[e_commodity::cigars]            = 90;
    W.add_unit_outdoors( colony.id, e_direction::w,
                         e_outdoor_job::tobacco,
                         e_unit_type::expert_tobacco_planter );
    W.add_unit_outdoors( colony.id, e_direction::e,
                         e_outdoor_job::tobacco,
                         e_unit_type::petty_criminal );
    W.add_unit_indoors( colony.id, e_indoor_job::cigars,
                        e_unit_type::master_tobacconist );
    W.add_unit_indoors( colony.id, e_indoor_job::cigars,
                        e_unit_type::master_tobacconist );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::w, SP{ .what = e_outdoor_job::tobacco,
                                  .quantity = 6 } },
            { e_direction::e, SP{ .what = e_outdoor_job::tobacco,
                                  .quantity = 3 } } } );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::tobacco, .quantity = 4 } );
    REQUIRE( pr.tobacco_cigars ==
             RMP{
                 .raw_produced                 = 13,
                 .raw_consumed_theoretical     = 24,
                 .raw_delta_theoretical        = -11,
                 .raw_consumed_actual          = 15,
                 .raw_delta_final              = -2,
                 .product_produced_theoretical = 36,
                 .product_produced_actual      = 23,
                 .product_delta_final          = 10,
             } );
  }
}

TEST_CASE( "[production] cotton/cloth [explorer]" ) {
  World W;
  W.create_default_map();

  using SP  = SquareProduction;
  using LP  = refl::enum_map<e_direction, SP>;
  using RMP = RawMaterialAndProduct;

  W.settings().difficulty = e_difficulty::explorer;

  SECTION( "center square cotton only" ) {
    Colony&          colony = W.add_colony( W.kPrairieTile );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::cotton, .quantity = 3 } );
    REQUIRE( pr.cotton_cloth ==
             RMP{
                 .raw_produced                 = 3,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 3,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 3,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
  }

  SECTION( "center square cotton only, almost full warehouse" ) {
    Colony& colony = W.add_colony( W.kPrairieTile );
    colony.commodities[e_commodity::cotton] = 98;
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::cotton, .quantity = 3 } );
    REQUIRE( pr.cotton_cloth ==
             RMP{
                 .raw_produced                 = 3,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 3,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 2,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
  }

  SECTION( "center square cotton only, full warehouse" ) {
    Colony& colony = W.add_colony( W.kPrairieTile );
    colony.commodities[e_commodity::cotton] = 100;
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::cotton, .quantity = 3 } );
    REQUIRE( pr.cotton_cloth ==
             RMP{
                 .raw_produced                 = 3,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 3,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
  }

  SECTION( "center square cotton only, over warehouse" ) {
    Colony& colony = W.add_colony( W.kPrairieTile );
    colony.commodities[e_commodity::cotton] = 150;
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::cotton, .quantity = 3 } );
    REQUIRE( pr.cotton_cloth ==
             RMP{
                 .raw_produced                 = 3,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 3,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
  }

  SECTION( "one cotton planter" ) {
    Colony& colony = W.add_colony( W.kPrairieTile );
    W.add_unit_outdoors( colony.id, e_direction::e,
                         e_outdoor_job::cotton );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::e, SP{ .what = e_outdoor_job::cotton,
                                  .quantity = 3 } } } );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::cotton, .quantity = 3 } );
    REQUIRE( pr.cotton_cloth ==
             RMP{
                 .raw_produced                 = 6,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 6,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 6,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
  }

  SECTION( "no cotton center square" ) {
    Colony&          colony = W.add_colony( W.kGrasslandTile );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::tobacco, .quantity = 3 } );
    REQUIRE( pr.cotton_cloth ==
             RMP{
                 .raw_produced                 = 0,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 0,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
  }

  SECTION( "no cotton center square/with rum distiller" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    W.add_unit_indoors( colony.id, e_indoor_job::rum,
                        e_unit_type::master_rum_distiller );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::tobacco, .quantity = 3 } );
    REQUIRE( pr.cotton_cloth ==
             RMP{
                 .raw_produced                 = 0,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 0,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
  }

  SECTION(
      "no cotton center square/with weaver, cotton in store" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.commodities[e_commodity::cotton] = 50;
    W.add_unit_indoors( colony.id, e_indoor_job::cloth,
                        e_unit_type::free_colonist );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::tobacco, .quantity = 3 } );
    REQUIRE( pr.cotton_cloth ==
             RMP{
                 .raw_produced                 = 0,
                 .raw_consumed_theoretical     = 3,
                 .raw_delta_theoretical        = -3,
                 .raw_consumed_actual          = 3,
                 .raw_delta_final              = -3,
                 .product_produced_theoretical = 3,
                 .product_produced_actual      = 3,
                 .product_delta_final          = 3,
             } );
  }

  SECTION(
      "no cotton center square/with master weaver, some cotton "
      "in store" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.commodities[e_commodity::cotton] = 3;
    W.add_unit_indoors( colony.id, e_indoor_job::cloth,
                        e_unit_type::master_weaver );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::tobacco, .quantity = 3 } );
    REQUIRE( pr.cotton_cloth ==
             RMP{
                 .raw_produced                 = 0,
                 .raw_consumed_theoretical     = 6,
                 .raw_delta_theoretical        = -6,
                 .raw_consumed_actual          = 3,
                 .raw_delta_final              = -3,
                 .product_produced_theoretical = 6,
                 .product_produced_actual      = 3,
                 .product_delta_final          = 3,
             } );
  }

  SECTION(
      "no cotton center square/with weaver, some cotton in "
      "store" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.commodities[e_commodity::cotton] = 3;
    W.add_unit_indoors( colony.id, e_indoor_job::cloth,
                        e_unit_type::free_colonist );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::tobacco, .quantity = 3 } );
    REQUIRE( pr.cotton_cloth ==
             RMP{
                 .raw_produced                 = 0,
                 .raw_consumed_theoretical     = 3,
                 .raw_delta_theoretical        = -3,
                 .raw_consumed_actual          = 3,
                 .raw_delta_final              = -3,
                 .product_produced_theoretical = 3,
                 .product_produced_actual      = 3,
                 .product_delta_final          = 3,
             } );
  }

  SECTION(
      "no cotton center square/with master weaver, cotton in "
      "store" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.commodities[e_commodity::cotton] = 50;
    W.add_unit_indoors( colony.id, e_indoor_job::cloth,
                        e_unit_type::master_weaver );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::tobacco, .quantity = 3 } );
    REQUIRE( pr.cotton_cloth ==
             RMP{
                 .raw_produced                 = 0,
                 .raw_consumed_theoretical     = 6,
                 .raw_delta_theoretical        = -6,
                 .raw_consumed_actual          = 6,
                 .raw_delta_final              = -6,
                 .product_produced_theoretical = 6,
                 .product_produced_actual      = 6,
                 .product_delta_final          = 6,
             } );
  }

  SECTION(
      "no cotton center square/with master weaver, cotton in "
      "store, almost full warehouse for cloth" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.commodities[e_commodity::cotton] = 50;
    colony.commodities[e_commodity::cloth]  = 98;
    W.add_unit_indoors( colony.id, e_indoor_job::cloth,
                        e_unit_type::master_weaver );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::tobacco, .quantity = 3 } );
    REQUIRE( pr.cotton_cloth ==
             RMP{
                 .raw_produced             = 0,
                 .raw_consumed_theoretical = 6,
                 .raw_delta_theoretical    = -6,
                 // No backpressure.
                 .raw_consumed_actual          = 6,
                 .raw_delta_final              = -6,
                 .product_produced_theoretical = 6,
                 .product_produced_actual      = 6,
                 .product_delta_final          = 2,
             } );
  }

  SECTION(
      "no cotton center square/with master weaver, cotton in "
      "store, full warehouse for cloth" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.commodities[e_commodity::cotton] = 50;
    colony.commodities[e_commodity::cloth]  = 100;
    W.add_unit_indoors( colony.id, e_indoor_job::cloth,
                        e_unit_type::master_weaver );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::tobacco, .quantity = 3 } );
    REQUIRE( pr.cotton_cloth ==
             RMP{
                 .raw_produced             = 0,
                 .raw_consumed_theoretical = 6,
                 .raw_delta_theoretical    = -6,
                 // No backpressure.
                 .raw_consumed_actual          = 6,
                 .raw_delta_final              = -6,
                 .product_produced_theoretical = 6,
                 .product_produced_actual      = 6,
                 .product_delta_final          = 0,
             } );
  }

  SECTION(
      "no cotton center square/with master weaver, cotton in "
      "store, over warehouse for cloth" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.commodities[e_commodity::cotton] = 50;
    colony.commodities[e_commodity::cloth]  = 150;
    W.add_unit_indoors( colony.id, e_indoor_job::cloth,
                        e_unit_type::master_weaver );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::tobacco, .quantity = 3 } );
    REQUIRE( pr.cotton_cloth ==
             RMP{
                 .raw_produced             = 0,
                 .raw_consumed_theoretical = 6,
                 .raw_delta_theoretical    = -6,
                 // No backpressure.
                 .raw_consumed_actual          = 6,
                 .raw_delta_final              = -6,
                 .product_produced_theoretical = 6,
                 .product_produced_actual      = 6,
                 .product_delta_final          = 0,
             } );
  }

  SECTION(
      "cotton center square/with master weaver, no cotton in "
      "store, no cloth in store" ) {
    Colony& colony = W.add_colony( W.kPrairieTile );
    colony.commodities[e_commodity::cotton] = 0;
    colony.commodities[e_commodity::cloth]  = 0;
    W.add_unit_indoors( colony.id, e_indoor_job::cloth,
                        e_unit_type::master_weaver );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::cotton, .quantity = 3 } );
    REQUIRE( pr.cotton_cloth ==
             RMP{
                 .raw_produced                 = 3,
                 .raw_consumed_theoretical     = 6,
                 .raw_delta_theoretical        = -3,
                 .raw_consumed_actual          = 3,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 6,
                 .product_produced_actual      = 3,
                 .product_delta_final          = 3,
             } );
  }

  SECTION(
      "cotton center square/with master weaver, no cotton in "
      "store, full cloth warehouse" ) {
    Colony& colony = W.add_colony( W.kPrairieTile );
    colony.commodities[e_commodity::cotton] = 0;
    colony.commodities[e_commodity::cloth]  = 100;
    W.add_unit_indoors( colony.id, e_indoor_job::cloth,
                        e_unit_type::master_weaver );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::cotton, .quantity = 3 } );
    REQUIRE( pr.cotton_cloth ==
             RMP{
                 .raw_produced                 = 3,
                 .raw_consumed_theoretical     = 6,
                 .raw_delta_theoretical        = -3,
                 .raw_consumed_actual          = 3,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 6,
                 .product_produced_actual      = 3,
                 .product_delta_final          = 0,
             } );
  }

  SECTION(
      "cotton center square, one cotton planter, with master "
      "weaver, no cotton in store, no cloth in store, weaver's "
      "shop" ) {
    Colony& colony = W.add_colony( W.kPrairieTile );
    colony.buildings[e_colony_building::weavers_shop] = true;
    colony.commodities[e_commodity::cotton]           = 0;
    colony.commodities[e_commodity::cloth]            = 0;
    W.add_unit_outdoors( colony.id, e_direction::e,
                         e_outdoor_job::cotton,
                         e_unit_type::free_colonist );
    W.add_unit_indoors( colony.id, e_indoor_job::cloth,
                        e_unit_type::master_weaver );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::e, SP{ .what = e_outdoor_job::cotton,
                                  .quantity = 3 } } } );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::cotton, .quantity = 3 } );
    REQUIRE( pr.cotton_cloth ==
             RMP{
                 .raw_produced                 = 6,
                 .raw_consumed_theoretical     = 12,
                 .raw_delta_theoretical        = -6,
                 .raw_consumed_actual          = 6,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 12,
                 .product_produced_actual      = 6,
                 .product_delta_final          = 6,
             } );
  }

  SECTION(
      "cotton center square, one cotton planter, with master "
      "weaver, some cotton in store, no cloth in store, "
      "weaver's shop" ) {
    Colony& colony = W.add_colony( W.kPrairieTile );
    colony.buildings[e_colony_building::weavers_shop] = true;
    colony.commodities[e_commodity::cotton]           = 50;
    colony.commodities[e_commodity::cloth]            = 0;
    W.add_unit_outdoors( colony.id, e_direction::e,
                         e_outdoor_job::cotton,
                         e_unit_type::free_colonist );
    W.add_unit_indoors( colony.id, e_indoor_job::cloth,
                        e_unit_type::master_weaver );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::e, SP{ .what = e_outdoor_job::cotton,
                                  .quantity = 3 } } } );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::cotton, .quantity = 3 } );
    REQUIRE( pr.cotton_cloth ==
             RMP{
                 .raw_produced                 = 6,
                 .raw_consumed_theoretical     = 12,
                 .raw_delta_theoretical        = -6,
                 .raw_consumed_actual          = 12,
                 .raw_delta_final              = -6,
                 .product_produced_theoretical = 12,
                 .product_produced_actual      = 12,
                 .product_delta_final          = 12,
             } );
  }

  SECTION(
      "cotton center square, one cotton planter, with master "
      "weaver, no cotton in store, no cloth in store, "
      "textile_mill" ) {
    Colony& colony = W.add_colony( W.kPrairieTile );
    colony.buildings[e_colony_building::textile_mill] = true;
    colony.commodities[e_commodity::cotton]           = 0;
    colony.commodities[e_commodity::cloth]            = 0;
    W.add_unit_outdoors( colony.id, e_direction::e,
                         e_outdoor_job::cotton,
                         e_unit_type::free_colonist );
    W.add_unit_indoors( colony.id, e_indoor_job::cloth,
                        e_unit_type::master_weaver );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::e, SP{ .what = e_outdoor_job::cotton,
                                  .quantity = 3 } } } );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::cotton, .quantity = 3 } );
    REQUIRE( pr.cotton_cloth ==
             RMP{
                 .raw_produced                 = 6,
                 .raw_consumed_theoretical     = 12,
                 .raw_delta_theoretical        = -6,
                 .raw_consumed_actual          = 6,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 18,
                 .product_produced_actual      = 9,
                 .product_delta_final          = 9,
             } );
  }

  SECTION(
      "cotton center square, one cotton planter, with master "
      "weaver, some cotton in store, no cloth in store, textile "
      "mill" ) {
    Colony& colony = W.add_colony( W.kPrairieTile );
    colony.buildings[e_colony_building::textile_mill] = true;
    colony.commodities[e_commodity::cotton]           = 2;
    colony.commodities[e_commodity::cloth]            = 0;
    W.add_unit_outdoors( colony.id, e_direction::e,
                         e_outdoor_job::cotton,
                         e_unit_type::free_colonist );
    W.add_unit_indoors( colony.id, e_indoor_job::cloth,
                        e_unit_type::master_weaver );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::e, SP{ .what = e_outdoor_job::cotton,
                                  .quantity = 3 } } } );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::cotton, .quantity = 3 } );
    REQUIRE( pr.cotton_cloth ==
             RMP{
                 .raw_produced                 = 6,
                 .raw_consumed_theoretical     = 12,
                 .raw_delta_theoretical        = -6,
                 .raw_consumed_actual          = 8,
                 .raw_delta_final              = -2,
                 .product_produced_theoretical = 18,
                 .product_produced_actual      = 12,
                 .product_delta_final          = 12,
             } );
  }

  SECTION(
      "cotton center square, one expert cotton planter, with "
      "master weaver, no cotton in store, no cloth in store, "
      "textile mill" ) {
    Colony& colony = W.add_colony( W.kPrairieTile );
    colony.buildings[e_colony_building::textile_mill] = true;
    colony.commodities[e_commodity::cotton]           = 0;
    colony.commodities[e_commodity::cloth]            = 0;
    W.add_unit_outdoors( colony.id, e_direction::e,
                         e_outdoor_job::cotton,
                         e_unit_type::expert_cotton_planter );
    W.add_unit_indoors( colony.id, e_indoor_job::cloth,
                        e_unit_type::master_weaver );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::e, SP{ .what = e_outdoor_job::cotton,
                                  .quantity = 6 } } } );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::cotton, .quantity = 3 } );
    REQUIRE( pr.cotton_cloth ==
             RMP{
                 .raw_produced                 = 9,
                 .raw_consumed_theoretical     = 12,
                 .raw_delta_theoretical        = -3,
                 .raw_consumed_actual          = 9,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 18,
                 .product_produced_actual      = 14,
                 .product_delta_final          = 14,
             } );
  }

  SECTION(
      "cotton center square, one expert cotton planter, one "
      "cotton planter, with master weaver, no cotton in store, "
      "no cloth in store, textile mill" ) {
    Colony& colony = W.add_colony( W.kPrairieTile );
    colony.buildings[e_colony_building::textile_mill] = true;
    colony.commodities[e_commodity::cotton]           = 0;
    colony.commodities[e_commodity::cloth]            = 0;
    W.add_unit_outdoors( colony.id, e_direction::e,
                         e_outdoor_job::cotton,
                         e_unit_type::expert_cotton_planter );
    W.add_unit_outdoors( colony.id, e_direction::s,
                         e_outdoor_job::cotton,
                         e_unit_type::petty_criminal );
    W.add_unit_indoors( colony.id, e_indoor_job::cloth,
                        e_unit_type::master_weaver );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::e, SP{ .what = e_outdoor_job::cotton,
                                  .quantity = 6 } },
            { e_direction::s, SP{ .what = e_outdoor_job::cotton,
                                  .quantity = 3 } } } );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::cotton, .quantity = 3 } );
    REQUIRE( pr.cotton_cloth ==
             RMP{
                 .raw_produced                 = 12,
                 .raw_consumed_theoretical     = 12,
                 .raw_delta_theoretical        = 0,
                 .raw_consumed_actual          = 12,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 18,
                 .product_produced_actual      = 18,
                 .product_delta_final          = 18,
             } );
  }

  SECTION(
      "cotton center square, one expert cotton planter, one "
      "cotton planter, with master weaver, full cotton in "
      "store, no cloth in store, textile mill" ) {
    Colony& colony = W.add_colony( W.kPrairieTile );
    colony.buildings[e_colony_building::textile_mill] = true;
    colony.commodities[e_commodity::cotton]           = 100;
    colony.commodities[e_commodity::cloth]            = 0;
    W.add_unit_outdoors( colony.id, e_direction::e,
                         e_outdoor_job::cotton,
                         e_unit_type::expert_cotton_planter );
    W.add_unit_outdoors( colony.id, e_direction::s,
                         e_outdoor_job::cotton,
                         e_unit_type::petty_criminal );
    W.add_unit_indoors( colony.id, e_indoor_job::cloth,
                        e_unit_type::master_weaver );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::e, SP{ .what = e_outdoor_job::cotton,
                                  .quantity = 6 } },
            { e_direction::s, SP{ .what = e_outdoor_job::cotton,
                                  .quantity = 3 } } } );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::cotton, .quantity = 3 } );
    REQUIRE( pr.cotton_cloth ==
             RMP{
                 .raw_produced                 = 12,
                 .raw_consumed_theoretical     = 12,
                 .raw_delta_theoretical        = 0,
                 .raw_consumed_actual          = 12,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 18,
                 .product_produced_actual      = 18,
                 .product_delta_final          = 18,
             } );
  }

  SECTION(
      "cotton center square, one expert cotton planter, one "
      "cotton planter, two master weavers, full cotton in "
      "store, no cloth in store, textile mill" ) {
    Colony& colony = W.add_colony( W.kPrairieTile );
    colony.buildings[e_colony_building::textile_mill] = true;
    colony.commodities[e_commodity::cotton]           = 100;
    colony.commodities[e_commodity::cloth]            = 0;
    W.add_unit_outdoors( colony.id, e_direction::e,
                         e_outdoor_job::cotton,
                         e_unit_type::expert_cotton_planter );
    W.add_unit_outdoors( colony.id, e_direction::s,
                         e_outdoor_job::cotton,
                         e_unit_type::petty_criminal );
    W.add_unit_indoors( colony.id, e_indoor_job::cloth,
                        e_unit_type::master_weaver );
    W.add_unit_indoors( colony.id, e_indoor_job::cloth,
                        e_unit_type::master_weaver );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::e, SP{ .what = e_outdoor_job::cotton,
                                  .quantity = 6 } },
            { e_direction::s, SP{ .what = e_outdoor_job::cotton,
                                  .quantity = 3 } } } );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::cotton, .quantity = 3 } );
    REQUIRE( pr.cotton_cloth ==
             RMP{
                 .raw_produced                 = 12,
                 .raw_consumed_theoretical     = 24,
                 .raw_delta_theoretical        = -12,
                 .raw_consumed_actual          = 24,
                 .raw_delta_final              = -12,
                 .product_produced_theoretical = 36,
                 .product_produced_actual      = 36,
                 .product_delta_final          = 36,
             } );
  }

  SECTION(
      "cotton center square, one expert cotton planter, one "
      "cotton planter, two master weavers, some cotton in "
      "store, no cloth in store, textile mill" ) {
    Colony& colony = W.add_colony( W.kPrairieTile );
    colony.buildings[e_colony_building::textile_mill] = true;
    colony.commodities[e_commodity::cotton]           = 2;
    colony.commodities[e_commodity::cloth]            = 0;
    W.add_unit_outdoors( colony.id, e_direction::e,
                         e_outdoor_job::cotton,
                         e_unit_type::expert_cotton_planter );
    W.add_unit_outdoors( colony.id, e_direction::s,
                         e_outdoor_job::cotton,
                         e_unit_type::petty_criminal );
    W.add_unit_indoors( colony.id, e_indoor_job::cloth,
                        e_unit_type::master_weaver );
    W.add_unit_indoors( colony.id, e_indoor_job::cloth,
                        e_unit_type::master_weaver );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::e, SP{ .what = e_outdoor_job::cotton,
                                  .quantity = 6 } },
            { e_direction::s, SP{ .what = e_outdoor_job::cotton,
                                  .quantity = 3 } } } );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::cotton, .quantity = 3 } );
    REQUIRE( pr.cotton_cloth ==
             RMP{
                 .raw_produced                 = 12,
                 .raw_consumed_theoretical     = 24,
                 .raw_delta_theoretical        = -12,
                 .raw_consumed_actual          = 14,
                 .raw_delta_final              = -2,
                 .product_produced_theoretical = 36,
                 .product_produced_actual      = 21,
                 .product_delta_final          = 21,
             } );
  }

  SECTION(
      "cotton center square, one expert cotton planter, one "
      "cotton planter, two master weavers, some cotton in "
      "store, cloth almost full, textile mill" ) {
    Colony& colony = W.add_colony( W.kPrairieTile );
    colony.buildings[e_colony_building::textile_mill] = true;
    colony.commodities[e_commodity::cotton]           = 2;
    colony.commodities[e_commodity::cloth]            = 90;
    W.add_unit_outdoors( colony.id, e_direction::e,
                         e_outdoor_job::cotton,
                         e_unit_type::expert_cotton_planter );
    W.add_unit_outdoors( colony.id, e_direction::s,
                         e_outdoor_job::cotton,
                         e_unit_type::petty_criminal );
    W.add_unit_indoors( colony.id, e_indoor_job::cloth,
                        e_unit_type::master_weaver );
    W.add_unit_indoors( colony.id, e_indoor_job::cloth,
                        e_unit_type::master_weaver );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::e, SP{ .what = e_outdoor_job::cotton,
                                  .quantity = 6 } },
            { e_direction::s, SP{ .what = e_outdoor_job::cotton,
                                  .quantity = 3 } } } );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::cotton, .quantity = 3 } );
    REQUIRE( pr.cotton_cloth ==
             RMP{
                 .raw_produced                 = 12,
                 .raw_consumed_theoretical     = 24,
                 .raw_delta_theoretical        = -12,
                 .raw_consumed_actual          = 14,
                 .raw_delta_final              = -2,
                 .product_produced_theoretical = 36,
                 .product_produced_actual      = 21,
                 .product_delta_final          = 10,
             } );
  }
}

TEST_CASE( "[production] food/horses [discoverer]" ) {
  World W;
  W.create_default_map();

  using FP = FoodProduction;
  using SP = SquareProduction;
  using LP = refl::enum_map<e_direction, SP>;

  W.settings().difficulty = e_difficulty::discoverer;
  int const bonus         = 2;

  SECTION( "no units no horses/arctic" ) {
    Colony&          colony = W.add_colony( W.kArcticTile );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE( pr.center_food_production == 0 + bonus );
    REQUIRE( pr.food_horses ==
             FP{
                 .corn_produced                          = 2,
                 .fish_produced                          = 0,
                 .food_produced                          = 2,
                 .food_consumed_by_colonists_theoretical = 0,
                 .food_consumed_by_colonists_actual      = 0,
                 .food_deficit                           = 0,
                 .food_surplus_before_horses             = 2,
                 .horses_produced_theoretical            = 0,
                 .max_new_horses_allowed                 = 2,
                 .horses_produced_actual                 = 0,
                 .food_consumed_by_horses                = 0,
                 .horses_delta_final                     = 0,
                 .food_delta_final                       = 2,
                 .colonist_starved                       = false,
             } );
  }

  SECTION( "no units no horses" ) {
    Colony&          colony = W.add_colony( W.kGrasslandTile );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE( pr.center_food_production == 3 + bonus );
    REQUIRE( pr.food_horses ==
             FP{
                 .corn_produced                          = 5,
                 .fish_produced                          = 0,
                 .food_produced                          = 5,
                 .food_consumed_by_colonists_theoretical = 0,
                 .food_consumed_by_colonists_actual      = 0,
                 .food_deficit                           = 0,
                 .food_surplus_before_horses             = 5,
                 .horses_produced_theoretical            = 0,
                 .max_new_horses_allowed                 = 5,
                 .horses_produced_actual                 = 0,
                 .food_consumed_by_horses                = 0,
                 .horses_delta_final                     = 0,
                 .food_delta_final                       = 5,
                 .colonist_starved                       = false,
             } );
  }

  SECTION( "food no warehouse limit" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.commodities[e_commodity::food] = 350;
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE( pr.center_food_production == 3 + bonus );
    REQUIRE( pr.food_horses ==
             FP{
                 .corn_produced                          = 5,
                 .fish_produced                          = 0,
                 .food_produced                          = 5,
                 .food_consumed_by_colonists_theoretical = 0,
                 .food_consumed_by_colonists_actual      = 0,
                 .food_deficit                           = 0,
                 .food_surplus_before_horses             = 5,
                 .horses_produced_theoretical            = 0,
                 .max_new_horses_allowed                 = 5,
                 .horses_produced_actual                 = 0,
                 .food_consumed_by_horses                = 0,
                 .horses_delta_final                     = 0,
                 .food_delta_final                       = 5,
                 .colonist_starved                       = false,
             } );
  }

  SECTION( "one farmer no horses" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    W.add_unit_outdoors( colony.id, e_direction::w,
                         e_outdoor_job::food,
                         e_unit_type::free_colonist );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::w, SP{ .what = e_outdoor_job::food,
                                  .quantity = 3 } } } );
    REQUIRE( pr.center_food_production == 3 + bonus );
    REQUIRE( pr.food_horses ==
             FP{
                 .corn_produced                          = 8,
                 .fish_produced                          = 0,
                 .food_produced                          = 8,
                 .food_consumed_by_colonists_theoretical = 2,
                 .food_consumed_by_colonists_actual      = 2,
                 .food_deficit                           = 0,
                 .food_surplus_before_horses             = 6,
                 .horses_produced_theoretical            = 0,
                 .max_new_horses_allowed                 = 6,
                 .horses_produced_actual                 = 0,
                 .food_consumed_by_horses                = 0,
                 .horses_delta_final                     = 0,
                 .food_delta_final                       = 6,
                 .colonist_starved                       = false,
             } );
  }

  SECTION( "one fisherman no horses" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    W.add_unit_outdoors( colony.id, e_direction::nw,
                         e_outdoor_job::fish,
                         e_unit_type::expert_fisherman );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::nw, SP{ .what = e_outdoor_job::fish,
                                   .quantity = 6 } } } );
    REQUIRE( pr.center_food_production == 3 + bonus );
    REQUIRE( pr.food_horses ==
             FP{
                 .corn_produced                          = 5,
                 .fish_produced                          = 6,
                 .food_produced                          = 11,
                 .food_consumed_by_colonists_theoretical = 2,
                 .food_consumed_by_colonists_actual      = 2,
                 .food_deficit                           = 0,
                 .food_surplus_before_horses             = 9,
                 .horses_produced_theoretical            = 0,
                 .max_new_horses_allowed                 = 9,
                 .horses_produced_actual                 = 0,
                 .food_consumed_by_horses                = 0,
                 .horses_delta_final                     = 0,
                 .food_delta_final                       = 9,
                 .colonist_starved                       = false,
             } );
  }

  SECTION( "one farmer breaking even" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    W.add_unit_outdoors( colony.id, e_direction::w,
                         e_outdoor_job::food,
                         e_unit_type::free_colonist );
    // These are just to consume food.
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::w, SP{ .what = e_outdoor_job::food,
                                  .quantity = 3 } } } );
    REQUIRE( pr.center_food_production == 3 + bonus );
    REQUIRE( pr.food_horses ==
             FP{
                 .corn_produced                          = 8,
                 .fish_produced                          = 0,
                 .food_produced                          = 8,
                 .food_consumed_by_colonists_theoretical = 8,
                 .food_consumed_by_colonists_actual      = 8,
                 .food_deficit                           = 0,
                 .food_surplus_before_horses             = 0,
                 .horses_produced_theoretical            = 0,
                 .max_new_horses_allowed                 = 0,
                 .horses_produced_actual                 = 0,
                 .food_consumed_by_horses                = 0,
                 .horses_delta_final                     = 0,
                 .food_delta_final                       = 0,
                 .colonist_starved                       = false,
             } );
  }

  SECTION( "one farmer breaking even, horses=2" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.commodities[e_commodity::horses] = 2;
    W.add_unit_outdoors( colony.id, e_direction::w,
                         e_outdoor_job::food,
                         e_unit_type::free_colonist );
    // These are just to consume food.
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::w, SP{ .what = e_outdoor_job::food,
                                  .quantity = 3 } } } );
    REQUIRE( pr.center_food_production == 3 + bonus );
    REQUIRE( pr.food_horses ==
             FP{
                 .corn_produced                          = 8,
                 .fish_produced                          = 0,
                 .food_produced                          = 8,
                 .food_consumed_by_colonists_theoretical = 8,
                 .food_consumed_by_colonists_actual      = 8,
                 .food_deficit                           = 0,
                 .food_surplus_before_horses             = 0,
                 .horses_produced_theoretical            = 2,
                 .max_new_horses_allowed                 = 0,
                 .horses_produced_actual                 = 0,
                 .food_consumed_by_horses                = 0,
                 .horses_delta_final                     = 0,
                 .food_delta_final                       = 0,
                 .colonist_starved                       = false,
             } );
  }

  SECTION( "one farmer surplus=1, horses=1" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.commodities[e_commodity::horses] = 1;
    W.add_unit_outdoors( colony.id, e_direction::w,
                         e_outdoor_job::food,
                         e_unit_type::native_convert );
    // These are just to consume food.
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::w, SP{ .what = e_outdoor_job::food,
                                  .quantity = 4 } } } );
    REQUIRE( pr.center_food_production == 3 + bonus );
    REQUIRE( pr.food_horses ==
             FP{
                 .corn_produced                          = 9,
                 .fish_produced                          = 0,
                 .food_produced                          = 9,
                 .food_consumed_by_colonists_theoretical = 8,
                 .food_consumed_by_colonists_actual      = 8,
                 .food_deficit                           = 0,
                 .food_surplus_before_horses             = 1,
                 .horses_produced_theoretical            = 0,
                 .max_new_horses_allowed                 = 1,
                 .horses_produced_actual                 = 0,
                 .food_consumed_by_horses                = 0,
                 .horses_delta_final                     = 0,
                 .food_delta_final                       = 1,
                 .colonist_starved                       = false,
             } );
  }

  SECTION( "one farmer surplus=1, horses=2" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.commodities[e_commodity::horses] = 2;
    W.add_unit_outdoors( colony.id, e_direction::w,
                         e_outdoor_job::food,
                         e_unit_type::native_convert );
    // These are just to consume food.
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::w, SP{ .what = e_outdoor_job::food,
                                  .quantity = 4 } } } );
    REQUIRE( pr.center_food_production == 3 + bonus );
    REQUIRE( pr.food_horses ==
             FP{
                 .corn_produced                          = 9,
                 .fish_produced                          = 0,
                 .food_produced                          = 9,
                 .food_consumed_by_colonists_theoretical = 8,
                 .food_consumed_by_colonists_actual      = 8,
                 .food_deficit                           = 0,
                 .food_surplus_before_horses             = 1,
                 .horses_produced_theoretical            = 2,
                 .max_new_horses_allowed                 = 1,
                 .horses_produced_actual                 = 1,
                 .food_consumed_by_horses                = 1,
                 .horses_delta_final                     = 1,
                 .food_delta_final                       = 0,
                 .colonist_starved                       = false,
             } );
  }

  SECTION( "one fisherman surplus=3, horses=0" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    W.add_unit_outdoors( colony.id, e_direction::nw,
                         e_outdoor_job::fish,
                         e_unit_type::expert_fisherman );
    // These are just to consume food.
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::nw, SP{ .what = e_outdoor_job::fish,
                                   .quantity = 6 } } } );
    REQUIRE( pr.center_food_production == 3 + bonus );
    REQUIRE( pr.food_horses ==
             FP{
                 .corn_produced                          = 5,
                 .fish_produced                          = 6,
                 .food_produced                          = 11,
                 .food_consumed_by_colonists_theoretical = 8,
                 .food_consumed_by_colonists_actual      = 8,
                 .food_deficit                           = 0,
                 .food_surplus_before_horses             = 3,
                 .horses_produced_theoretical            = 0,
                 .max_new_horses_allowed                 = 3,
                 .horses_produced_actual                 = 0,
                 .food_consumed_by_horses                = 0,
                 .horses_delta_final                     = 0,
                 .food_delta_final                       = 3,
                 .colonist_starved                       = false,
             } );
  }

  SECTION( "one farmer surplus=3, horses=50" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.commodities[e_commodity::horses] = 50;
    W.add_unit_outdoors( colony.id, e_direction::w,
                         e_outdoor_job::food,
                         e_unit_type::native_convert );
    // These are just to consume food.
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::w, SP{ .what = e_outdoor_job::food,
                                  .quantity = 4 } } } );
    REQUIRE( pr.center_food_production == 3 + bonus );
    REQUIRE( pr.food_horses ==
             FP{
                 .corn_produced                          = 9,
                 .fish_produced                          = 0,
                 .food_produced                          = 9,
                 .food_consumed_by_colonists_theoretical = 6,
                 .food_consumed_by_colonists_actual      = 6,
                 .food_deficit                           = 0,
                 .food_surplus_before_horses             = 3,
                 .horses_produced_theoretical            = 2,
                 .max_new_horses_allowed                 = 3,
                 .horses_produced_actual                 = 2,
                 .food_consumed_by_horses                = 2,
                 .horses_delta_final                     = 2,
                 .food_delta_final                       = 1,
                 .colonist_starved                       = false,
             } );
  }

  SECTION( "one farmer surplus=3, horses=51" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.commodities[e_commodity::horses] = 51;
    W.add_unit_outdoors( colony.id, e_direction::w,
                         e_outdoor_job::food,
                         e_unit_type::native_convert );
    // These are just to consume food.
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::w, SP{ .what = e_outdoor_job::food,
                                  .quantity = 4 } } } );
    REQUIRE( pr.center_food_production == 3 + bonus );
    REQUIRE( pr.food_horses ==
             FP{
                 .corn_produced                          = 9,
                 .fish_produced                          = 0,
                 .food_produced                          = 9,
                 .food_consumed_by_colonists_theoretical = 6,
                 .food_consumed_by_colonists_actual      = 6,
                 .food_deficit                           = 0,
                 .food_surplus_before_horses             = 3,
                 .horses_produced_theoretical            = 4,
                 .max_new_horses_allowed                 = 3,
                 .horses_produced_actual                 = 3,
                 .food_consumed_by_horses                = 3,
                 .horses_delta_final                     = 3,
                 .food_delta_final                       = 0,
                 .colonist_starved                       = false,
             } );
  }

  SECTION( "one farmer surplus=1, horses=max-2" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.commodities[e_commodity::horses] = 98;
    W.add_unit_outdoors( colony.id, e_direction::w,
                         e_outdoor_job::food,
                         e_unit_type::native_convert );
    // These are just to consume food.
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::w, SP{ .what = e_outdoor_job::food,
                                  .quantity = 4 } } } );
    REQUIRE( pr.center_food_production == 3 + bonus );
    REQUIRE( pr.food_horses ==
             FP{
                 .corn_produced                          = 9,
                 .fish_produced                          = 0,
                 .food_produced                          = 9,
                 .food_consumed_by_colonists_theoretical = 8,
                 .food_consumed_by_colonists_actual      = 8,
                 .food_deficit                           = 0,
                 .food_surplus_before_horses             = 1,
                 .horses_produced_theoretical            = 4,
                 .max_new_horses_allowed                 = 1,
                 .horses_produced_actual                 = 1,
                 .food_consumed_by_horses                = 1,
                 .horses_delta_final                     = 1,
                 .food_delta_final                       = 0,
                 .colonist_starved                       = false,
             } );
  }

  SECTION( "one farmer surplus=3, horses=max-1" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.commodities[e_commodity::horses] = 99;
    W.add_unit_outdoors( colony.id, e_direction::w,
                         e_outdoor_job::food,
                         e_unit_type::native_convert );
    // These are just to consume food.
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::w, SP{ .what = e_outdoor_job::food,
                                  .quantity = 4 } } } );
    REQUIRE( pr.center_food_production == 3 + bonus );
    REQUIRE( pr.food_horses ==
             FP{
                 .corn_produced                          = 9,
                 .fish_produced                          = 0,
                 .food_produced                          = 9,
                 .food_consumed_by_colonists_theoretical = 6,
                 .food_consumed_by_colonists_actual      = 6,
                 .food_deficit                           = 0,
                 .food_surplus_before_horses             = 3,
                 .horses_produced_theoretical            = 4,
                 .max_new_horses_allowed                 = 3,
                 .horses_produced_actual                 = 3,
                 .food_consumed_by_horses                = 3,
                 .horses_delta_final                     = 1,
                 .food_delta_final                       = 0,
                 .colonist_starved                       = false,
             } );
  }

  SECTION( "one farmer surplus=3, horses=max" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.commodities[e_commodity::horses] = 100;
    W.add_unit_outdoors( colony.id, e_direction::w,
                         e_outdoor_job::food,
                         e_unit_type::native_convert );
    // These are just to consume food.
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::w, SP{ .what = e_outdoor_job::food,
                                  .quantity = 4 } } } );
    REQUIRE( pr.center_food_production == 3 + bonus );
    REQUIRE( pr.food_horses ==
             FP{
                 .corn_produced                          = 9,
                 .fish_produced                          = 0,
                 .food_produced                          = 9,
                 .food_consumed_by_colonists_theoretical = 6,
                 .food_consumed_by_colonists_actual      = 6,
                 .food_deficit                           = 0,
                 .food_surplus_before_horses             = 3,
                 .horses_produced_theoretical            = 4,
                 .max_new_horses_allowed                 = 3,
                 .horses_produced_actual                 = 3,
                 .food_consumed_by_horses                = 3,
                 .horses_delta_final                     = 0,
                 .food_delta_final                       = 0,
                 .colonist_starved                       = false,
             } );
  }

  SECTION( "one farmer surplus=3, horses=50, +stable" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.buildings[e_colony_building::stable] = true;
    colony.commodities[e_commodity::horses]     = 50;
    W.add_unit_outdoors( colony.id, e_direction::w,
                         e_outdoor_job::food,
                         e_unit_type::native_convert );
    // These are just to consume food.
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::w, SP{ .what = e_outdoor_job::food,
                                  .quantity = 4 } } } );
    REQUIRE( pr.center_food_production == 3 + bonus );
    REQUIRE( pr.food_horses ==
             FP{
                 .corn_produced                          = 9,
                 .fish_produced                          = 0,
                 .food_produced                          = 9,
                 .food_consumed_by_colonists_theoretical = 6,
                 .food_consumed_by_colonists_actual      = 6,
                 .food_deficit                           = 0,
                 .food_surplus_before_horses             = 3,
                 .horses_produced_theoretical            = 4,
                 .max_new_horses_allowed                 = 3,
                 .horses_produced_actual                 = 3,
                 .food_consumed_by_horses                = 3,
                 .horses_delta_final                     = 3,
                 .food_delta_final                       = 0,
                 .colonist_starved                       = false,
             } );
  }

  SECTION( "one farmer surplus=3, horses=51, +stable" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.buildings[e_colony_building::stable] = true;
    colony.commodities[e_commodity::horses]     = 51;
    W.add_unit_outdoors( colony.id, e_direction::w,
                         e_outdoor_job::food,
                         e_unit_type::native_convert );
    // These are just to consume food.
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::w, SP{ .what = e_outdoor_job::food,
                                  .quantity = 4 } } } );
    REQUIRE( pr.center_food_production == 3 + bonus );
    REQUIRE( pr.food_horses ==
             FP{
                 .corn_produced                          = 9,
                 .fish_produced                          = 0,
                 .food_produced                          = 9,
                 .food_consumed_by_colonists_theoretical = 6,
                 .food_consumed_by_colonists_actual      = 6,
                 .food_deficit                           = 0,
                 .food_surplus_before_horses             = 3,
                 .horses_produced_theoretical            = 8,
                 .max_new_horses_allowed                 = 3,
                 .horses_produced_actual                 = 3,
                 .food_consumed_by_horses                = 3,
                 .horses_delta_final                     = 3,
                 .food_delta_final                       = 0,
                 .colonist_starved                       = false,
             } );
  }

  SECTION( "deficit no starve, horses=0" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.buildings[e_colony_building::stable] = true;
    colony.commodities[e_commodity::horses]     = 0;
    colony.commodities[e_commodity::food]       = 50;
    // These are just to consume food.
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE( pr.center_food_production == 3 + bonus );
    REQUIRE( pr.food_horses ==
             FP{
                 .corn_produced                          = 5,
                 .fish_produced                          = 0,
                 .food_produced                          = 5,
                 .food_consumed_by_colonists_theoretical = 6,
                 .food_consumed_by_colonists_actual      = 6,
                 .food_deficit                           = 0,
                 .food_surplus_before_horses             = 0,
                 .horses_produced_theoretical            = 0,
                 .max_new_horses_allowed                 = 0,
                 .horses_produced_actual                 = 0,
                 .food_consumed_by_horses                = 0,
                 .horses_delta_final                     = 0,
                 .food_delta_final                       = -1,
                 .colonist_starved                       = false,
             } );
  }

  SECTION( "deficit no starve, horses=51" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.commodities[e_commodity::horses] = 51;
    colony.commodities[e_commodity::food]   = 50;
    // These are just to consume food.
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    W.add_unit_indoors( colony.id, e_indoor_job::cigars );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE( pr.center_food_production == 3 + bonus );
    REQUIRE( pr.food_horses ==
             FP{
                 .corn_produced                          = 5,
                 .fish_produced                          = 0,
                 .food_produced                          = 5,
                 .food_consumed_by_colonists_theoretical = 8,
                 .food_consumed_by_colonists_actual      = 8,
                 .food_deficit                           = 0,
                 .food_surplus_before_horses             = 0,
                 .horses_produced_theoretical            = 4,
                 .max_new_horses_allowed                 = 0,
                 .horses_produced_actual                 = 0,
                 .food_consumed_by_horses                = 0,
                 .horses_delta_final                     = 0,
                 .food_delta_final                       = -3,
                 .colonist_starved                       = false,
             } );
  }

  SECTION( "deficit and starve" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    // These are just to consume food.
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    W.add_unit_indoors( colony.id, e_indoor_job::cigars );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE( pr.center_food_production == 3 + bonus );
    REQUIRE( pr.food_horses ==
             FP{
                 .corn_produced                          = 5,
                 .fish_produced                          = 0,
                 .food_produced                          = 5,
                 .food_consumed_by_colonists_theoretical = 8,
                 .food_consumed_by_colonists_actual      = 5,
                 .food_deficit                           = 3,
                 .food_surplus_before_horses             = 0,
                 .horses_produced_theoretical            = 0,
                 .max_new_horses_allowed                 = 0,
                 .horses_produced_actual                 = 0,
                 .food_consumed_by_horses                = 0,
                 .horses_delta_final                     = 0,
                 .food_delta_final                       = 0,
                 .colonist_starved                       = true,
             } );
  }

  SECTION( "deficit and starve, horses=51" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.commodities[e_commodity::horses] = 51;
    // These are just to consume food.
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    W.add_unit_indoors( colony.id, e_indoor_job::cigars );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE( pr.center_food_production == 3 + bonus );
    REQUIRE( pr.food_horses ==
             FP{
                 .corn_produced                          = 5,
                 .fish_produced                          = 0,
                 .food_produced                          = 5,
                 .food_consumed_by_colonists_theoretical = 8,
                 .food_consumed_by_colonists_actual      = 5,
                 .food_deficit                           = 3,
                 .food_surplus_before_horses             = 0,
                 .horses_produced_theoretical            = 4,
                 .max_new_horses_allowed                 = 0,
                 .horses_produced_actual                 = 0,
                 .food_consumed_by_horses                = 0,
                 .horses_delta_final                     = 0,
                 .food_delta_final                       = 0,
                 .colonist_starved                       = true,
             } );
  }

  SECTION( "two farmers, two fisherman, surplus" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    W.add_unit_outdoors( colony.id, e_direction::nw,
                         e_outdoor_job::fish,
                         e_unit_type::expert_fisherman );
    W.add_unit_outdoors( colony.id, e_direction::ne,
                         e_outdoor_job::fish,
                         e_unit_type::free_colonist );
    W.add_unit_outdoors( colony.id, e_direction::w,
                         e_outdoor_job::food,
                         e_unit_type::expert_farmer );
    W.add_unit_outdoors( colony.id, e_direction::e,
                         e_outdoor_job::food,
                         e_unit_type::free_colonist );
    // These are just to consume food.
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::nw,
              SP{ .what = e_outdoor_job::fish, .quantity = 6 } },
            { e_direction::ne,
              SP{ .what = e_outdoor_job::fish, .quantity = 4 } },
            { e_direction::e,
              SP{ .what = e_outdoor_job::food, .quantity = 3 } },
            { e_direction::w, SP{ .what = e_outdoor_job::food,
                                  .quantity = 5 } } } );
    REQUIRE( pr.center_food_production == 3 + bonus );
    REQUIRE( pr.food_horses ==
             FP{
                 .corn_produced                          = 13,
                 .fish_produced                          = 10,
                 .food_produced                          = 23,
                 .food_consumed_by_colonists_theoretical = 14,
                 .food_consumed_by_colonists_actual      = 14,
                 .food_deficit                           = 0,
                 .food_surplus_before_horses             = 9,
                 .horses_produced_theoretical            = 0,
                 .max_new_horses_allowed                 = 9,
                 .horses_produced_actual                 = 0,
                 .food_consumed_by_horses                = 0,
                 .horses_delta_final                     = 0,
                 .food_delta_final                       = 9,
                 .colonist_starved                       = false,
             } );
  }

  SECTION(
      "two farmers, two fisherman, surplus, warehouse, "
      "horses=101, stable" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.buildings[e_colony_building::stable]    = true;
    colony.buildings[e_colony_building::warehouse] = true;
    colony.commodities[e_commodity::horses]        = 101;
    W.add_unit_outdoors( colony.id, e_direction::nw,
                         e_outdoor_job::fish,
                         e_unit_type::expert_fisherman );
    W.add_unit_outdoors( colony.id, e_direction::ne,
                         e_outdoor_job::fish,
                         e_unit_type::free_colonist );
    W.add_unit_outdoors( colony.id, e_direction::w,
                         e_outdoor_job::food,
                         e_unit_type::expert_farmer );
    W.add_unit_outdoors( colony.id, e_direction::e,
                         e_outdoor_job::food,
                         e_unit_type::free_colonist );
    // These are just to consume food.
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::nw,
              SP{ .what = e_outdoor_job::fish, .quantity = 6 } },
            { e_direction::ne,
              SP{ .what = e_outdoor_job::fish, .quantity = 4 } },
            { e_direction::e,
              SP{ .what = e_outdoor_job::food, .quantity = 3 } },
            { e_direction::w, SP{ .what = e_outdoor_job::food,
                                  .quantity = 5 } } } );
    REQUIRE( pr.center_food_production == 3 + bonus );
    REQUIRE( pr.food_horses ==
             FP{
                 .corn_produced                          = 13,
                 .fish_produced                          = 10,
                 .food_produced                          = 23,
                 .food_consumed_by_colonists_theoretical = 10,
                 .food_consumed_by_colonists_actual      = 10,
                 .food_deficit                           = 0,
                 .food_surplus_before_horses             = 13,
                 .horses_produced_theoretical            = 12,
                 .max_new_horses_allowed                 = 13,
                 .horses_produced_actual                 = 12,
                 .food_consumed_by_horses                = 12,
                 .horses_delta_final                     = 12,
                 .food_delta_final                       = 1,
                 .colonist_starved                       = false,
             } );
  }

  SECTION(
      "one farmer, surplus=2, warehouse expansion, horses=200, "
      "+stable" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.buildings[e_colony_building::stable] = true;
    colony.buildings[e_colony_building::warehouse_expansion] =
        true;
    colony.commodities[e_commodity::horses] = 200;
    W.add_unit_outdoors( colony.id, e_direction::e,
                         e_outdoor_job::food,
                         e_unit_type::free_colonist );
    // These are just to consume food.
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::e, SP{ .what = e_outdoor_job::food,
                                  .quantity = 3 } } } );
    REQUIRE( pr.center_food_production == 3 + bonus );
    REQUIRE( pr.food_horses ==
             FP{
                 .corn_produced                          = 8,
                 .fish_produced                          = 0,
                 .food_produced                          = 8,
                 .food_consumed_by_colonists_theoretical = 6,
                 .food_consumed_by_colonists_actual      = 6,
                 .food_deficit                           = 0,
                 .food_surplus_before_horses             = 2,
                 .horses_produced_theoretical            = 16,
                 .max_new_horses_allowed                 = 2,
                 .horses_produced_actual                 = 2,
                 .food_consumed_by_horses                = 2,
                 .horses_delta_final                     = 2,
                 .food_delta_final                       = 0,
                 .colonist_starved                       = false,
             } );
  }
}

// This test case is not meant to be exhaustive, it is just to
// make sure that the center square food production bonus gets
// applied correctly on the explorer difficulty level.
TEST_CASE( "[production] food/horses [explorer]" ) {
  World W;
  W.create_default_map();

  using FP = FoodProduction;
  using SP = SquareProduction;
  using LP = refl::enum_map<e_direction, SP>;

  W.settings().difficulty = e_difficulty::explorer;
  int const bonus         = 1;

  SECTION( "no units no horses/arctic" ) {
    Colony&          colony = W.add_colony( W.kArcticTile );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE( pr.center_food_production == 0 + bonus );
    REQUIRE( pr.food_horses ==
             FP{
                 .corn_produced                          = 1,
                 .fish_produced                          = 0,
                 .food_produced                          = 1,
                 .food_consumed_by_colonists_theoretical = 0,
                 .food_consumed_by_colonists_actual      = 0,
                 .food_deficit                           = 0,
                 .food_surplus_before_horses             = 1,
                 .horses_produced_theoretical            = 0,
                 .max_new_horses_allowed                 = 1,
                 .horses_produced_actual                 = 0,
                 .food_consumed_by_horses                = 0,
                 .horses_delta_final                     = 0,
                 .food_delta_final                       = 1,
                 .colonist_starved                       = false,
             } );
  }

  SECTION(
      "one farmer, surplus=2, warehouse expansion, horses=100, "
      "+stable" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.buildings[e_colony_building::stable] = true;
    colony.buildings[e_colony_building::warehouse_expansion] =
        true;
    colony.commodities[e_commodity::horses] = 100;
    W.add_unit_outdoors( colony.id, e_direction::e,
                         e_outdoor_job::food,
                         e_unit_type::free_colonist );
    // These are just to consume food.
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::e, SP{ .what = e_outdoor_job::food,
                                  .quantity = 3 } } } );
    REQUIRE( pr.center_food_production == 3 + bonus );
    REQUIRE( pr.food_horses ==
             FP{
                 .corn_produced                          = 7,
                 .fish_produced                          = 0,
                 .food_produced                          = 7,
                 .food_consumed_by_colonists_theoretical = 6,
                 .food_consumed_by_colonists_actual      = 6,
                 .food_deficit                           = 0,
                 .food_surplus_before_horses             = 1,
                 .horses_produced_theoretical            = 8,
                 .max_new_horses_allowed                 = 1,
                 .horses_produced_actual                 = 1,
                 .food_consumed_by_horses                = 1,
                 .horses_delta_final                     = 1,
                 .food_delta_final                       = 0,
                 .colonist_starved                       = false,
             } );
  }
}

// This test case is not meant to be exhaustive, it is just to
// make sure that the center square food production bonus gets
// applied correctly on the viceroy difficulty level.
TEST_CASE( "[production] food/horses [viceroy]" ) {
  World W;
  W.create_default_map();

  using FP = FoodProduction;
  using SP = SquareProduction;
  using LP = refl::enum_map<e_direction, SP>;

  W.settings().difficulty = e_difficulty::viceroy;
  int const bonus         = 0;

  SECTION( "no units no horses/arctic" ) {
    Colony&          colony = W.add_colony( W.kArcticTile );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE( pr.center_food_production == 0 + bonus );
    REQUIRE( pr.food_horses ==
             FP{
                 .corn_produced                          = 0,
                 .fish_produced                          = 0,
                 .food_produced                          = 0,
                 .food_consumed_by_colonists_theoretical = 0,
                 .food_consumed_by_colonists_actual      = 0,
                 .food_deficit                           = 0,
                 .food_surplus_before_horses             = 0,
                 .horses_produced_theoretical            = 0,
                 .max_new_horses_allowed                 = 0,
                 .horses_produced_actual                 = 0,
                 .food_consumed_by_horses                = 0,
                 .horses_delta_final                     = 0,
                 .food_delta_final                       = 0,
                 .colonist_starved                       = false,
             } );
  }

  SECTION(
      "one farmer, surplus=2, warehouse expansion, horses=100, "
      "+stable" ) {
    Colony& colony = W.add_colony( W.kGrasslandTile );
    colony.buildings[e_colony_building::stable] = true;
    colony.buildings[e_colony_building::warehouse_expansion] =
        true;
    colony.commodities[e_commodity::horses] = 100;
    W.add_unit_outdoors( colony.id, e_direction::e,
                         e_outdoor_job::food,
                         e_unit_type::free_colonist );
    // These are just to consume food.
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::e, SP{ .what = e_outdoor_job::food,
                                  .quantity = 3 } } } );
    REQUIRE( pr.center_food_production == 3 + bonus );
    REQUIRE( pr.food_horses ==
             FP{
                 .corn_produced                          = 6,
                 .fish_produced                          = 0,
                 .food_produced                          = 6,
                 .food_consumed_by_colonists_theoretical = 6,
                 .food_consumed_by_colonists_actual      = 6,
                 .food_deficit                           = 0,
                 .food_surplus_before_horses             = 0,
                 .horses_produced_theoretical            = 8,
                 .max_new_horses_allowed                 = 0,
                 .horses_produced_actual                 = 0,
                 .food_consumed_by_horses                = 0,
                 .horses_delta_final                     = 0,
                 .food_delta_final                       = 0,
                 .colonist_starved                       = false,
             } );
  }
}

TEST_CASE( "[production] ore/tools/muskets [conquistador]" ) {
  World W;
  W.create_default_map();

  using SP  = SquareProduction;
  using LP  = refl::enum_map<e_direction, SP>;
  using RMP = RawMaterialAndProduct;

  W.settings().difficulty = e_difficulty::conquistador;

  SECTION( "center square ore only" ) {
    Colony&          colony = W.add_colony( W.kDesertTile );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.center_extra_production ==
             SP{ .what = e_outdoor_job::ore, .quantity = 2 } );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE( pr.ore_tools ==
             RMP{
                 .raw_produced                 = 2,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 2,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 2,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
    REQUIRE( pr.tools_muskets ==
             RMP{
                 .raw_produced                 = 0,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 0,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
  }

  SECTION( "center square ore only, almost full warehouse" ) {
    Colony& colony = W.add_colony( W.kDesertTile );
    colony.commodities[e_commodity::ore] = 98;
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.center_extra_production ==
             SP{ .what = e_outdoor_job::ore, .quantity = 2 } );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE( pr.ore_tools ==
             RMP{
                 .raw_produced                 = 2,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 2,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 2,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
    REQUIRE( pr.tools_muskets ==
             RMP{
                 .raw_produced                 = 0,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 0,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
  }

  SECTION( "center square ore only, almost full warehouse" ) {
    Colony& colony = W.add_colony( W.kDesertTile );
    colony.commodities[e_commodity::ore] = 99;
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.center_extra_production ==
             SP{ .what = e_outdoor_job::ore, .quantity = 2 } );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE( pr.ore_tools ==
             RMP{
                 .raw_produced                 = 2,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 2,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 1,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
    REQUIRE( pr.tools_muskets ==
             RMP{
                 .raw_produced                 = 0,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 0,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
  }

  SECTION( "center square ore only, full warehouse" ) {
    Colony& colony = W.add_colony( W.kDesertTile );
    colony.commodities[e_commodity::ore] = 100;
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.center_extra_production ==
             SP{ .what = e_outdoor_job::ore, .quantity = 2 } );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE( pr.ore_tools ==
             RMP{
                 .raw_produced                 = 2,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 2,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
    REQUIRE( pr.tools_muskets ==
             RMP{
                 .raw_produced                 = 0,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 0,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
  }

  SECTION( "center square ore only, over warehouse" ) {
    Colony& colony = W.add_colony( W.kDesertTile );
    colony.commodities[e_commodity::ore] = 150;
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.center_extra_production ==
             SP{ .what = e_outdoor_job::ore, .quantity = 2 } );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE( pr.ore_tools ==
             RMP{
                 .raw_produced                 = 2,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 2,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
    REQUIRE( pr.tools_muskets ==
             RMP{
                 .raw_produced                 = 0,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 0,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
  }

  SECTION( "one ore miner" ) {
    Colony& colony = W.add_colony( W.kDesertTile );
    W.add_unit_outdoors( colony.id, e_direction::e,
                         e_outdoor_job::ore,
                         e_unit_type::free_colonist );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.center_extra_production ==
             SP{ .what = e_outdoor_job::ore, .quantity = 2 } );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::e, SP{ .what     = e_outdoor_job::ore,
                                  .quantity = 2 } } } );
    REQUIRE( pr.ore_tools ==
             RMP{
                 .raw_produced                 = 4,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 4,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 4,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
    REQUIRE( pr.tools_muskets ==
             RMP{
                 .raw_produced                 = 0,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 0,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
  }

  SECTION( "no ore center square" ) {
    Colony& colony =
        W.add_colony( W.kDesertTile - Delta{ .w = 1 } );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::tobacco, .quantity = 3 } );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE( pr.ore_tools ==
             RMP{
                 .raw_produced                 = 0,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 0,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
    REQUIRE( pr.tools_muskets ==
             RMP{
                 .raw_produced                 = 0,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 0,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
  }

  SECTION(
      "no ore center square/with blacksmith, ore in store" ) {
    Colony& colony =
        W.add_colony( W.kDesertTile - Delta{ .w = 1 } );
    colony.commodities[e_commodity::ore] = 2;
    W.add_unit_indoors( colony.id, e_indoor_job::tools,
                        e_unit_type::free_colonist );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::tobacco, .quantity = 3 } );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE( pr.ore_tools ==
             RMP{
                 .raw_produced                 = 0,
                 .raw_consumed_theoretical     = 3,
                 .raw_delta_theoretical        = -3,
                 .raw_consumed_actual          = 2,
                 .raw_delta_final              = -2,
                 .product_produced_theoretical = 3,
                 .product_produced_actual      = 2,
                 .product_delta_final          = 2,
             } );
    REQUIRE( pr.tools_muskets ==
             RMP{
                 .raw_produced                 = 2,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 2,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 2,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
  }

  SECTION(
      "no ore center square/with master blacksmith, some ore in "
      "store" ) {
    Colony& colony =
        W.add_colony( W.kDesertTile - Delta{ .w = 1 } );
    colony.commodities[e_commodity::ore] = 2;
    W.add_unit_indoors( colony.id, e_indoor_job::tools,
                        e_unit_type::master_blacksmith );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::tobacco, .quantity = 3 } );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE( pr.ore_tools ==
             RMP{
                 .raw_produced                 = 0,
                 .raw_consumed_theoretical     = 6,
                 .raw_delta_theoretical        = -6,
                 .raw_consumed_actual          = 2,
                 .raw_delta_final              = -2,
                 .product_produced_theoretical = 6,
                 .product_produced_actual      = 2,
                 .product_delta_final          = 2,
             } );
    REQUIRE( pr.tools_muskets ==
             RMP{
                 .raw_produced                 = 2,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 2,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 2,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
  }

  SECTION(
      "no ore center square/with master blacksmith, some ore in "
      "store, gunsmith" ) {
    Colony& colony =
        W.add_colony( W.kDesertTile - Delta{ .w = 1 } );
    colony.commodities[e_commodity::ore]        = 2;
    colony.buildings[e_colony_building::armory] = true;
    W.add_unit_indoors( colony.id, e_indoor_job::tools,
                        e_unit_type::master_blacksmith );
    W.add_unit_indoors( colony.id, e_indoor_job::muskets,
                        e_unit_type::master_gunsmith );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::tobacco, .quantity = 3 } );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE( pr.ore_tools ==
             RMP{
                 .raw_produced                 = 0,
                 .raw_consumed_theoretical     = 6,
                 .raw_delta_theoretical        = -6,
                 .raw_consumed_actual          = 2,
                 .raw_delta_final              = -2,
                 .product_produced_theoretical = 6,
                 .product_produced_actual      = 2,
                 .product_delta_final          = 0,
             } );
    REQUIRE( pr.tools_muskets ==
             RMP{
                 .raw_produced                 = 2,
                 .raw_consumed_theoretical     = 6,
                 .raw_delta_theoretical        = -4,
                 .raw_consumed_actual          = 2,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 6,
                 .product_produced_actual      = 2,
                 .product_delta_final          = 2,
             } );
  }

  SECTION(
      "no ore center square/with blacksmith, some ore in "
      "store" ) {
    Colony& colony =
        W.add_colony( W.kDesertTile - Delta{ .w = 1 } );
    colony.commodities[e_commodity::ore] = 10;
    W.add_unit_indoors( colony.id, e_indoor_job::tools,
                        e_unit_type::free_colonist );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::tobacco, .quantity = 3 } );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE( pr.ore_tools ==
             RMP{
                 .raw_produced                 = 0,
                 .raw_consumed_theoretical     = 3,
                 .raw_delta_theoretical        = -3,
                 .raw_consumed_actual          = 3,
                 .raw_delta_final              = -3,
                 .product_produced_theoretical = 3,
                 .product_produced_actual      = 3,
                 .product_delta_final          = 3,
             } );
    REQUIRE( pr.tools_muskets ==
             RMP{
                 .raw_produced                 = 3,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 3,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 3,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
  }

  SECTION(
      "no ore center square/with master blacksmith, ore in "
      "store, almost full warehouse for tools" ) {
    Colony& colony =
        W.add_colony( W.kDesertTile - Delta{ .w = 1 } );
    colony.commodities[e_commodity::ore]   = 10;
    colony.commodities[e_commodity::tools] = 98;
    W.add_unit_indoors( colony.id, e_indoor_job::tools,
                        e_unit_type::master_blacksmith );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::tobacco, .quantity = 3 } );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE( pr.ore_tools ==
             RMP{
                 .raw_produced                 = 0,
                 .raw_consumed_theoretical     = 6,
                 .raw_delta_theoretical        = -6,
                 .raw_consumed_actual          = 6,
                 .raw_delta_final              = -6,
                 .product_produced_theoretical = 6,
                 .product_produced_actual      = 6,
                 .product_delta_final          = 2,
             } );
    REQUIRE( pr.tools_muskets ==
             RMP{
                 .raw_produced                 = 6,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 6,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 2,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
  }

  SECTION(
      "no ore center square/with master blacksmith, ore in "
      "store, almost full warehouse for tools, master "
      "gunsmith" ) {
    Colony& colony =
        W.add_colony( W.kDesertTile - Delta{ .w = 1 } );
    colony.commodities[e_commodity::ore]        = 10;
    colony.commodities[e_commodity::tools]      = 98;
    colony.buildings[e_colony_building::armory] = true;
    W.add_unit_indoors( colony.id, e_indoor_job::tools,
                        e_unit_type::master_blacksmith );
    W.add_unit_indoors( colony.id, e_indoor_job::muskets,
                        e_unit_type::master_gunsmith );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::tobacco, .quantity = 3 } );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE( pr.ore_tools ==
             RMP{
                 .raw_produced                 = 0,
                 .raw_consumed_theoretical     = 6,
                 .raw_delta_theoretical        = -6,
                 .raw_consumed_actual          = 6,
                 .raw_delta_final              = -6,
                 .product_produced_theoretical = 6,
                 .product_produced_actual      = 6,
                 .product_delta_final          = 0,
             } );
    REQUIRE( pr.tools_muskets ==
             RMP{
                 .raw_produced                 = 6,
                 .raw_consumed_theoretical     = 6,
                 .raw_delta_theoretical        = 0,
                 .raw_consumed_actual          = 6,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 6,
                 .product_produced_actual      = 6,
                 .product_delta_final          = 6,
             } );
  }

  SECTION(
      "no ore center square/with master blacksmith, ore in "
      "store, full warehouse for tools" ) {
    Colony& colony =
        W.add_colony( W.kDesertTile - Delta{ .w = 1 } );
    colony.commodities[e_commodity::ore]   = 2;
    colony.commodities[e_commodity::tools] = 100;
    W.add_unit_indoors( colony.id, e_indoor_job::tools,
                        e_unit_type::master_blacksmith );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::tobacco, .quantity = 3 } );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE( pr.ore_tools ==
             RMP{
                 .raw_produced                 = 0,
                 .raw_consumed_theoretical     = 6,
                 .raw_delta_theoretical        = -6,
                 .raw_consumed_actual          = 2,
                 .raw_delta_final              = -2,
                 .product_produced_theoretical = 6,
                 .product_produced_actual      = 2,
                 .product_delta_final          = 0,
             } );
    REQUIRE( pr.tools_muskets ==
             RMP{
                 .raw_produced                 = 2,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 2,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
  }

  SECTION(
      "no ore center square/with master blacksmith, ore in "
      "store, full warehouse for tools, gunsmith, no muskets in "
      "store" ) {
    Colony& colony =
        W.add_colony( W.kDesertTile - Delta{ .w = 1 } );
    colony.commodities[e_commodity::ore]        = 2;
    colony.commodities[e_commodity::tools]      = 100;
    colony.buildings[e_colony_building::armory] = true;
    W.add_unit_indoors( colony.id, e_indoor_job::tools,
                        e_unit_type::master_blacksmith );
    W.add_unit_indoors( colony.id, e_indoor_job::muskets,
                        e_unit_type::free_colonist );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::tobacco, .quantity = 3 } );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE( pr.ore_tools ==
             RMP{
                 .raw_produced                 = 0,
                 .raw_consumed_theoretical     = 6,
                 .raw_delta_theoretical        = -6,
                 .raw_consumed_actual          = 2,
                 .raw_delta_final              = -2,
                 .product_produced_theoretical = 6,
                 .product_produced_actual      = 2,
                 .product_delta_final          = -1,
             } );
    REQUIRE( pr.tools_muskets ==
             RMP{
                 .raw_produced                 = 2,
                 .raw_consumed_theoretical     = 3,
                 .raw_delta_theoretical        = -1,
                 .raw_consumed_actual          = 3,
                 .raw_delta_final              = -1,
                 .product_produced_theoretical = 3,
                 .product_produced_actual      = 3,
                 .product_delta_final          = 3,
             } );
  }

  SECTION(
      "no ore center square/with master blacksmith, ore in "
      "store, full warehouse for tools, gunsmith, some muskets "
      "in store" ) {
    Colony& colony =
        W.add_colony( W.kDesertTile - Delta{ .w = 1 } );
    colony.commodities[e_commodity::ore]        = 2;
    colony.commodities[e_commodity::tools]      = 100;
    colony.commodities[e_commodity::muskets]    = 98;
    colony.buildings[e_colony_building::armory] = true;
    W.add_unit_indoors( colony.id, e_indoor_job::tools,
                        e_unit_type::master_blacksmith );
    W.add_unit_indoors( colony.id, e_indoor_job::muskets,
                        e_unit_type::master_gunsmith );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::tobacco, .quantity = 3 } );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE( pr.ore_tools ==
             RMP{
                 .raw_produced                 = 0,
                 .raw_consumed_theoretical     = 6,
                 .raw_delta_theoretical        = -6,
                 .raw_consumed_actual          = 2,
                 .raw_delta_final              = -2,
                 .product_produced_theoretical = 6,
                 .product_produced_actual      = 2,
                 .product_delta_final          = -4,
             } );
    REQUIRE( pr.tools_muskets ==
             RMP{
                 .raw_produced                 = 2,
                 .raw_consumed_theoretical     = 6,
                 .raw_delta_theoretical        = -4,
                 .raw_consumed_actual          = 6,
                 .raw_delta_final              = -4,
                 .product_produced_theoretical = 6,
                 .product_produced_actual      = 6,
                 .product_delta_final          = 2,
             } );
  }

  SECTION(
      "no ore center square/with master blacksmith, ore in "
      "store, full warehouse for tools, gunsmith, full muskets "
      "in store" ) {
    Colony& colony =
        W.add_colony( W.kDesertTile - Delta{ .w = 1 } );
    colony.commodities[e_commodity::ore]        = 10;
    colony.commodities[e_commodity::tools]      = 100;
    colony.commodities[e_commodity::muskets]    = 100;
    colony.buildings[e_colony_building::armory] = true;
    W.add_unit_indoors( colony.id, e_indoor_job::tools,
                        e_unit_type::master_blacksmith );
    W.add_unit_indoors( colony.id, e_indoor_job::muskets,
                        e_unit_type::free_colonist );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::tobacco, .quantity = 3 } );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE( pr.ore_tools ==
             RMP{
                 .raw_produced                 = 0,
                 .raw_consumed_theoretical     = 6,
                 .raw_delta_theoretical        = -6,
                 .raw_consumed_actual          = 6,
                 .raw_delta_final              = -6,
                 .product_produced_theoretical = 6,
                 .product_produced_actual      = 6,
                 .product_delta_final          = 0,
             } );
    REQUIRE( pr.tools_muskets ==
             RMP{
                 .raw_produced                 = 6,
                 .raw_consumed_theoretical     = 3,
                 .raw_delta_theoretical        = 3,
                 .raw_consumed_actual          = 3,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 3,
                 .product_produced_actual      = 3,
                 .product_delta_final          = 0,
             } );
  }

  SECTION(
      "no ore center square/with master blacksmith, ore in "
      "store, over warehouse for tools" ) {
    Colony& colony =
        W.add_colony( W.kDesertTile - Delta{ .w = 1 } );
    colony.commodities[e_commodity::ore]   = 2;
    colony.commodities[e_commodity::tools] = 150;
    W.add_unit_indoors( colony.id, e_indoor_job::tools,
                        e_unit_type::master_blacksmith );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::tobacco, .quantity = 3 } );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE( pr.ore_tools ==
             RMP{
                 .raw_produced                 = 0,
                 .raw_consumed_theoretical     = 6,
                 .raw_delta_theoretical        = -6,
                 .raw_consumed_actual          = 2,
                 .raw_delta_final              = -2,
                 .product_produced_theoretical = 6,
                 .product_produced_actual      = 2,
                 .product_delta_final          = 0,
             } );
    REQUIRE( pr.tools_muskets ==
             RMP{
                 .raw_produced                 = 2,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 2,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
  }

  SECTION(
      "no ore center square/with master blacksmith, ore in "
      "store, over warehouse for tools, gunsmith, no guns in "
      "store" ) {
    Colony& colony =
        W.add_colony( W.kDesertTile - Delta{ .w = 1 } );
    colony.commodities[e_commodity::ore]        = 2;
    colony.commodities[e_commodity::tools]      = 150;
    colony.buildings[e_colony_building::armory] = true;
    W.add_unit_indoors( colony.id, e_indoor_job::tools,
                        e_unit_type::master_blacksmith );
    W.add_unit_indoors( colony.id, e_indoor_job::muskets,
                        e_unit_type::master_gunsmith );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::tobacco, .quantity = 3 } );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE( pr.ore_tools ==
             RMP{
                 .raw_produced                 = 0,
                 .raw_consumed_theoretical     = 6,
                 .raw_delta_theoretical        = -6,
                 .raw_consumed_actual          = 2,
                 .raw_delta_final              = -2,
                 .product_produced_theoretical = 6,
                 .product_produced_actual      = 2,
                 .product_delta_final          = -4,
             } );
    REQUIRE( pr.tools_muskets ==
             RMP{
                 .raw_produced                 = 2,
                 .raw_consumed_theoretical     = 6,
                 .raw_delta_theoretical        = -4,
                 .raw_consumed_actual          = 6,
                 .raw_delta_final              = -4,
                 .product_produced_theoretical = 6,
                 .product_produced_actual      = 6,
                 .product_delta_final          = 6,
             } );
  }

  SECTION(
      "no ore center square/with master blacksmith, ore in "
      "store, over warehouse for tools, gunsmith, some guns in "
      "store" ) {
    Colony& colony =
        W.add_colony( W.kDesertTile - Delta{ .w = 1 } );
    colony.commodities[e_commodity::ore]        = 2;
    colony.commodities[e_commodity::tools]      = 150;
    colony.commodities[e_commodity::muskets]    = 95;
    colony.buildings[e_colony_building::armory] = true;
    W.add_unit_indoors( colony.id, e_indoor_job::tools,
                        e_unit_type::master_blacksmith );
    W.add_unit_indoors( colony.id, e_indoor_job::muskets,
                        e_unit_type::master_gunsmith );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::tobacco, .quantity = 3 } );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE( pr.ore_tools ==
             RMP{
                 .raw_produced                 = 0,
                 .raw_consumed_theoretical     = 6,
                 .raw_delta_theoretical        = -6,
                 .raw_consumed_actual          = 2,
                 .raw_delta_final              = -2,
                 .product_produced_theoretical = 6,
                 .product_produced_actual      = 2,
                 .product_delta_final          = -4,
             } );
    REQUIRE( pr.tools_muskets ==
             RMP{
                 .raw_produced                 = 2,
                 .raw_consumed_theoretical     = 6,
                 .raw_delta_theoretical        = -4,
                 .raw_consumed_actual          = 6,
                 .raw_delta_final              = -4,
                 .product_produced_theoretical = 6,
                 .product_produced_actual      = 6,
                 .product_delta_final          = 5,
             } );
  }

  SECTION(
      "no ore center square/with master blacksmith, ore in "
      "store, over warehouse for tools, gunsmith, full muskets "
      "in store" ) {
    Colony& colony =
        W.add_colony( W.kDesertTile - Delta{ .w = 1 } );
    colony.commodities[e_commodity::ore]        = 2;
    colony.commodities[e_commodity::tools]      = 150;
    colony.commodities[e_commodity::muskets]    = 100;
    colony.buildings[e_colony_building::armory] = true;
    W.add_unit_indoors( colony.id, e_indoor_job::tools,
                        e_unit_type::master_blacksmith );
    W.add_unit_indoors( colony.id, e_indoor_job::muskets,
                        e_unit_type::master_gunsmith );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::tobacco, .quantity = 3 } );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE( pr.ore_tools ==
             RMP{
                 .raw_produced                 = 0,
                 .raw_consumed_theoretical     = 6,
                 .raw_delta_theoretical        = -6,
                 .raw_consumed_actual          = 2,
                 .raw_delta_final              = -2,
                 .product_produced_theoretical = 6,
                 .product_produced_actual      = 2,
                 .product_delta_final          = -4,
             } );
    REQUIRE( pr.tools_muskets ==
             RMP{
                 .raw_produced                 = 2,
                 .raw_consumed_theoretical     = 6,
                 .raw_delta_theoretical        = -4,
                 .raw_consumed_actual          = 6,
                 .raw_delta_final              = -4,
                 .product_produced_theoretical = 6,
                 .product_produced_actual      = 6,
                 .product_delta_final          = 0,
             } );
  }

  SECTION(
      "ore center square/with master blacksmith, no ore in "
      "store, no tools in store" ) {
    Colony& colony = W.add_colony( W.kDesertTile );
    W.add_unit_indoors( colony.id, e_indoor_job::tools,
                        e_unit_type::master_blacksmith );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.center_extra_production ==
             SP{ .what = e_outdoor_job::ore, .quantity = 2 } );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE( pr.ore_tools ==
             RMP{
                 .raw_produced                 = 2,
                 .raw_consumed_theoretical     = 6,
                 .raw_delta_theoretical        = -4,
                 .raw_consumed_actual          = 2,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 6,
                 .product_produced_actual      = 2,
                 .product_delta_final          = 2,
             } );
    REQUIRE( pr.tools_muskets ==
             RMP{
                 .raw_produced                 = 2,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 2,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 2,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
  }

  SECTION(
      "ore center square/with master gunsmith, no ore in "
      "store, no tools in store" ) {
    Colony& colony = W.add_colony( W.kDesertTile );
    colony.buildings[e_colony_building::armory] = true;
    W.add_unit_indoors( colony.id, e_indoor_job::muskets,
                        e_unit_type::master_gunsmith );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.center_extra_production ==
             SP{ .what = e_outdoor_job::ore, .quantity = 2 } );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE( pr.ore_tools ==
             RMP{
                 .raw_produced                 = 2,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 2,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 2,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
    REQUIRE( pr.tools_muskets ==
             RMP{
                 .raw_produced                 = 0,
                 .raw_consumed_theoretical     = 6,
                 .raw_delta_theoretical        = -6,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 6,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
  }

  SECTION(
      "no ore center square/with gunsmith and blacksmith, no "
      "ore in store, no tools in store" ) {
    Colony& colony =
        W.add_colony( W.kDesertTile - Delta{ .w = 1 } );
    colony.buildings[e_colony_building::armory] = true;
    W.add_unit_indoors( colony.id, e_indoor_job::tools,
                        e_unit_type::master_blacksmith );
    W.add_unit_indoors( colony.id, e_indoor_job::muskets,
                        e_unit_type::master_gunsmith );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::tobacco, .quantity = 3 } );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE( pr.ore_tools ==
             RMP{
                 .raw_produced                 = 0,
                 .raw_consumed_theoretical     = 6,
                 .raw_delta_theoretical        = -6,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 6,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
    REQUIRE( pr.tools_muskets ==
             RMP{
                 .raw_produced                 = 0,
                 .raw_consumed_theoretical     = 6,
                 .raw_delta_theoretical        = -6,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 6,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
  }

  SECTION(
      "no ore center square/with gunsmith and blacksmith, no "
      "ore in store, some tools in store" ) {
    Colony& colony =
        W.add_colony( W.kDesertTile - Delta{ .w = 1 } );
    colony.buildings[e_colony_building::armory] = true;
    colony.commodities[e_commodity::tools]      = 30;
    W.add_unit_indoors( colony.id, e_indoor_job::tools,
                        e_unit_type::master_blacksmith );
    W.add_unit_indoors( colony.id, e_indoor_job::muskets,
                        e_unit_type::master_gunsmith );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE(
        pr.center_extra_production ==
        SP{ .what = e_outdoor_job::tobacco, .quantity = 3 } );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE( pr.ore_tools ==
             RMP{
                 .raw_produced                 = 0,
                 .raw_consumed_theoretical     = 6,
                 .raw_delta_theoretical        = -6,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 6,
                 .product_produced_actual      = 0,
                 .product_delta_final          = -6,
             } );
    REQUIRE( pr.tools_muskets ==
             RMP{
                 .raw_produced                 = 0,
                 .raw_consumed_theoretical     = 6,
                 .raw_delta_theoretical        = -6,
                 .raw_consumed_actual          = 6,
                 .raw_delta_final              = -6,
                 .product_produced_theoretical = 6,
                 .product_produced_actual      = 6,
                 .product_delta_final          = 6,
             } );
  }

  SECTION(
      "ore center square/with master blacksmith, no ore in "
      "store, full tools warehouse" ) {
    Colony& colony = W.add_colony( W.kDesertTile );
    colony.commodities[e_commodity::tools] = 100;
    W.add_unit_indoors( colony.id, e_indoor_job::tools,
                        e_unit_type::master_blacksmith );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.center_extra_production ==
             SP{ .what = e_outdoor_job::ore, .quantity = 2 } );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE( pr.ore_tools ==
             RMP{
                 .raw_produced                 = 2,
                 .raw_consumed_theoretical     = 6,
                 .raw_delta_theoretical        = -4,
                 .raw_consumed_actual          = 2,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 6,
                 .product_produced_actual      = 2,
                 .product_delta_final          = 0,
             } );
    REQUIRE( pr.tools_muskets ==
             RMP{
                 .raw_produced                 = 2,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 2,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
  }

  SECTION(
      "ore center square/with master blacksmith and gunsmith, "
      "no ore in store, full tools warehouse" ) {
    Colony& colony = W.add_colony( W.kDesertTile );
    colony.buildings[e_colony_building::armory] = true;
    colony.commodities[e_commodity::tools]      = 100;
    W.add_unit_indoors( colony.id, e_indoor_job::tools,
                        e_unit_type::master_blacksmith );
    W.add_unit_indoors( colony.id, e_indoor_job::muskets,
                        e_unit_type::free_colonist );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.center_extra_production ==
             SP{ .what = e_outdoor_job::ore, .quantity = 2 } );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE( pr.ore_tools ==
             RMP{
                 .raw_produced                 = 2,
                 .raw_consumed_theoretical     = 6,
                 .raw_delta_theoretical        = -4,
                 .raw_consumed_actual          = 2,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 6,
                 .product_produced_actual      = 2,
                 .product_delta_final          = -1,
             } );
    REQUIRE( pr.tools_muskets ==
             RMP{
                 .raw_produced                 = 2,
                 .raw_consumed_theoretical     = 3,
                 .raw_delta_theoretical        = -1,
                 .raw_consumed_actual          = 3,
                 .raw_delta_final              = -1,
                 .product_produced_theoretical = 3,
                 .product_produced_actual      = 3,
                 .product_delta_final          = 3,
             } );
  }

  SECTION(
      "ore center square/with master blacksmith and gunsmith, "
      "no ore in store, full tools and muskets warehouse" ) {
    Colony& colony = W.add_colony( W.kDesertTile );
    colony.commodities[e_commodity::tools]      = 100;
    colony.commodities[e_commodity::muskets]    = 100;
    colony.buildings[e_colony_building::armory] = true;
    W.add_unit_indoors( colony.id, e_indoor_job::tools,
                        e_unit_type::master_blacksmith );
    W.add_unit_indoors( colony.id, e_indoor_job::muskets,
                        e_unit_type::free_colonist );
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.center_extra_production ==
             SP{ .what = e_outdoor_job::ore, .quantity = 2 } );
    REQUIRE( pr.land_production == LP{} );
    REQUIRE( pr.ore_tools ==
             RMP{
                 .raw_produced                 = 2,
                 .raw_consumed_theoretical     = 6,
                 .raw_delta_theoretical        = -4,
                 .raw_consumed_actual          = 2,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 6,
                 .product_produced_actual      = 2,
                 .product_delta_final          = -1,
             } );
    REQUIRE( pr.tools_muskets ==
             RMP{
                 .raw_produced                 = 2,
                 .raw_consumed_theoretical     = 3,
                 .raw_delta_theoretical        = -1,
                 .raw_consumed_actual          = 3,
                 .raw_delta_final              = -1,
                 .product_produced_theoretical = 3,
                 .product_produced_actual      = 3,
                 .product_delta_final          = 0,
             } );
  }

  SECTION(
      "ore center square, one ore miner, with master "
      "blacksmith, no ore in store, no tools in store, "
      "blacksmith's shop" ) {
    Colony& colony = W.add_colony( W.kDesertTile );
    W.add_unit_indoors( colony.id, e_indoor_job::tools,
                        e_unit_type::master_blacksmith );
    W.add_unit_outdoors( colony.id, e_direction::e,
                         e_outdoor_job::ore,
                         e_unit_type::free_colonist );
    colony.buildings[e_colony_building::blacksmiths_shop] = true;
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.center_extra_production ==
             SP{ .what = e_outdoor_job::ore, .quantity = 2 } );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::e, SP{ .what     = e_outdoor_job::ore,
                                  .quantity = 2 } } } );
    REQUIRE( pr.ore_tools ==
             RMP{
                 .raw_produced                 = 4,
                 .raw_consumed_theoretical     = 12,
                 .raw_delta_theoretical        = -8,
                 .raw_consumed_actual          = 4,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 12,
                 .product_produced_actual      = 4,
                 .product_delta_final          = 4,
             } );
    REQUIRE( pr.tools_muskets ==
             RMP{
                 .raw_produced                 = 4,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 4,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 4,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
  }

  SECTION(
      "ore center square, one ore miner, with master blacksmith "
      "and gunsmith, some ore in store, no tools in store, "
      "blacksmith's shop, magazine" ) {
    Colony& colony = W.add_colony( W.kDesertTile );
    colony.commodities[e_commodity::ore] = 40;
    W.add_unit_indoors( colony.id, e_indoor_job::tools,
                        e_unit_type::master_blacksmith );
    W.add_unit_indoors( colony.id, e_indoor_job::muskets,
                        e_unit_type::free_colonist );
    W.add_unit_outdoors( colony.id, e_direction::e,
                         e_outdoor_job::ore,
                         e_unit_type::free_colonist );
    colony.buildings[e_colony_building::blacksmiths_shop] = true;
    colony.buildings[e_colony_building::magazine]         = true;
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.center_extra_production ==
             SP{ .what = e_outdoor_job::ore, .quantity = 2 } );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::e, SP{ .what     = e_outdoor_job::ore,
                                  .quantity = 2 } } } );
    REQUIRE( pr.ore_tools ==
             RMP{
                 .raw_produced                 = 4,
                 .raw_consumed_theoretical     = 12,
                 .raw_delta_theoretical        = -8,
                 .raw_consumed_actual          = 12,
                 .raw_delta_final              = -8,
                 .product_produced_theoretical = 12,
                 .product_produced_actual      = 12,
                 .product_delta_final          = 6,
             } );
    REQUIRE( pr.tools_muskets ==
             RMP{
                 .raw_produced                 = 12,
                 .raw_consumed_theoretical     = 6,
                 .raw_delta_theoretical        = 6,
                 .raw_consumed_actual          = 6,
                 .raw_delta_final              = 6,
                 .product_produced_theoretical = 6,
                 .product_produced_actual      = 6,
                 .product_delta_final          = 6,
             } );
  }

  SECTION(
      "ore center square, one ore miner, with master "
      "blacksmith, some ore in store, no tools in store, "
      "blacksmith's shop" ) {
    Colony& colony = W.add_colony( W.kDesertTile );
    colony.commodities[e_commodity::ore] = 8;
    W.add_unit_indoors( colony.id, e_indoor_job::tools,
                        e_unit_type::master_blacksmith );
    W.add_unit_outdoors( colony.id, e_direction::e,
                         e_outdoor_job::ore,
                         e_unit_type::expert_ore_miner );
    colony.buildings[e_colony_building::blacksmiths_shop] = true;
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.center_extra_production ==
             SP{ .what = e_outdoor_job::ore, .quantity = 2 } );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::e, SP{ .what     = e_outdoor_job::ore,
                                  .quantity = 4 } } } );
    REQUIRE( pr.ore_tools ==
             RMP{
                 .raw_produced                 = 6,
                 .raw_consumed_theoretical     = 12,
                 .raw_delta_theoretical        = -6,
                 .raw_consumed_actual          = 12,
                 .raw_delta_final              = -6,
                 .product_produced_theoretical = 12,
                 .product_produced_actual      = 12,
                 .product_delta_final          = 12,
             } );
    REQUIRE( pr.tools_muskets ==
             RMP{
                 .raw_produced                 = 12,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 12,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 12,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
  }

  SECTION(
      "ore center square, one ore miner, with master "
      "blacksmith, master gunsmith, no ore in store, some "
      "tools in store, blacksmith's shop, arsenal" ) {
    Colony& colony = W.add_colony( W.kDesertTile );
    colony.commodities[e_commodity::tools] = 8;
    W.add_unit_indoors( colony.id, e_indoor_job::tools,
                        e_unit_type::master_blacksmith );
    W.add_unit_indoors( colony.id, e_indoor_job::muskets,
                        e_unit_type::master_gunsmith );
    W.add_unit_outdoors( colony.id, e_direction::e,
                         e_outdoor_job::ore,
                         e_unit_type::free_colonist );
    colony.buildings[e_colony_building::blacksmiths_shop] = true;
    colony.buildings[e_colony_building::arsenal]          = true;
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.center_extra_production ==
             SP{ .what = e_outdoor_job::ore, .quantity = 2 } );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::e, SP{ .what     = e_outdoor_job::ore,
                                  .quantity = 2 } } } );
    REQUIRE( pr.ore_tools ==
             RMP{
                 .raw_produced                 = 4,
                 .raw_consumed_theoretical     = 12,
                 .raw_delta_theoretical        = -8,
                 .raw_consumed_actual          = 4,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 12,
                 .product_produced_actual      = 4,
                 .product_delta_final          = -8,
             } );
    REQUIRE( pr.tools_muskets ==
             RMP{
                 .raw_produced                 = 4,
                 .raw_consumed_theoretical     = 12,
                 .raw_delta_theoretical        = -8,
                 .raw_consumed_actual          = 12,
                 .raw_delta_final              = -8,
                 .product_produced_theoretical = 18,
                 .product_produced_actual      = 18,
                 .product_delta_final          = 18,
             } );
  }

  SECTION(
      "ore center square, one ore miner, with master "
      "blacksmith, no ore in store, some tools in store, iron "
      "works" ) {
    Colony& colony = W.add_colony( W.kDesertTile );
    colony.commodities[e_commodity::tools] = 3;
    W.add_unit_indoors( colony.id, e_indoor_job::tools,
                        e_unit_type::master_blacksmith );
    W.add_unit_outdoors( colony.id, e_direction::e,
                         e_outdoor_job::ore,
                         e_unit_type::free_colonist );
    colony.buildings[e_colony_building::iron_works] = true;
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.center_extra_production ==
             SP{ .what = e_outdoor_job::ore, .quantity = 2 } );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::e, SP{ .what     = e_outdoor_job::ore,
                                  .quantity = 2 } } } );
    REQUIRE( pr.ore_tools ==
             RMP{
                 .raw_produced                 = 4,
                 .raw_consumed_theoretical     = 12,
                 .raw_delta_theoretical        = -8,
                 .raw_consumed_actual          = 4,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 18,
                 .product_produced_actual      = 6,
                 .product_delta_final          = 6,
             } );
    REQUIRE( pr.tools_muskets ==
             RMP{
                 .raw_produced                 = 6,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 6,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 6,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
  }

  SECTION(
      "ore center square, one ore miner, with master "
      "blacksmith, master gunsmith, no ore in store, some tools "
      "in store, iron works, arsenal" ) {
    Colony& colony = W.add_colony( W.kDesertTile );
    colony.commodities[e_commodity::tools] = 3;
    W.add_unit_indoors( colony.id, e_indoor_job::tools,
                        e_unit_type::master_blacksmith );
    W.add_unit_indoors( colony.id, e_indoor_job::muskets,
                        e_unit_type::master_gunsmith );
    W.add_unit_outdoors( colony.id, e_direction::e,
                         e_outdoor_job::ore,
                         e_unit_type::free_colonist );
    colony.buildings[e_colony_building::iron_works] = true;
    colony.buildings[e_colony_building::arsenal]    = true;
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.center_extra_production ==
             SP{ .what = e_outdoor_job::ore, .quantity = 2 } );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::e, SP{ .what     = e_outdoor_job::ore,
                                  .quantity = 2 } } } );
    REQUIRE( pr.ore_tools ==
             RMP{
                 .raw_produced                 = 4,
                 .raw_consumed_theoretical     = 12,
                 .raw_delta_theoretical        = -8,
                 .raw_consumed_actual          = 4,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 18,
                 .product_produced_actual      = 6,
                 .product_delta_final          = -3,
             } );
    REQUIRE( pr.tools_muskets ==
             RMP{
                 .raw_produced                 = 6,
                 .raw_consumed_theoretical     = 12,
                 .raw_delta_theoretical        = -6,
                 .raw_consumed_actual          = 9,
                 .raw_delta_final              = -3,
                 .product_produced_theoretical = 18,
                 .product_produced_actual      = 14,
                 .product_delta_final          = 14,
             } );
  }

  SECTION(
      "ore center square, one ore miner, with IS blacksmith, "
      "no ore in store, no tools in store, iron works" ) {
    Colony& colony = W.add_colony( W.kDesertTile );
    W.add_unit_indoors( colony.id, e_indoor_job::tools,
                        e_unit_type::indentured_servant );
    W.add_unit_outdoors( colony.id, e_direction::e,
                         e_outdoor_job::ore,
                         e_unit_type::free_colonist );
    colony.buildings[e_colony_building::iron_works] = true;
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.center_extra_production ==
             SP{ .what = e_outdoor_job::ore, .quantity = 2 } );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::e, SP{ .what     = e_outdoor_job::ore,
                                  .quantity = 2 } } } );
    REQUIRE( pr.ore_tools ==
             RMP{
                 .raw_produced                 = 4,
                 .raw_consumed_theoretical     = 4,
                 .raw_delta_theoretical        = 0,
                 .raw_consumed_actual          = 4,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 6,
                 .product_produced_actual      = 6,
                 .product_delta_final          = 6,
             } );
    REQUIRE( pr.tools_muskets ==
             RMP{
                 .raw_produced                 = 6,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 6,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 6,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
  }

  SECTION(
      "ore center square, one ore miner, with master gunsmith, "
      "some ore in store, some tools in store, magazine" ) {
    Colony& colony = W.add_colony( W.kDesertTile );
    colony.commodities[e_commodity::ore]   = 8;
    colony.commodities[e_commodity::tools] = 1;
    W.add_unit_indoors( colony.id, e_indoor_job::muskets,
                        e_unit_type::master_gunsmith );
    W.add_unit_outdoors( colony.id, e_direction::e,
                         e_outdoor_job::ore,
                         e_unit_type::expert_ore_miner );
    colony.buildings[e_colony_building::magazine] = true;
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.center_extra_production ==
             SP{ .what = e_outdoor_job::ore, .quantity = 2 } );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::e, SP{ .what     = e_outdoor_job::ore,
                                  .quantity = 4 } } } );
    REQUIRE( pr.ore_tools ==
             RMP{
                 .raw_produced                 = 6,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 6,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 6,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = -1,
             } );
    REQUIRE( pr.tools_muskets ==
             RMP{
                 .raw_produced                 = 0,
                 .raw_consumed_theoretical     = 12,
                 .raw_delta_theoretical        = -12,
                 .raw_consumed_actual          = 1,
                 .raw_delta_final              = -1,
                 .product_produced_theoretical = 12,
                 .product_produced_actual      = 1,
                 .product_delta_final          = 1,
             } );
  }

  SECTION(
      "ore center square, one ore miner, with master gunsmith, "
      "some ore in store, some tools in store, arsenal" ) {
    Colony& colony = W.add_colony( W.kDesertTile );
    colony.commodities[e_commodity::ore]   = 8;
    colony.commodities[e_commodity::tools] = 1;
    W.add_unit_indoors( colony.id, e_indoor_job::muskets,
                        e_unit_type::master_gunsmith );
    W.add_unit_outdoors( colony.id, e_direction::e,
                         e_outdoor_job::ore,
                         e_unit_type::expert_ore_miner );
    colony.buildings[e_colony_building::arsenal] = true;
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.center_extra_production ==
             SP{ .what = e_outdoor_job::ore, .quantity = 2 } );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::e, SP{ .what     = e_outdoor_job::ore,
                                  .quantity = 4 } } } );
    REQUIRE( pr.ore_tools ==
             RMP{
                 .raw_produced                 = 6,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 6,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 6,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = -1,
             } );
    REQUIRE( pr.tools_muskets ==
             RMP{
                 .raw_produced                 = 0,
                 .raw_consumed_theoretical     = 12,
                 .raw_delta_theoretical        = -12,
                 .raw_consumed_actual          = 1,
                 .raw_delta_final              = -1,
                 .product_produced_theoretical = 18,
                 .product_produced_actual      = 2,
                 .product_delta_final          = 2,
             } );
  }

  SECTION(
      "ore center square, one expert ore miner, one ore miner, "
      "with one master blacksmiths, one blacksmith, full ore in "
      "store, no tools in store, iron works" ) {
    Colony& colony = W.add_colony( W.kDesertTile );
    colony.commodities[e_commodity::ore] = 100;
    W.add_unit_indoors( colony.id, e_indoor_job::tools,
                        e_unit_type::master_blacksmith );
    W.add_unit_indoors( colony.id, e_indoor_job::tools,
                        e_unit_type::free_colonist );
    W.add_unit_outdoors( colony.id, e_direction::e,
                         e_outdoor_job::ore,
                         e_unit_type::free_colonist );
    W.add_unit_outdoors( colony.id, e_direction::s,
                         e_outdoor_job::ore,
                         e_unit_type::expert_ore_miner );
    colony.buildings[e_colony_building::iron_works] = true;
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.center_extra_production ==
             SP{ .what = e_outdoor_job::ore, .quantity = 2 } );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::e,
              SP{ .what = e_outdoor_job::ore, .quantity = 2 } },
            { e_direction::s, SP{ .what     = e_outdoor_job::ore,
                                  .quantity = 4 } } } );
    REQUIRE( pr.ore_tools ==
             RMP{
                 .raw_produced                 = 8,
                 .raw_consumed_theoretical     = 18,
                 .raw_delta_theoretical        = -10,
                 .raw_consumed_actual          = 18,
                 .raw_delta_final              = -10,
                 .product_produced_theoretical = 27,
                 .product_produced_actual      = 27,
                 .product_delta_final          = 27,
             } );
    REQUIRE( pr.tools_muskets ==
             RMP{
                 .raw_produced                 = 27,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 27,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 27,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
  }

  SECTION(
      "ore center square, one expert ore miner, one ore miner, "
      "with master blacksmith, one master gunsmith, full ore in "
      "store, full tools in store, iron works, armory" ) {
    Colony& colony = W.add_colony( W.kDesertTile );
    colony.commodities[e_commodity::ore]   = 100;
    colony.commodities[e_commodity::tools] = 100;
    W.add_unit_indoors( colony.id, e_indoor_job::tools,
                        e_unit_type::master_blacksmith );
    W.add_unit_indoors( colony.id, e_indoor_job::muskets,
                        e_unit_type::master_gunsmith );
    W.add_unit_outdoors( colony.id, e_direction::e,
                         e_outdoor_job::ore,
                         e_unit_type::free_colonist );
    W.add_unit_outdoors( colony.id, e_direction::s,
                         e_outdoor_job::ore,
                         e_unit_type::expert_ore_miner );
    colony.buildings[e_colony_building::iron_works] = true;
    colony.buildings[e_colony_building::armory]     = true;
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.center_extra_production ==
             SP{ .what = e_outdoor_job::ore, .quantity = 2 } );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::e,
              SP{ .what = e_outdoor_job::ore, .quantity = 2 } },
            { e_direction::s, SP{ .what     = e_outdoor_job::ore,
                                  .quantity = 4 } } } );
    REQUIRE( pr.ore_tools ==
             RMP{
                 .raw_produced                 = 8,
                 .raw_consumed_theoretical     = 12,
                 .raw_delta_theoretical        = -4,
                 .raw_consumed_actual          = 12,
                 .raw_delta_final              = -4,
                 .product_produced_theoretical = 18,
                 .product_produced_actual      = 18,
                 .product_delta_final          = 0,
             } );
    REQUIRE( pr.tools_muskets ==
             RMP{
                 .raw_produced                 = 18,
                 .raw_consumed_theoretical     = 6,
                 .raw_delta_theoretical        = 12,
                 .raw_consumed_actual          = 6,
                 .raw_delta_final              = 0,
                 .product_produced_theoretical = 6,
                 .product_produced_actual      = 6,
                 .product_delta_final          = 6,
             } );
  }

  SECTION(
      "ore center square, one expert ore miner, one ore miner, "
      "two master blacksmiths, some ore in store, no tools in "
      "store, iron works" ) {
    Colony& colony = W.add_colony( W.kDesertTile );
    colony.commodities[e_commodity::ore] = 3;
    W.add_unit_indoors( colony.id, e_indoor_job::tools,
                        e_unit_type::master_blacksmith );
    W.add_unit_indoors( colony.id, e_indoor_job::tools,
                        e_unit_type::master_blacksmith );
    W.add_unit_outdoors( colony.id, e_direction::e,
                         e_outdoor_job::ore,
                         e_unit_type::free_colonist );
    W.add_unit_outdoors( colony.id, e_direction::s,
                         e_outdoor_job::ore,
                         e_unit_type::expert_ore_miner );
    colony.buildings[e_colony_building::iron_works] = true;
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.center_extra_production ==
             SP{ .what = e_outdoor_job::ore, .quantity = 2 } );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::e,
              SP{ .what = e_outdoor_job::ore, .quantity = 2 } },
            { e_direction::s, SP{ .what     = e_outdoor_job::ore,
                                  .quantity = 4 } } } );
    REQUIRE( pr.ore_tools ==
             RMP{
                 .raw_produced                 = 8,
                 .raw_consumed_theoretical     = 24,
                 .raw_delta_theoretical        = -16,
                 .raw_consumed_actual          = 11,
                 .raw_delta_final              = -3,
                 .product_produced_theoretical = 36,
                 .product_produced_actual      = 17,
                 .product_delta_final          = 17,
             } );
    REQUIRE( pr.tools_muskets ==
             RMP{
                 .raw_produced                 = 17,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 17,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 17,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
  }

  SECTION(
      "ore center square, one expert ore miner, one ore miner, "
      "two master blacksmiths, some ore in store, tools almost "
      "full, iron works" ) {
    Colony& colony = W.add_colony( W.kDesertTile );
    colony.commodities[e_commodity::ore]   = 5;
    colony.commodities[e_commodity::tools] = 98;
    W.add_unit_indoors( colony.id, e_indoor_job::tools,
                        e_unit_type::master_blacksmith );
    W.add_unit_indoors( colony.id, e_indoor_job::tools,
                        e_unit_type::master_blacksmith );
    W.add_unit_outdoors( colony.id, e_direction::e,
                         e_outdoor_job::ore,
                         e_unit_type::free_colonist );
    W.add_unit_outdoors( colony.id, e_direction::s,
                         e_outdoor_job::ore,
                         e_unit_type::expert_ore_miner );
    colony.buildings[e_colony_building::iron_works] = true;
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.center_extra_production ==
             SP{ .what = e_outdoor_job::ore, .quantity = 2 } );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::e,
              SP{ .what = e_outdoor_job::ore, .quantity = 2 } },
            { e_direction::s, SP{ .what     = e_outdoor_job::ore,
                                  .quantity = 4 } } } );
    REQUIRE( pr.ore_tools ==
             RMP{
                 .raw_produced                 = 8,
                 .raw_consumed_theoretical     = 24,
                 .raw_delta_theoretical        = -16,
                 .raw_consumed_actual          = 13,
                 .raw_delta_final              = -5,
                 .product_produced_theoretical = 36,
                 .product_produced_actual      = 20,
                 .product_delta_final          = 2,
             } );
    REQUIRE( pr.tools_muskets ==
             RMP{
                 .raw_produced                 = 20,
                 .raw_consumed_theoretical     = 0,
                 .raw_delta_theoretical        = 20,
                 .raw_consumed_actual          = 0,
                 .raw_delta_final              = 2,
                 .product_produced_theoretical = 0,
                 .product_produced_actual      = 0,
                 .product_delta_final          = 0,
             } );
  }

  SECTION(
      "ore center square, one expert ore miner, one ore miner, "
      "two master blacksmiths, three gunsmiths, some ore in "
      "store, tools almost full, iron works, arsenal" ) {
    Colony& colony = W.add_colony( W.kDesertTile );
    colony.commodities[e_commodity::ore]   = 5;
    colony.commodities[e_commodity::tools] = 98;
    W.add_unit_indoors( colony.id, e_indoor_job::tools,
                        e_unit_type::master_blacksmith );
    W.add_unit_indoors( colony.id, e_indoor_job::tools,
                        e_unit_type::master_blacksmith );
    W.add_unit_indoors( colony.id, e_indoor_job::muskets,
                        e_unit_type::master_gunsmith );
    W.add_unit_indoors( colony.id, e_indoor_job::muskets,
                        e_unit_type::master_gunsmith );
    W.add_unit_indoors( colony.id, e_indoor_job::muskets,
                        e_unit_type::master_gunsmith );
    W.add_unit_outdoors( colony.id, e_direction::e,
                         e_outdoor_job::ore,
                         e_unit_type::free_colonist );
    W.add_unit_outdoors( colony.id, e_direction::s,
                         e_outdoor_job::ore,
                         e_unit_type::expert_ore_miner );
    colony.buildings[e_colony_building::iron_works] = true;
    colony.buildings[e_colony_building::arsenal]    = true;
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.center_extra_production ==
             SP{ .what = e_outdoor_job::ore, .quantity = 2 } );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::e,
              SP{ .what = e_outdoor_job::ore, .quantity = 2 } },
            { e_direction::s, SP{ .what     = e_outdoor_job::ore,
                                  .quantity = 4 } } } );
    REQUIRE( pr.ore_tools ==
             RMP{
                 .raw_produced                 = 8,
                 .raw_consumed_theoretical     = 24,
                 .raw_delta_theoretical        = -16,
                 .raw_consumed_actual          = 13,
                 .raw_delta_final              = -5,
                 .product_produced_theoretical = 36,
                 .product_produced_actual      = 20,
                 .product_delta_final          = -16,
             } );
    REQUIRE( pr.tools_muskets ==
             RMP{
                 .raw_produced                 = 20,
                 .raw_consumed_theoretical     = 36,
                 .raw_delta_theoretical        = -16,
                 .raw_consumed_actual          = 36,
                 .raw_delta_final              = -16,
                 .product_produced_theoretical = 54,
                 .product_produced_actual      = 54,
                 .product_delta_final          = 54,
             } );
  }

  SECTION(
      "ore center square, one expert ore miner, one ore miner, "
      "two master blacksmiths, three gunsmiths, some ore in "
      "store, tools almost full, muskets almost full, iron "
      "works, arsenal" ) {
    Colony& colony = W.add_colony( W.kDesertTile );
    colony.commodities[e_commodity::ore]     = 5;
    colony.commodities[e_commodity::tools]   = 98;
    colony.commodities[e_commodity::muskets] = 90;
    W.add_unit_indoors( colony.id, e_indoor_job::tools,
                        e_unit_type::master_blacksmith );
    W.add_unit_indoors( colony.id, e_indoor_job::tools,
                        e_unit_type::master_blacksmith );
    W.add_unit_indoors( colony.id, e_indoor_job::muskets,
                        e_unit_type::master_gunsmith );
    W.add_unit_indoors( colony.id, e_indoor_job::muskets,
                        e_unit_type::master_gunsmith );
    W.add_unit_indoors( colony.id, e_indoor_job::muskets,
                        e_unit_type::master_gunsmith );
    W.add_unit_outdoors( colony.id, e_direction::e,
                         e_outdoor_job::ore,
                         e_unit_type::free_colonist );
    W.add_unit_outdoors( colony.id, e_direction::s,
                         e_outdoor_job::ore,
                         e_unit_type::expert_ore_miner );
    colony.buildings[e_colony_building::iron_works] = true;
    colony.buildings[e_colony_building::arsenal]    = true;
    ColonyProduction pr =
        production_for_colony( W.ss(), colony );
    REQUIRE( pr.center_extra_production ==
             SP{ .what = e_outdoor_job::ore, .quantity = 2 } );
    REQUIRE(
        pr.land_production ==
        LP{ { e_direction::e,
              SP{ .what = e_outdoor_job::ore, .quantity = 2 } },
            { e_direction::s, SP{ .what     = e_outdoor_job::ore,
                                  .quantity = 4 } } } );
    REQUIRE( pr.ore_tools ==
             RMP{
                 .raw_produced                 = 8,
                 .raw_consumed_theoretical     = 24,
                 .raw_delta_theoretical        = -16,
                 .raw_consumed_actual          = 13,
                 .raw_delta_final              = -5,
                 .product_produced_theoretical = 36,
                 .product_produced_actual      = 20,
                 .product_delta_final          = -16,
             } );
    REQUIRE( pr.tools_muskets ==
             RMP{
                 .raw_produced                 = 20,
                 .raw_consumed_theoretical     = 36,
                 .raw_delta_theoretical        = -16,
                 .raw_consumed_actual          = 36,
                 .raw_delta_final              = -16,
                 .product_produced_theoretical = 54,
                 .product_produced_actual      = 54,
                 .product_delta_final          = 10,
             } );
  }
}

TEST_CASE( "[production] bell production [discoverer]" ) {
  World W;
  W.create_default_map();
  // TODO
}

TEST_CASE( "[production] with 50 percent SoL" ) {
  World W;
  W.create_default_map();
  // TODO
}

TEST_CASE( "[production] with 100 percent SoL" ) {
  World W;
  W.create_default_map();
  // TODO
}

TEST_CASE( "[production] with tory penalty" ) {
  World W;
  W.create_default_map();
  // TODO
}

} // namespace
} // namespace rn
