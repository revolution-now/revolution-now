/****************************************************************
**land-production.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-14.
*
* Description: Unit tests for the src/land-production.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/land-production.hpp"

// Testing.
#include "test/fake/world.hpp"

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
    MapSquare const S = make_sea_lane();
    MapSquare const L = make_grassland();
    // clang-format off
    vector<MapSquare> tiles{
      _, L, _, _, _, _,
      L, L, L, _, _, _,
      _, L, L, _, _, _,
    };
    // clang-format on
    build_map( std::move( tiles ), 6_w );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[production] outdoor_job_for_expertise" ) {
  unordered_set<e_unit_activity> used;
  auto f = [&]( e_unit_activity activity ) {
    used.insert( activity );
    return outdoor_job_for_expertise( activity );
  };
  REQUIRE( f( e_unit_activity::farming ) ==
           e_outdoor_job::food );
  REQUIRE( f( e_unit_activity::fishing ) ==
           e_outdoor_job::fish );
  REQUIRE( f( e_unit_activity::sugar_planting ) ==
           e_outdoor_job::sugar );
  REQUIRE( f( e_unit_activity::tobacco_planting ) ==
           e_outdoor_job::tobacco );
  REQUIRE( f( e_unit_activity::cotton_planting ) ==
           e_outdoor_job::cotton );
  REQUIRE( f( e_unit_activity::fur_trapping ) ==
           e_outdoor_job::fur );
  REQUIRE( f( e_unit_activity::lumberjacking ) ==
           e_outdoor_job::lumber );
  REQUIRE( f( e_unit_activity::ore_mining ) ==
           e_outdoor_job::ore );
  REQUIRE( f( e_unit_activity::silver_mining ) ==
           e_outdoor_job::silver );
  REQUIRE( f( e_unit_activity::carpentry ) == nothing );
  REQUIRE( f( e_unit_activity::rum_distilling ) == nothing );
  REQUIRE( f( e_unit_activity::tobacconistry ) == nothing );
  REQUIRE( f( e_unit_activity::weaving ) == nothing );
  REQUIRE( f( e_unit_activity::fur_trading ) == nothing );
  REQUIRE( f( e_unit_activity::blacksmithing ) == nothing );
  REQUIRE( f( e_unit_activity::gunsmithing ) == nothing );
  REQUIRE( f( e_unit_activity::fighting ) == nothing );
  REQUIRE( f( e_unit_activity::pioneering ) == nothing );
  REQUIRE( f( e_unit_activity::scouting ) == nothing );
  REQUIRE( f( e_unit_activity::missioning ) == nothing );
  REQUIRE( f( e_unit_activity::bell_ringing ) == nothing );
  REQUIRE( f( e_unit_activity::preaching ) == nothing );
  REQUIRE( f( e_unit_activity::teaching ) == nothing );

  REQUIRE( used.size() == refl::enum_count<e_unit_activity> );
}

TEST_CASE( "[production] activity_for_outdoor_job" ) {
  unordered_set<e_outdoor_job> used;
  auto                         f = [&]( e_outdoor_job job ) {
    used.insert( job );
    return activity_for_outdoor_job( job );
  };
  REQUIRE( f( e_outdoor_job::food ) ==
           e_unit_activity::farming );
  REQUIRE( f( e_outdoor_job::fish ) ==
           e_unit_activity::fishing );
  REQUIRE( f( e_outdoor_job::sugar ) ==
           e_unit_activity::sugar_planting );
  REQUIRE( f( e_outdoor_job::tobacco ) ==
           e_unit_activity::tobacco_planting );
  REQUIRE( f( e_outdoor_job::cotton ) ==
           e_unit_activity::cotton_planting );
  REQUIRE( f( e_outdoor_job::fur ) ==
           e_unit_activity::fur_trapping );
  REQUIRE( f( e_outdoor_job::lumber ) ==
           e_unit_activity::lumberjacking );
  REQUIRE( f( e_outdoor_job::ore ) ==
           e_unit_activity::ore_mining );
  REQUIRE( f( e_outdoor_job::silver ) ==
           e_unit_activity::silver_mining );

  REQUIRE( used.size() == refl::enum_count<e_outdoor_job> );
}

TEST_CASE( "[production] production_on_square/cotton" ) {
  World W;
  W.create_default_map();
  gfx::point P{ .x = 0, .y = 1 };

  e_outdoor_job const job = e_outdoor_job::cotton;

  auto S = [&]() -> decltype( auto ) { return W.square( P ); };

  SECTION( "petty_criminal" ) {
    auto f = [&] {
      return production_on_square( job, W.terrain(),
                                   e_unit_type::petty_criminal,
                                   Coord::from_gfx( P ) );
    };

    S()        = { .surface = e_surface::land };
    S().ground = e_ground_terrain::grassland;
    REQUIRE( f() == 0 );
    S().road = true;
    REQUIRE( f() == 0 );
    S().irrigation = true;
    REQUIRE( f() == 0 );
    S().river = e_river::minor;
    REQUIRE( f() == 0 );
    S().river = e_river::major;
    REQUIRE( f() == 0 );
    S().ground_resource = e_natural_resource::cotton;
    REQUIRE( f() == 0 );
    S().overlay = e_land_overlay::forest;
    REQUIRE( f() == 0 );

    S()        = { .surface = e_surface::land };
    S().ground = e_ground_terrain::prairie;
    REQUIRE( f() == 3 );
    S().road = true;
    REQUIRE( f() == 3 );
    S().river = e_river::major;
    REQUIRE( f() == 5 );
    S().river = e_river::minor;
    REQUIRE( f() == 4 );
    S().irrigation = true;
    REQUIRE( f() == 5 );
    S().ground_resource = e_natural_resource::cotton;
    REQUIRE( f() == 8 );
    S().overlay = e_land_overlay::forest;
    REQUIRE( f() == 2 );
  }

  SECTION( "free_colonist" ) {
    auto f = [&] {
      return production_on_square( job, W.terrain(),
                                   e_unit_type::free_colonist,
                                   Coord::from_gfx( P ) );
    };

    S()        = { .surface = e_surface::land };
    S().ground = e_ground_terrain::grassland;
    REQUIRE( f() == 0 );
    S().road = true;
    REQUIRE( f() == 0 );
    S().irrigation = true;
    REQUIRE( f() == 0 );
    S().river = e_river::major;
    REQUIRE( f() == 0 );
    S().river = e_river::minor;
    REQUIRE( f() == 0 );
    S().ground_resource = e_natural_resource::cotton;
    REQUIRE( f() == 0 );
    S().overlay = e_land_overlay::forest;
    REQUIRE( f() == 0 );

    S()        = { .surface = e_surface::land };
    S().ground = e_ground_terrain::prairie;
    REQUIRE( f() == 3 );
    S().road = true;
    REQUIRE( f() == 3 );
    S().river = e_river::major;
    REQUIRE( f() == 5 );
    S().river = e_river::minor;
    REQUIRE( f() == 4 );
    S().irrigation = true;
    REQUIRE( f() == 5 );
    S().ground_resource = e_natural_resource::cotton;
    REQUIRE( f() == 8 );
    S().overlay = e_land_overlay::forest;
    REQUIRE( f() == 2 );
  }

  SECTION( "expert" ) {
    auto f = [&] {
      return production_on_square(
          job, W.terrain(), e_unit_type::expert_cotton_planter,
          Coord::from_gfx( P ) );
    };

    S()        = { .surface = e_surface::land };
    S().ground = e_ground_terrain::grassland;
    REQUIRE( f() == 0 );
    S().road = true;
    REQUIRE( f() == 0 );
    S().irrigation = true;
    REQUIRE( f() == 0 );
    S().river = e_river::major;
    REQUIRE( f() == 0 );
    S().river = e_river::minor;
    REQUIRE( f() == 0 );
    S().ground_resource = e_natural_resource::cotton;
    REQUIRE( f() == 0 );
    S().overlay = e_land_overlay::forest;
    REQUIRE( f() == 0 );

    S()        = { .surface = e_surface::land };
    S().ground = e_ground_terrain::prairie;
    REQUIRE( f() == 6 );
    S().road = true;
    REQUIRE( f() == 6 );
    S().irrigation = true;
    REQUIRE( f() == 8 );
    S().river = e_river::major;
    REQUIRE( f() == 12 );
    S().river = e_river::minor;
    REQUIRE( f() == 10 );
    S().ground_resource = e_natural_resource::cotton;
    REQUIRE( f() == 16 );
    S().overlay = e_land_overlay::forest;
    REQUIRE( f() == 4 );
  }
}

TEST_CASE( "[production] production_on_square/silver" ) {
  World W;
  W.create_default_map();
  gfx::point P{ .x = 0, .y = 1 };

  e_outdoor_job const job = e_outdoor_job::silver;

  auto S = [&]() -> decltype( auto ) { return W.square( P ); };

  SECTION( "petty_criminal" ) {
    auto f = [&] {
      return production_on_square( job, W.terrain(),
                                   e_unit_type::petty_criminal,
                                   Coord::from_gfx( P ) );
    };

    S()        = { .surface = e_surface::land };
    S().ground = e_ground_terrain::grassland;
    REQUIRE( f() == 0 );
    S().irrigation = true;
    REQUIRE( f() == 0 );
    S().overlay = e_land_overlay::forest;
    REQUIRE( f() == 0 );
    S().overlay = e_land_overlay::hills;
    REQUIRE( f() == 0 );
    S().road = true;
    REQUIRE( f() == 0 );
    S().road    = false;
    S().overlay = e_land_overlay::mountains;
    REQUIRE( f() == 0 );
    S().ground_resource = e_natural_resource::silver_depleted;
    REQUIRE( f() == 1 );
    S().ground_resource = e_natural_resource::silver;
    REQUIRE( f() == 3 );
    S().road = true;
    REQUIRE( f() == 4 );
    S().river = e_river::major;
    REQUIRE( f() == 6 );
    S().river = e_river::minor;
    REQUIRE( f() == 5 );
  }

  SECTION( "free_colonist" ) {
    auto f = [&] {
      return production_on_square( job, W.terrain(),
                                   e_unit_type::free_colonist,
                                   Coord::from_gfx( P ) );
    };

    S()        = { .surface = e_surface::land };
    S().ground = e_ground_terrain::grassland;
    REQUIRE( f() == 0 );
    S().irrigation = true;
    REQUIRE( f() == 0 );
    S().overlay = e_land_overlay::forest;
    REQUIRE( f() == 0 );
    S().overlay = e_land_overlay::hills;
    REQUIRE( f() == 0 );
    S().road = true;
    REQUIRE( f() == 0 );
    S().road    = false;
    S().overlay = e_land_overlay::mountains;
    REQUIRE( f() == 0 );
    S().ground_resource = e_natural_resource::silver_depleted;
    REQUIRE( f() == 1 );
    S().ground_resource = e_natural_resource::silver;
    REQUIRE( f() == 3 );
    S().road = true;
    REQUIRE( f() == 4 );
    S().river = e_river::major;
    REQUIRE( f() == 6 );
    S().river = e_river::minor;
    REQUIRE( f() == 5 );
  }

  SECTION( "expert" ) {
    auto f = [&] {
      return production_on_square(
          job, W.terrain(), e_unit_type::expert_silver_miner,
          Coord::from_gfx( P ) );
    };

    S()        = { .surface = e_surface::land };
    S().ground = e_ground_terrain::grassland;
    REQUIRE( f() == 0 );
    S().irrigation = true;
    REQUIRE( f() == 0 );
    S().overlay = e_land_overlay::forest;
    REQUIRE( f() == 0 );
    S().overlay = e_land_overlay::hills;
    REQUIRE( f() == 0 );
    S().road = true;
    REQUIRE( f() == 0 );
    S().road    = false;
    S().overlay = e_land_overlay::mountains;
    REQUIRE( f() == 1 );
    S().road = true;
    REQUIRE( f() == 1 );
    S().road            = false;
    S().ground_resource = e_natural_resource::silver_depleted;
    REQUIRE( f() == 2 );
    S().ground_resource = e_natural_resource::silver;
    REQUIRE( f() == 6 );
    S().road = true;
    REQUIRE( f() == 8 );
    S().river = e_river::major;
    REQUIRE( f() == 12 );
    S().river = e_river::minor;
    REQUIRE( f() == 10 );
  }
}

TEST_CASE( "[production] production_on_square/food" ) {
  World W;
  W.create_default_map();
  gfx::point P{ .x = 0, .y = 1 };

  e_outdoor_job const job = e_outdoor_job::food;

  auto S = [&]() -> decltype( auto ) { return W.square( P ); };

  SECTION( "petty_criminal" ) {
    auto f = [&] {
      return production_on_square( job, W.terrain(),
                                   e_unit_type::petty_criminal,
                                   Coord::from_gfx( P ) );
    };

    S()        = { .surface = e_surface::land };
    S().ground = e_ground_terrain::grassland;
    REQUIRE( f() == 3 );
    S().road = true;
    REQUIRE( f() == 3 );
    S().irrigation = true;
    REQUIRE( f() == 4 );
    S().river = e_river::major;
    REQUIRE( f() == 5 );
    S().river = e_river::minor;
    REQUIRE( f() == 5 );
    S().ground_resource = e_natural_resource::cotton;
    REQUIRE( f() == 5 );
    S().ground_resource = e_natural_resource::wheat;
    REQUIRE( f() == 7 );
    S().overlay = e_land_overlay::forest;
    REQUIRE( f() == 3 );
    S().forest_resource = e_natural_resource::deer;
    REQUIRE( f() == 5 );
    S().overlay = e_land_overlay::mountains;
    REQUIRE( f() == 0 );
    S().ground_resource = nothing;
    S().overlay         = e_land_overlay::hills;
    REQUIRE( f() == 3 );

    S()        = { .surface = e_surface::land };
    S().ground = e_ground_terrain::plains;
    REQUIRE( f() == 5 );
    S().road = true;
    REQUIRE( f() == 5 );
    S().river = e_river::major;
    REQUIRE( f() == 6 );
    S().river = e_river::minor;
    REQUIRE( f() == 6 );
    S().irrigation = true;
    REQUIRE( f() == 7 );
    S().ground_resource = e_natural_resource::cotton;
    REQUIRE( f() == 7 );
    S().ground_resource = e_natural_resource::wheat;
    REQUIRE( f() == 9 );
    S().overlay = e_land_overlay::forest;
    REQUIRE( f() == 4 );
    S().forest_resource = e_natural_resource::deer;
    REQUIRE( f() == 6 );
    S().overlay = e_land_overlay::mountains;
    REQUIRE( f() == 0 );
    S().ground_resource = nothing;
    S().overlay         = e_land_overlay::hills;
    REQUIRE( f() == 3 );
  }

  SECTION( "free_colonist" ) {
    auto f = [&] {
      return production_on_square( job, W.terrain(),
                                   e_unit_type::free_colonist,
                                   Coord::from_gfx( P ) );
    };

    S()        = { .surface = e_surface::land };
    S().ground = e_ground_terrain::grassland;
    REQUIRE( f() == 3 );
    S().road = true;
    REQUIRE( f() == 3 );
    S().irrigation = true;
    REQUIRE( f() == 4 );
    S().river = e_river::major;
    REQUIRE( f() == 5 );
    S().river = e_river::minor;
    REQUIRE( f() == 5 );
    S().ground_resource = e_natural_resource::cotton;
    REQUIRE( f() == 5 );
    S().ground_resource = e_natural_resource::wheat;
    REQUIRE( f() == 7 );
    S().overlay = e_land_overlay::forest;
    REQUIRE( f() == 3 );
    S().forest_resource = e_natural_resource::deer;
    REQUIRE( f() == 5 );
    S().overlay = e_land_overlay::mountains;
    REQUIRE( f() == 0 );
    S().ground_resource = nothing;
    S().overlay         = e_land_overlay::hills;
    REQUIRE( f() == 3 );

    S()        = { .surface = e_surface::land };
    S().ground = e_ground_terrain::plains;
    REQUIRE( f() == 5 );
    S().road = true;
    REQUIRE( f() == 5 );
    S().river = e_river::major;
    REQUIRE( f() == 6 );
    S().river = e_river::minor;
    REQUIRE( f() == 6 );
    S().irrigation = true;
    REQUIRE( f() == 7 );
    S().ground_resource = e_natural_resource::cotton;
    REQUIRE( f() == 7 );
    S().ground_resource = e_natural_resource::wheat;
    REQUIRE( f() == 9 );
    S().overlay = e_land_overlay::forest;
    REQUIRE( f() == 4 );
    S().forest_resource = e_natural_resource::deer;
    REQUIRE( f() == 6 );
    S().overlay = e_land_overlay::mountains;
    REQUIRE( f() == 0 );
    S().ground_resource = nothing;
    S().overlay         = e_land_overlay::hills;
    REQUIRE( f() == 3 );
  }

  SECTION( "expert" ) {
    auto f = [&] {
      return production_on_square( job, W.terrain(),
                                   e_unit_type::expert_farmer,
                                   Coord::from_gfx( P ) );
    };

    S()        = { .surface = e_surface::land };
    S().ground = e_ground_terrain::grassland;
    REQUIRE( f() == 5 );
    S().road = true;
    REQUIRE( f() == 5 );
    S().irrigation = true;
    REQUIRE( f() == 6 );
    S().river = e_river::major;
    REQUIRE( f() == 7 );
    S().river = e_river::minor;
    REQUIRE( f() == 7 );
    S().ground_resource = e_natural_resource::cotton;
    REQUIRE( f() == 7 );
    S().ground_resource = e_natural_resource::wheat;
    REQUIRE( f() == 11 );
    S().overlay = e_land_overlay::forest;
    REQUIRE( f() == 5 );
    S().forest_resource = e_natural_resource::deer;
    REQUIRE( f() == 9 );
    S().overlay = e_land_overlay::mountains;
    REQUIRE( f() == 0 );
    S().ground_resource = nothing;
    S().overlay         = e_land_overlay::hills;
    REQUIRE( f() == 5 );

    S()        = { .surface = e_surface::land };
    S().ground = e_ground_terrain::plains;
    REQUIRE( f() == 7 );
    S().road = true;
    REQUIRE( f() == 7 );
    S().river = e_river::major;
    REQUIRE( f() == 8 );
    S().river = e_river::minor;
    REQUIRE( f() == 8 );
    S().irrigation = true;
    REQUIRE( f() == 9 );
    S().ground_resource = e_natural_resource::cotton;
    REQUIRE( f() == 9 );
    S().ground_resource = e_natural_resource::wheat;
    REQUIRE( f() == 13 );
    S().overlay = e_land_overlay::forest;
    REQUIRE( f() == 6 );
    S().forest_resource = e_natural_resource::deer;
    REQUIRE( f() == 10 );
    S().overlay = e_land_overlay::mountains;
    REQUIRE( f() == 0 );
    S().ground_resource = nothing;
    S().overlay         = e_land_overlay::hills;
    REQUIRE( f() == 5 );
  }
}

TEST_CASE( "[production] production_on_square/sugar" ) {
  World W;
  W.create_default_map();
  gfx::point P{ .x = 0, .y = 1 };

  e_outdoor_job const job = e_outdoor_job::sugar;

  auto S = [&]() -> decltype( auto ) { return W.square( P ); };

  SECTION( "petty_criminal" ) {
    auto f = [&] {
      return production_on_square( job, W.terrain(),
                                   e_unit_type::petty_criminal,
                                   Coord::from_gfx( P ) );
    };

    S()        = { .surface = e_surface::land };
    S().ground = e_ground_terrain::grassland;
    REQUIRE( f() == 0 );
    S().road = true;
    REQUIRE( f() == 0 );
    S().irrigation = true;
    REQUIRE( f() == 0 );
    S().river = e_river::major;
    REQUIRE( f() == 0 );
    S().river = e_river::minor;
    REQUIRE( f() == 0 );
    S().ground_resource = e_natural_resource::cotton;
    REQUIRE( f() == 0 );
    S().ground_resource = e_natural_resource::sugar;
    REQUIRE( f() == 0 );
    S().overlay = e_land_overlay::forest;
    REQUIRE( f() == 0 );
    S().overlay = e_land_overlay::mountains;
    REQUIRE( f() == 0 );
    S().ground_resource = nothing;
    S().overlay         = e_land_overlay::hills;
    REQUIRE( f() == 0 );

    S()        = { .surface = e_surface::land };
    S().ground = e_ground_terrain::savannah;
    REQUIRE( f() == 3 );
    S().road = true;
    REQUIRE( f() == 3 );
    S().river = e_river::major;
    REQUIRE( f() == 5 );
    S().river = e_river::minor;
    REQUIRE( f() == 4 );
    S().irrigation = true;
    REQUIRE( f() == 5 );
    S().ground_resource = e_natural_resource::cotton;
    REQUIRE( f() == 5 );
    S().ground_resource = e_natural_resource::sugar;
    REQUIRE( f() == 8 );
    S().overlay = e_land_overlay::forest;
    REQUIRE( f() == 2 );
    S().overlay = e_land_overlay::mountains;
    REQUIRE( f() == 0 );
    S().ground_resource = nothing;
    S().overlay         = e_land_overlay::hills;
    REQUIRE( f() == 0 );
  }

  SECTION( "free_colonist" ) {
    auto f = [&] {
      return production_on_square( job, W.terrain(),
                                   e_unit_type::free_colonist,
                                   Coord::from_gfx( P ) );
    };

    S()        = { .surface = e_surface::land };
    S().ground = e_ground_terrain::grassland;
    REQUIRE( f() == 0 );
    S().road = true;
    REQUIRE( f() == 0 );
    S().irrigation = true;
    REQUIRE( f() == 0 );
    S().river = e_river::major;
    REQUIRE( f() == 0 );
    S().river = e_river::minor;
    REQUIRE( f() == 0 );
    S().ground_resource = e_natural_resource::cotton;
    REQUIRE( f() == 0 );
    S().ground_resource = e_natural_resource::sugar;
    REQUIRE( f() == 0 );
    S().overlay = e_land_overlay::forest;
    REQUIRE( f() == 0 );
    S().overlay = e_land_overlay::mountains;
    REQUIRE( f() == 0 );
    S().ground_resource = nothing;
    S().overlay         = e_land_overlay::hills;
    REQUIRE( f() == 0 );

    S()        = { .surface = e_surface::land };
    S().ground = e_ground_terrain::savannah;
    REQUIRE( f() == 3 );
    S().road = true;
    REQUIRE( f() == 3 );
    S().river = e_river::major;
    REQUIRE( f() == 5 );
    S().river = e_river::minor;
    REQUIRE( f() == 4 );
    S().irrigation = true;
    REQUIRE( f() == 5 );
    S().ground_resource = e_natural_resource::cotton;
    REQUIRE( f() == 5 );
    S().ground_resource = e_natural_resource::sugar;
    REQUIRE( f() == 8 );
    S().overlay = e_land_overlay::forest;
    REQUIRE( f() == 2 );
    S().overlay = e_land_overlay::mountains;
    REQUIRE( f() == 0 );
    S().ground_resource = nothing;
    S().overlay         = e_land_overlay::hills;
    REQUIRE( f() == 0 );
  }

  SECTION( "expert" ) {
    auto f = [&] {
      return production_on_square(
          job, W.terrain(), e_unit_type::expert_sugar_planter,
          Coord::from_gfx( P ) );
    };

    S()        = { .surface = e_surface::land };
    S().ground = e_ground_terrain::grassland;
    REQUIRE( f() == 0 );
    S().road = true;
    REQUIRE( f() == 0 );
    S().irrigation = true;
    REQUIRE( f() == 0 );
    S().river = e_river::major;
    REQUIRE( f() == 0 );
    S().river = e_river::minor;
    REQUIRE( f() == 0 );
    S().ground_resource = e_natural_resource::cotton;
    REQUIRE( f() == 0 );
    S().ground_resource = e_natural_resource::sugar;
    REQUIRE( f() == 0 );
    S().overlay = e_land_overlay::forest;
    REQUIRE( f() == 0 );
    S().overlay = e_land_overlay::mountains;
    REQUIRE( f() == 0 );
    S().ground_resource = nothing;
    S().overlay         = e_land_overlay::hills;
    REQUIRE( f() == 0 );

    S()        = { .surface = e_surface::land };
    S().ground = e_ground_terrain::savannah;
    REQUIRE( f() == 6 );
    S().road = true;
    REQUIRE( f() == 6 );
    S().river = e_river::major;
    REQUIRE( f() == 10 );
    S().river = e_river::minor;
    REQUIRE( f() == 8 );
    S().irrigation = true;
    REQUIRE( f() == 10 );
    S().ground_resource = e_natural_resource::cotton;
    REQUIRE( f() == 10 );
    S().ground_resource = e_natural_resource::sugar;
    REQUIRE( f() == 16 );
    S().overlay = e_land_overlay::forest;
    REQUIRE( f() == 4 );
    S().overlay = e_land_overlay::mountains;
    REQUIRE( f() == 0 );
    S().ground_resource = nothing;
    S().overlay         = e_land_overlay::hills;
    REQUIRE( f() == 0 );
  }
}

TEST_CASE( "[production] production_on_square/tobacco" ) {
  World W;
  W.create_default_map();
  gfx::point P{ .x = 0, .y = 1 };

  e_outdoor_job const job = e_outdoor_job::tobacco;

  auto S = [&]() -> decltype( auto ) { return W.square( P ); };

  SECTION( "petty_criminal" ) {
    auto f = [&] {
      return production_on_square( job, W.terrain(),
                                   e_unit_type::petty_criminal,
                                   Coord::from_gfx( P ) );
    };

    S()        = { .surface = e_surface::land };
    S().ground = e_ground_terrain::savannah;
    REQUIRE( f() == 0 );
    S().road = true;
    REQUIRE( f() == 0 );
    S().irrigation = true;
    REQUIRE( f() == 0 );
    S().river = e_river::major;
    REQUIRE( f() == 0 );
    S().river = e_river::minor;
    REQUIRE( f() == 0 );
    S().ground_resource = e_natural_resource::cotton;
    REQUIRE( f() == 0 );
    S().ground_resource = e_natural_resource::tobacco;
    REQUIRE( f() == 0 );
    S().overlay = e_land_overlay::forest;
    REQUIRE( f() == 0 );
    S().overlay = e_land_overlay::mountains;
    REQUIRE( f() == 0 );
    S().ground_resource = nothing;
    S().overlay         = e_land_overlay::hills;
    REQUIRE( f() == 0 );

    S()        = { .surface = e_surface::land };
    S().ground = e_ground_terrain::grassland;
    REQUIRE( f() == 3 );
    S().road = true;
    REQUIRE( f() == 3 );
    S().river = e_river::major;
    REQUIRE( f() == 5 );
    S().river = e_river::minor;
    REQUIRE( f() == 4 );
    S().irrigation = true;
    REQUIRE( f() == 5 );
    S().ground_resource = e_natural_resource::cotton;
    REQUIRE( f() == 5 );
    S().ground_resource = e_natural_resource::tobacco;
    REQUIRE( f() == 8 );
    S().overlay = e_land_overlay::forest;
    REQUIRE( f() == 2 );
    S().overlay = e_land_overlay::mountains;
    REQUIRE( f() == 0 );
    S().ground_resource = nothing;
    S().overlay         = e_land_overlay::hills;
    REQUIRE( f() == 0 );
  }

  SECTION( "free_colonist" ) {
    auto f = [&] {
      return production_on_square( job, W.terrain(),
                                   e_unit_type::free_colonist,
                                   Coord::from_gfx( P ) );
    };

    S()        = { .surface = e_surface::land };
    S().ground = e_ground_terrain::savannah;
    REQUIRE( f() == 0 );
    S().road = true;
    REQUIRE( f() == 0 );
    S().irrigation = true;
    REQUIRE( f() == 0 );
    S().river = e_river::major;
    REQUIRE( f() == 0 );
    S().river = e_river::minor;
    REQUIRE( f() == 0 );
    S().ground_resource = e_natural_resource::cotton;
    REQUIRE( f() == 0 );
    S().ground_resource = e_natural_resource::tobacco;
    REQUIRE( f() == 0 );
    S().overlay = e_land_overlay::forest;
    REQUIRE( f() == 0 );
    S().overlay = e_land_overlay::mountains;
    REQUIRE( f() == 0 );
    S().ground_resource = nothing;
    S().overlay         = e_land_overlay::hills;
    REQUIRE( f() == 0 );

    S()        = { .surface = e_surface::land };
    S().ground = e_ground_terrain::grassland;
    REQUIRE( f() == 3 );
    S().road = true;
    REQUIRE( f() == 3 );
    S().river = e_river::major;
    REQUIRE( f() == 5 );
    S().river = e_river::minor;
    REQUIRE( f() == 4 );
    S().irrigation = true;
    REQUIRE( f() == 5 );
    S().ground_resource = e_natural_resource::cotton;
    REQUIRE( f() == 5 );
    S().ground_resource = e_natural_resource::tobacco;
    REQUIRE( f() == 8 );
    S().overlay = e_land_overlay::forest;
    REQUIRE( f() == 2 );
    S().overlay = e_land_overlay::mountains;
    REQUIRE( f() == 0 );
    S().ground_resource = nothing;
    S().overlay         = e_land_overlay::hills;
    REQUIRE( f() == 0 );
  }

  SECTION( "expert" ) {
    auto f = [&] {
      return production_on_square(
          job, W.terrain(), e_unit_type::expert_tobacco_planter,
          Coord::from_gfx( P ) );
    };

    S()        = { .surface = e_surface::land };
    S().ground = e_ground_terrain::savannah;
    REQUIRE( f() == 0 );
    S().road = true;
    REQUIRE( f() == 0 );
    S().irrigation = true;
    REQUIRE( f() == 0 );
    S().river = e_river::major;
    REQUIRE( f() == 0 );
    S().river = e_river::minor;
    REQUIRE( f() == 0 );
    S().ground_resource = e_natural_resource::cotton;
    REQUIRE( f() == 0 );
    S().ground_resource = e_natural_resource::tobacco;
    REQUIRE( f() == 0 );
    S().overlay = e_land_overlay::forest;
    REQUIRE( f() == 0 );
    S().overlay = e_land_overlay::mountains;
    REQUIRE( f() == 0 );
    S().ground_resource = nothing;
    S().overlay         = e_land_overlay::hills;
    REQUIRE( f() == 0 );

    S()        = { .surface = e_surface::land };
    S().ground = e_ground_terrain::grassland;
    REQUIRE( f() == 6 );
    S().road = true;
    REQUIRE( f() == 6 );
    S().river = e_river::major;
    REQUIRE( f() == 10 );
    S().river = e_river::minor;
    REQUIRE( f() == 8 );
    S().irrigation = true;
    REQUIRE( f() == 10 );
    S().ground_resource = e_natural_resource::cotton;
    REQUIRE( f() == 10 );
    S().ground_resource = e_natural_resource::tobacco;
    REQUIRE( f() == 16 );
    S().overlay = e_land_overlay::forest;
    REQUIRE( f() == 4 );
    S().overlay = e_land_overlay::mountains;
    REQUIRE( f() == 0 );
    S().ground_resource = nothing;
    S().overlay         = e_land_overlay::hills;
    REQUIRE( f() == 0 );
  }
}

TEST_CASE( "[production] production_on_square/fur" ) {
  World W;
  W.create_default_map();
  gfx::point P{ .x = 0, .y = 1 };

  e_outdoor_job const job = e_outdoor_job::fur;

  auto S = [&]() -> decltype( auto ) { return W.square( P ); };

  SECTION( "petty_criminal" ) {
    auto f = [&] {
      return production_on_square( job, W.terrain(),
                                   e_unit_type::petty_criminal,
                                   Coord::from_gfx( P ) );
    };

    S()        = { .surface = e_surface::land };
    S().ground = e_ground_terrain::savannah;
    REQUIRE( f() == 0 );
    S().road = true;
    REQUIRE( f() == 0 );
    S().irrigation = true;
    REQUIRE( f() == 0 );
    S().river = e_river::major;
    REQUIRE( f() == 0 );
    S().river = e_river::minor;
    REQUIRE( f() == 0 );
    S().ground_resource = e_natural_resource::beaver;
    REQUIRE( f() == 0 );

    S()         = { .surface = e_surface::land };
    S().ground  = e_ground_terrain::savannah;
    S().overlay = e_land_overlay::forest;
    REQUIRE( f() == 2 );
    S().road = true;
    REQUIRE( f() == 4 );
    S().river = e_river::major;
    REQUIRE( f() == 8 );
    S().river = e_river::minor;
    REQUIRE( f() == 6 );
    S().irrigation = true;
    REQUIRE( f() == 6 );
    S().forest_resource = e_natural_resource::minerals;
    REQUIRE( f() == 6 );
    S().forest_resource = e_natural_resource::beaver;
    REQUIRE( f() == 9 );
    S().overlay = e_land_overlay::mountains;
    REQUIRE( f() == 0 );
    S().overlay = e_land_overlay::hills;
    REQUIRE( f() == 0 );
  }

  SECTION( "indentured_servant" ) {
    auto f = [&] {
      return production_on_square(
          job, W.terrain(), e_unit_type::indentured_servant,
          Coord::from_gfx( P ) );
    };

    S()        = { .surface = e_surface::land };
    S().ground = e_ground_terrain::savannah;
    REQUIRE( f() == 0 );
    S().road = true;
    REQUIRE( f() == 0 );
    S().irrigation = true;
    REQUIRE( f() == 0 );
    S().river = e_river::major;
    REQUIRE( f() == 0 );
    S().river = e_river::minor;
    REQUIRE( f() == 0 );
    S().ground_resource = e_natural_resource::beaver;
    REQUIRE( f() == 0 );

    S()         = { .surface = e_surface::land };
    S().ground  = e_ground_terrain::savannah;
    S().overlay = e_land_overlay::forest;
    REQUIRE( f() == 2 );
    S().road = true;
    REQUIRE( f() == 4 );
    S().river = e_river::major;
    REQUIRE( f() == 8 );
    S().river = e_river::minor;
    REQUIRE( f() == 6 );
    S().irrigation = true;
    REQUIRE( f() == 6 );
    S().forest_resource = e_natural_resource::minerals;
    REQUIRE( f() == 6 );
    S().forest_resource = e_natural_resource::beaver;
    REQUIRE( f() == 9 );
    S().overlay = e_land_overlay::mountains;
    REQUIRE( f() == 0 );
    S().overlay = e_land_overlay::hills;
    REQUIRE( f() == 0 );
  }

  SECTION( "expert" ) {
    auto f = [&] {
      return production_on_square(
          job, W.terrain(), e_unit_type::expert_fur_trapper,
          Coord::from_gfx( P ) );
    };

    S()        = { .surface = e_surface::land };
    S().ground = e_ground_terrain::savannah;
    REQUIRE( f() == 0 );
    S().road = true;
    REQUIRE( f() == 0 );
    S().irrigation = true;
    REQUIRE( f() == 0 );
    S().river = e_river::major;
    REQUIRE( f() == 0 );
    S().river = e_river::minor;
    REQUIRE( f() == 0 );
    S().ground_resource = e_natural_resource::beaver;
    REQUIRE( f() == 0 );

    S()         = { .surface = e_surface::land };
    S().ground  = e_ground_terrain::savannah;
    S().overlay = e_land_overlay::forest;
    REQUIRE( f() == 4 );
    S().road = true;
    REQUIRE( f() == 8 );
    S().river = e_river::major;
    REQUIRE( f() == 16 );
    S().river = e_river::minor;
    REQUIRE( f() == 12 );
    S().irrigation = true;
    REQUIRE( f() == 12 );
    S().forest_resource = e_natural_resource::minerals;
    REQUIRE( f() == 12 );
    S().forest_resource = e_natural_resource::beaver;
    REQUIRE( f() == 18 );
    S().overlay = e_land_overlay::mountains;
    REQUIRE( f() == 0 );
    S().overlay = e_land_overlay::hills;
    REQUIRE( f() == 0 );
  }
}

TEST_CASE( "[production] production_on_square/lumber" ) {
  World W;
  W.create_default_map();
  gfx::point P{ .x = 0, .y = 1 };

  e_outdoor_job const job = e_outdoor_job::lumber;

  auto S = [&]() -> decltype( auto ) { return W.square( P ); };

  SECTION( "petty_criminal" ) {
    auto f = [&] {
      return production_on_square( job, W.terrain(),
                                   e_unit_type::petty_criminal,
                                   Coord::from_gfx( P ) );
    };

    S()        = { .surface = e_surface::land };
    S().ground = e_ground_terrain::savannah;
    REQUIRE( f() == 0 );
    S().road = true;
    REQUIRE( f() == 0 );
    S().irrigation = true;
    REQUIRE( f() == 0 );
    S().river = e_river::major;
    REQUIRE( f() == 0 );
    S().river = e_river::minor;
    REQUIRE( f() == 0 );
    S().ground_resource = e_natural_resource::tree;
    REQUIRE( f() == 0 );

    S()         = { .surface = e_surface::land };
    S().ground  = e_ground_terrain::savannah;
    S().overlay = e_land_overlay::forest;
    REQUIRE( f() == 4 );
    S().road = true;
    REQUIRE( f() == 6 );
    S().river = e_river::major;
    REQUIRE( f() == 10 );
    S().river = e_river::minor;
    REQUIRE( f() == 8 );
    S().irrigation = true;
    REQUIRE( f() == 8 );
    S().forest_resource = e_natural_resource::minerals;
    REQUIRE( f() == 8 );
    S().forest_resource = e_natural_resource::tree;
    REQUIRE( f() == 12 );
    S().overlay = e_land_overlay::mountains;
    REQUIRE( f() == 0 );
    S().overlay = e_land_overlay::hills;
    REQUIRE( f() == 0 );
  }

  SECTION( "free_colonist" ) {
    auto f = [&] {
      return production_on_square( job, W.terrain(),
                                   e_unit_type::free_colonist,
                                   Coord::from_gfx( P ) );
    };

    S()        = { .surface = e_surface::land };
    S().ground = e_ground_terrain::savannah;
    REQUIRE( f() == 0 );
    S().road = true;
    REQUIRE( f() == 0 );
    S().irrigation = true;
    REQUIRE( f() == 0 );
    S().river = e_river::major;
    REQUIRE( f() == 0 );
    S().river = e_river::minor;
    REQUIRE( f() == 0 );
    S().ground_resource = e_natural_resource::tree;
    REQUIRE( f() == 0 );

    S()         = { .surface = e_surface::land };
    S().ground  = e_ground_terrain::savannah;
    S().overlay = e_land_overlay::forest;
    REQUIRE( f() == 4 );
    S().road = true;
    REQUIRE( f() == 6 );
    S().river = e_river::major;
    REQUIRE( f() == 10 );
    S().river = e_river::minor;
    REQUIRE( f() == 8 );
    S().irrigation = true;
    REQUIRE( f() == 8 );
    S().forest_resource = e_natural_resource::minerals;
    REQUIRE( f() == 8 );
    S().forest_resource = e_natural_resource::tree;
    REQUIRE( f() == 12 );
    S().overlay = e_land_overlay::mountains;
    REQUIRE( f() == 0 );
    S().overlay = e_land_overlay::hills;
    REQUIRE( f() == 0 );
  }

  SECTION( "expert" ) {
    auto f = [&] {
      return production_on_square(
          job, W.terrain(), e_unit_type::expert_lumberjack,
          Coord::from_gfx( P ) );
    };

    S()        = { .surface = e_surface::land };
    S().ground = e_ground_terrain::savannah;
    REQUIRE( f() == 0 );
    S().road = true;
    REQUIRE( f() == 0 );
    S().irrigation = true;
    REQUIRE( f() == 0 );
    S().river = e_river::major;
    REQUIRE( f() == 0 );
    S().river = e_river::minor;
    REQUIRE( f() == 0 );
    S().ground_resource = e_natural_resource::tree;
    REQUIRE( f() == 0 );

    S()         = { .surface = e_surface::land };
    S().ground  = e_ground_terrain::savannah;
    S().overlay = e_land_overlay::forest;
    REQUIRE( f() == 8 );
    S().road = true;
    REQUIRE( f() == 12 );
    S().river = e_river::major;
    REQUIRE( f() == 20 );
    S().river = e_river::minor;
    REQUIRE( f() == 16 );
    S().irrigation = true;
    REQUIRE( f() == 16 );
    S().forest_resource = e_natural_resource::minerals;
    REQUIRE( f() == 16 );
    S().forest_resource = e_natural_resource::tree;
    REQUIRE( f() == 24 );
    S().overlay = e_land_overlay::mountains;
    REQUIRE( f() == 0 );
    S().overlay = e_land_overlay::hills;
    REQUIRE( f() == 0 );
  }
}

TEST_CASE( "[production] production_on_square/ore" ) {
  World W;
  W.create_default_map();
  gfx::point P{ .x = 0, .y = 1 };

  e_outdoor_job const job = e_outdoor_job::ore;

  auto S = [&]() -> decltype( auto ) { return W.square( P ); };

  SECTION( "petty_criminal" ) {
    auto f = [&] {
      return production_on_square( job, W.terrain(),
                                   e_unit_type::petty_criminal,
                                   Coord::from_gfx( P ) );
    };

    S()        = { .surface = e_surface::land };
    S().ground = e_ground_terrain::savannah;
    REQUIRE( f() == 0 );
    S().road = true;
    REQUIRE( f() == 0 );
    S().irrigation = true;
    REQUIRE( f() == 0 );
    S().river = e_river::major;
    REQUIRE( f() == 0 );
    S().river = e_river::minor;
    REQUIRE( f() == 0 );
    S().ground_resource = e_natural_resource::ore;
    REQUIRE( f() == 0 );

    S()         = { .surface = e_surface::land };
    S().overlay = e_land_overlay::hills;
    REQUIRE( f() == 4 );
    S().road = true;
    REQUIRE( f() == 5 );
    S().river = e_river::major;
    REQUIRE( f() == 7 );
    S().river = e_river::minor;
    REQUIRE( f() == 6 );
    S().irrigation = true;
    REQUIRE( f() == 6 );
    S().ground_resource = e_natural_resource::tree;
    REQUIRE( f() == 6 );
    S().ground_resource = e_natural_resource::ore;
    REQUIRE( f() == 8 );
    S().ground_resource = e_natural_resource::minerals;
    REQUIRE( f() == 9 );
    S().ground_resource = nothing;
    S().overlay         = e_land_overlay::mountains;
    REQUIRE( f() == 6 );
  }

  SECTION( "free_colonist" ) {
    auto f = [&] {
      return production_on_square( job, W.terrain(),
                                   e_unit_type::free_colonist,
                                   Coord::from_gfx( P ) );
    };

    S()        = { .surface = e_surface::land };
    S().ground = e_ground_terrain::savannah;
    REQUIRE( f() == 0 );
    S().road = true;
    REQUIRE( f() == 0 );
    S().irrigation = true;
    REQUIRE( f() == 0 );
    S().river = e_river::major;
    REQUIRE( f() == 0 );
    S().river = e_river::minor;
    REQUIRE( f() == 0 );
    S().ground_resource = e_natural_resource::ore;
    REQUIRE( f() == 0 );

    S()         = { .surface = e_surface::land };
    S().overlay = e_land_overlay::hills;
    REQUIRE( f() == 4 );
    S().road = true;
    REQUIRE( f() == 5 );
    S().river = e_river::major;
    REQUIRE( f() == 7 );
    S().river = e_river::minor;
    REQUIRE( f() == 6 );
    S().irrigation = true;
    REQUIRE( f() == 6 );
    S().ground_resource = e_natural_resource::tree;
    REQUIRE( f() == 6 );
    S().ground_resource = e_natural_resource::ore;
    REQUIRE( f() == 8 );
    S().ground_resource = e_natural_resource::minerals;
    REQUIRE( f() == 9 );
    S().ground_resource = nothing;
    S().overlay         = e_land_overlay::mountains;
    REQUIRE( f() == 6 );
  }

  SECTION( "expert" ) {
    auto f = [&] {
      return production_on_square( job, W.terrain(),
                                   e_unit_type::expert_ore_miner,
                                   Coord::from_gfx( P ) );
    };

    S()        = { .surface = e_surface::land };
    S().ground = e_ground_terrain::savannah;
    REQUIRE( f() == 0 );
    S().road = true;
    REQUIRE( f() == 0 );
    S().irrigation = true;
    REQUIRE( f() == 0 );
    S().river = e_river::major;
    REQUIRE( f() == 0 );
    S().river = e_river::minor;
    REQUIRE( f() == 0 );
    S().ground_resource = e_natural_resource::ore;
    REQUIRE( f() == 0 );

    S()         = { .surface = e_surface::land };
    S().overlay = e_land_overlay::hills;
    REQUIRE( f() == 8 );
    S().road = true;
    REQUIRE( f() == 10 );
    S().river = e_river::major;
    REQUIRE( f() == 14 );
    S().river = e_river::minor;
    REQUIRE( f() == 12 );
    S().irrigation = true;
    REQUIRE( f() == 12 );
    S().ground_resource = e_natural_resource::tree;
    REQUIRE( f() == 12 );
    S().ground_resource = e_natural_resource::ore;
    REQUIRE( f() == 16 );
    S().ground_resource = e_natural_resource::minerals;
    REQUIRE( f() == 18 );
    S().ground_resource = nothing;
    S().overlay         = e_land_overlay::mountains;
    REQUIRE( f() == 12 );
  }
}

TEST_CASE( "[production] production_on_square/fish" ) {
  World W;
  W.create_default_map();

  e_outdoor_job const job = e_outdoor_job::fish;

  SECTION( "two land border squares" ) {
    gfx::point P{ .x = 3, .y = 1 };

    auto S = [&]() -> decltype( auto ) { return W.square( P ); };
    CHECK( S().surface == e_surface::water );

    SECTION( "petty_criminal" ) {
      auto f = [&] {
        return production_on_square( job, W.terrain(),
                                     e_unit_type::petty_criminal,
                                     Coord::from_gfx( P ) );
      };

      S() = { .surface = e_surface::land };
      REQUIRE( f() == 0 );
      S().road = true;
      REQUIRE( f() == 0 );
      S().irrigation = true;
      REQUIRE( f() == 0 );
      S().river = e_river::major;
      REQUIRE( f() == 0 );
      S().river = e_river::minor;
      REQUIRE( f() == 0 );
      S().ground_resource = e_natural_resource::fish;
      REQUIRE( f() == 0 );
      S().overlay = e_land_overlay::mountains;
      REQUIRE( f() == 0 );

      S() = { .surface = e_surface::water };
      REQUIRE( f() == 2 );
      S().road = true;
      REQUIRE( f() == 2 );
      S().river = e_river::major;
      REQUIRE( f() == 4 );
      S().river = e_river::minor;
      REQUIRE( f() == 3 );
      S().ground_resource = e_natural_resource::fish;
      REQUIRE( f() == 6 );
      S().overlay = e_land_overlay::mountains;
      REQUIRE( f() == 6 );
    }

    SECTION( "free_colonist" ) {
      auto f = [&] {
        return production_on_square( job, W.terrain(),
                                     e_unit_type::free_colonist,
                                     Coord::from_gfx( P ) );
      };

      S() = { .surface = e_surface::land };
      REQUIRE( f() == 0 );
      S().road = true;
      REQUIRE( f() == 0 );
      S().irrigation = true;
      REQUIRE( f() == 0 );
      S().river = e_river::major;
      REQUIRE( f() == 0 );
      S().river = e_river::minor;
      REQUIRE( f() == 0 );
      S().ground_resource = e_natural_resource::fish;
      REQUIRE( f() == 0 );
      S().overlay = e_land_overlay::mountains;
      REQUIRE( f() == 0 );

      S() = { .surface = e_surface::water };
      REQUIRE( f() == 2 );
      S().road = true;
      REQUIRE( f() == 2 );
      S().river = e_river::major;
      REQUIRE( f() == 4 );
      S().river = e_river::minor;
      REQUIRE( f() == 3 );
      S().ground_resource = e_natural_resource::fish;
      REQUIRE( f() == 6 );
      S().overlay = e_land_overlay::mountains;
      REQUIRE( f() == 6 );
    }

    SECTION( "expert" ) {
      auto f = [&] {
        return production_on_square(
            job, W.terrain(), e_unit_type::expert_fisherman,
            Coord::from_gfx( P ) );
      };

      S() = { .surface = e_surface::land };
      REQUIRE( f() == 0 );
      S().road = true;
      REQUIRE( f() == 0 );
      S().irrigation = true;
      REQUIRE( f() == 0 );
      S().river = e_river::major;
      REQUIRE( f() == 0 );
      S().river = e_river::minor;
      REQUIRE( f() == 0 );
      S().ground_resource = e_natural_resource::fish;
      REQUIRE( f() == 0 );
      S().overlay = e_land_overlay::mountains;
      REQUIRE( f() == 0 );

      S() = { .surface = e_surface::water };
      REQUIRE( f() == 4 );
      S().road = true;
      REQUIRE( f() == 4 );
      S().river = e_river::major;
      REQUIRE( f() == 6 );
      S().river = e_river::minor;
      REQUIRE( f() == 5 );
      S().ground_resource = e_natural_resource::fish;
      REQUIRE( f() == 11 );
      S().overlay = e_land_overlay::mountains;
      REQUIRE( f() == 11 );
    }
  }

  SECTION( "three land border squares" ) {
    gfx::point P{ .x = 2, .y = 0 };
    auto S = [&]() -> decltype( auto ) { return W.square( P ); };
    CHECK( S().surface == e_surface::water );

    SECTION( "petty_criminal" ) {
      auto f = [&] {
        return production_on_square( job, W.terrain(),
                                     e_unit_type::petty_criminal,
                                     Coord::from_gfx( P ) );
      };

      S() = { .surface = e_surface::land };
      REQUIRE( f() == 0 );
      S().road = true;
      REQUIRE( f() == 0 );
      S().irrigation = true;
      REQUIRE( f() == 0 );
      S().river = e_river::major;
      REQUIRE( f() == 0 );
      S().river = e_river::minor;
      REQUIRE( f() == 0 );
      S().ground_resource = e_natural_resource::fish;
      REQUIRE( f() == 0 );
      S().overlay = e_land_overlay::mountains;
      REQUIRE( f() == 0 );

      S() = { .surface = e_surface::water };
      REQUIRE( f() == 4 );
      S().road = true;
      REQUIRE( f() == 4 );
      S().river = e_river::major;
      REQUIRE( f() == 6 );
      S().river = e_river::minor;
      REQUIRE( f() == 5 );
      S().ground_resource = e_natural_resource::fish;
      REQUIRE( f() == 8 );
      S().overlay = e_land_overlay::mountains;
      REQUIRE( f() == 8 );
    }

    SECTION( "free_colonist" ) {
      auto f = [&] {
        return production_on_square( job, W.terrain(),
                                     e_unit_type::free_colonist,
                                     Coord::from_gfx( P ) );
      };

      S() = { .surface = e_surface::land };
      REQUIRE( f() == 0 );
      S().road = true;
      REQUIRE( f() == 0 );
      S().irrigation = true;
      REQUIRE( f() == 0 );
      S().river = e_river::major;
      REQUIRE( f() == 0 );
      S().river = e_river::minor;
      REQUIRE( f() == 0 );
      S().ground_resource = e_natural_resource::fish;
      REQUIRE( f() == 0 );
      S().overlay = e_land_overlay::mountains;
      REQUIRE( f() == 0 );

      S() = { .surface = e_surface::water };
      REQUIRE( f() == 4 );
      S().road = true;
      REQUIRE( f() == 4 );
      S().river = e_river::major;
      REQUIRE( f() == 6 );
      S().river = e_river::minor;
      REQUIRE( f() == 5 );
      S().ground_resource = e_natural_resource::fish;
      REQUIRE( f() == 8 );
      S().overlay = e_land_overlay::mountains;
      REQUIRE( f() == 8 );
    }

    SECTION( "expert" ) {
      auto f = [&] {
        return production_on_square(
            job, W.terrain(), e_unit_type::expert_fisherman,
            Coord::from_gfx( P ) );
      };

      S() = { .surface = e_surface::land };
      REQUIRE( f() == 0 );
      S().road = true;
      REQUIRE( f() == 0 );
      S().irrigation = true;
      REQUIRE( f() == 0 );
      S().river = e_river::major;
      REQUIRE( f() == 0 );
      S().river = e_river::minor;
      REQUIRE( f() == 0 );
      S().ground_resource = e_natural_resource::fish;
      REQUIRE( f() == 0 );
      S().overlay = e_land_overlay::mountains;
      REQUIRE( f() == 0 );

      S() = { .surface = e_surface::water };
      REQUIRE( f() == 6 );
      S().road = true;
      REQUIRE( f() == 6 );
      S().river = e_river::major;
      REQUIRE( f() == 8 );
      S().river = e_river::minor;
      REQUIRE( f() == 7 );
      S().ground_resource = e_natural_resource::fish;
      REQUIRE( f() == 13 );
      S().overlay = e_land_overlay::mountains;
      REQUIRE( f() == 13 );
    }
  }
}

} // namespace
} // namespace rn
