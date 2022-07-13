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

// ss
#include "src/ss/player.rds.hpp"
#include "src/ss/ref.hpp"

// refl
#include "refl/to-str.hpp"

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

  void create_default_map() {
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
    // clang-format off
    vector<MapSquare> tiles{
      _, L, _,
      L, L, L,
      _, L, L,
    };
    // clang-format on
    build_map( std::move( tiles ), 3 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[production] production_for_slot" ) {
  ColonyProduction pr;

  pr.ore_products.muskets_produced_theoretical   = 1;
  pr.ore_products.tools_produced_theoretical     = 2;
  pr.sugar_rum.product_produced_theoretical      = 3;
  pr.cotton_cloth.product_produced_theoretical   = 4;
  pr.fur_coats.product_produced_theoretical      = 5;
  pr.tobacco_cigars.product_produced_theoretical = 6;
  pr.lumber_hammers.product_produced_theoretical = 7;
  pr.bells                                       = 8;
  pr.food.horses_produced_theoretical            = 12;
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

TEST_CASE( "[production] lumber/hammers" ) {
  World W;
  W.create_default_map();
  Colony&    colony = W.add_colony( Coord{ .x = 1, .y = 1 } );
  gfx::point P{ .x = 0, .y = 1 };

  auto lum = [&] {
    return production_for_colony( W.ss(), colony )
        .lumber_hammers;
  };

  // TODO
  // Need to add hammers.

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

TEST_CASE( "[production] food/horses" ) {
  World W;
  W.create_default_map();
  Colony& colony = W.add_colony( Coord{ .x = 1, .y = 1 } );
  REQUIRE( colony_population( colony ) == 0 );

  SECTION( "no units no horses/arctic" ) {
    // TODO: implement center production first.
  }

  SECTION( "no units no horses" ) {
    // TODO: implement center production first.
  }

  SECTION( "one farmer no horses" ) {
    // TODO: implement center production first.
  }

  SECTION( "one fisherman no horses" ) {
    // TODO: implement center production first.
  }

  SECTION( "one farmer breaking even" ) {
    // TODO: implement center production first.
  }

  SECTION( "one farmer breaking even, horses=2" ) {
    // TODO: implement center production first.
  }

  SECTION( "one farmer surplus=1, horses=1" ) {
    // TODO: implement center production first.
  }

  SECTION( "one farmer surplus=1, horses=2" ) {
    // TODO: implement center production first.
  }

  SECTION( "one fisherman surplus=3, horses=0" ) {
    // TODO: implement center production first.
  }

  SECTION( "one farmer surplus=3, horses=25" ) {
    // TODO: implement center production first.
  }

  SECTION( "one farmer surplus=3, horses=max" ) {
    // TODO: implement center production first.
  }

  SECTION( "one farmer surplus=3, horses=25, +stable" ) {
    // TODO: implement center production first.
  }

  SECTION( "deficit no starve, horses=0" ) {
    // TODO: implement center production first.
  }

  SECTION( "deficit no starve, horses=50" ) {
    // TODO: implement center production first.
  }

  SECTION( "deficit and starve" ) {
    // TODO: implement center production first.
  }

  SECTION( "deficit and starve, horses=50" ) {
    // TODO: implement center production first.
  }

  SECTION( "two farmers, two fisherman, surplus" ) {
    // TODO: implement center production first.
  }

  SECTION(
      "one farmer, surplus=2, warehouse expansion, horses=100, "
      "+stable" ) {
    // TODO: implement center production first.
  }
}

TEST_CASE( "[production] tobacco/cigar" ) {
  World W;
  W.create_default_map();
  // TODO
}

} // namespace
} // namespace rn
