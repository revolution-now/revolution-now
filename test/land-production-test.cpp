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

// ss
#include "ss/player.hpp"

// refl
#include "refl/to-str.hpp"

// Must be last.
#include "test/catch-common.hpp"

// C++ standard library
#include <unordered_set>

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
      _, L, _, _, _, _,
      L, L, L, _, _, _,
      _, L, L, _, _, _,
    };
    // clang-format on
    build_map( std::move( tiles ), 6 );
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
           e_outdoor_job::furs );
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
  REQUIRE( f( e_outdoor_job::furs ) ==
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
      return production_on_square(
          job, W.terrain(), W.default_player().fathers.has,
          e_unit_type::petty_criminal, Coord::from_gfx( P ) );
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
      return production_on_square(
          job, W.terrain(), W.default_player().fathers.has,
          e_unit_type::free_colonist, Coord::from_gfx( P ) );
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

  SECTION( "native_convert" ) {
    auto f = [&] {
      return production_on_square(
          job, W.terrain(), W.default_player().fathers.has,
          e_unit_type::native_convert, Coord::from_gfx( P ) );
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
    REQUIRE( f() == 4 );
    S().road = true;
    REQUIRE( f() == 4 );
    S().river = e_river::major;
    REQUIRE( f() == 6 );
    S().river = e_river::minor;
    REQUIRE( f() == 5 );
    S().irrigation = true;
    REQUIRE( f() == 6 );
    S().ground_resource = e_natural_resource::cotton;
    REQUIRE( f() == 9 );
    S().overlay = e_land_overlay::forest;
    REQUIRE( f() == 3 );
  }

  SECTION( "expert" ) {
    auto f = [&] {
      return production_on_square(
          job, W.terrain(), W.default_player().fathers.has,
          e_unit_type::expert_cotton_planter,
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
      return production_on_square(
          job, W.terrain(), W.default_player().fathers.has,
          e_unit_type::petty_criminal, Coord::from_gfx( P ) );
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
      return production_on_square(
          job, W.terrain(), W.default_player().fathers.has,
          e_unit_type::free_colonist, Coord::from_gfx( P ) );
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

  SECTION( "native_convert" ) {
    auto f = [&] {
      return production_on_square(
          job, W.terrain(), W.default_player().fathers.has,
          e_unit_type::native_convert, Coord::from_gfx( P ) );
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
          job, W.terrain(), W.default_player().fathers.has,
          e_unit_type::expert_silver_miner,
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
      return production_on_square(
          job, W.terrain(), W.default_player().fathers.has,
          e_unit_type::petty_criminal, Coord::from_gfx( P ) );
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
      return production_on_square(
          job, W.terrain(), W.default_player().fathers.has,
          e_unit_type::free_colonist, Coord::from_gfx( P ) );
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

  SECTION( "native_convert" ) {
    auto f = [&] {
      return production_on_square(
          job, W.terrain(), W.default_player().fathers.has,
          e_unit_type::native_convert, Coord::from_gfx( P ) );
    };

    S()        = { .surface = e_surface::land };
    S().ground = e_ground_terrain::grassland;
    REQUIRE( f() == 4 );
    S().road = true;
    REQUIRE( f() == 4 );
    S().irrigation = true;
    REQUIRE( f() == 5 );
    S().river = e_river::major;
    REQUIRE( f() == 6 );
    S().river = e_river::minor;
    REQUIRE( f() == 6 );
    S().ground_resource = e_natural_resource::cotton;
    REQUIRE( f() == 6 );
    S().ground_resource = e_natural_resource::wheat;
    REQUIRE( f() == 8 );
    S().overlay = e_land_overlay::forest;
    REQUIRE( f() == 4 );
    S().forest_resource = e_natural_resource::deer;
    REQUIRE( f() == 6 );
    S().overlay = e_land_overlay::mountains;
    REQUIRE( f() == 0 );
    S().ground_resource = nothing;
    S().overlay         = e_land_overlay::hills;
    REQUIRE( f() == 4 );

    S()        = { .surface = e_surface::land };
    S().ground = e_ground_terrain::plains;
    REQUIRE( f() == 6 );
    S().road = true;
    REQUIRE( f() == 6 );
    S().river = e_river::major;
    REQUIRE( f() == 7 );
    S().river = e_river::minor;
    REQUIRE( f() == 7 );
    S().irrigation = true;
    REQUIRE( f() == 8 );
    S().ground_resource = e_natural_resource::cotton;
    REQUIRE( f() == 8 );
    S().ground_resource = e_natural_resource::wheat;
    REQUIRE( f() == 10 );
    S().overlay = e_land_overlay::forest;
    REQUIRE( f() == 5 );
    S().forest_resource = e_natural_resource::deer;
    REQUIRE( f() == 7 );
    S().overlay = e_land_overlay::mountains;
    REQUIRE( f() == 0 );
    S().ground_resource = nothing;
    S().overlay         = e_land_overlay::hills;
    REQUIRE( f() == 4 );
  }

  SECTION( "expert" ) {
    auto f = [&] {
      return production_on_square(
          job, W.terrain(), W.default_player().fathers.has,
          e_unit_type::expert_farmer, Coord::from_gfx( P ) );
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
      return production_on_square(
          job, W.terrain(), W.default_player().fathers.has,
          e_unit_type::petty_criminal, Coord::from_gfx( P ) );
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
      return production_on_square(
          job, W.terrain(), W.default_player().fathers.has,
          e_unit_type::free_colonist, Coord::from_gfx( P ) );
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

  SECTION( "native_convert" ) {
    auto f = [&] {
      return production_on_square(
          job, W.terrain(), W.default_player().fathers.has,
          e_unit_type::native_convert, Coord::from_gfx( P ) );
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
    REQUIRE( f() == 4 );
    S().road = true;
    REQUIRE( f() == 4 );
    S().river = e_river::major;
    REQUIRE( f() == 6 );
    S().river = e_river::minor;
    REQUIRE( f() == 5 );
    S().irrigation = true;
    REQUIRE( f() == 6 );
    S().ground_resource = e_natural_resource::cotton;
    REQUIRE( f() == 6 );
    S().ground_resource = e_natural_resource::sugar;
    REQUIRE( f() == 9 );
    S().overlay = e_land_overlay::forest;
    REQUIRE( f() == 3 );
    S().overlay = e_land_overlay::mountains;
    REQUIRE( f() == 0 );
    S().ground_resource = nothing;
    S().overlay         = e_land_overlay::hills;
    REQUIRE( f() == 0 );
  }

  SECTION( "expert" ) {
    auto f = [&] {
      return production_on_square(
          job, W.terrain(), W.default_player().fathers.has,
          e_unit_type::expert_sugar_planter,
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
      return production_on_square(
          job, W.terrain(), W.default_player().fathers.has,
          e_unit_type::petty_criminal, Coord::from_gfx( P ) );
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
      return production_on_square(
          job, W.terrain(), W.default_player().fathers.has,
          e_unit_type::free_colonist, Coord::from_gfx( P ) );
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

  SECTION( "native_convert" ) {
    auto f = [&] {
      return production_on_square(
          job, W.terrain(), W.default_player().fathers.has,
          e_unit_type::native_convert, Coord::from_gfx( P ) );
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
    REQUIRE( f() == 4 );
    S().road = true;
    REQUIRE( f() == 4 );
    S().river = e_river::major;
    REQUIRE( f() == 6 );
    S().river = e_river::minor;
    REQUIRE( f() == 5 );
    S().irrigation = true;
    REQUIRE( f() == 6 );
    S().ground_resource = e_natural_resource::cotton;
    REQUIRE( f() == 6 );
    S().ground_resource = e_natural_resource::tobacco;
    REQUIRE( f() == 9 );
    S().overlay = e_land_overlay::forest;
    REQUIRE( f() == 3 );
    S().overlay = e_land_overlay::mountains;
    REQUIRE( f() == 0 );
    S().ground_resource = nothing;
    S().overlay         = e_land_overlay::hills;
    REQUIRE( f() == 0 );
  }

  SECTION( "expert" ) {
    auto f = [&] {
      return production_on_square(
          job, W.terrain(), W.default_player().fathers.has,
          e_unit_type::expert_tobacco_planter,
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

TEST_CASE( "[production] production_on_square/furs" ) {
  World W;
  W.create_default_map();
  gfx::point P{ .x = 0, .y = 1 };

  e_outdoor_job const job = e_outdoor_job::furs;

  auto S = [&]() -> decltype( auto ) { return W.square( P ); };

  SECTION( "petty_criminal" ) {
    auto f = [&] {
      return production_on_square(
          job, W.terrain(), W.default_player().fathers.has,
          e_unit_type::petty_criminal, Coord::from_gfx( P ) );
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
          job, W.terrain(), W.default_player().fathers.has,
          e_unit_type::indentured_servant,
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

  SECTION( "native_convert" ) {
    auto f = [&] {
      return production_on_square(
          job, W.terrain(), W.default_player().fathers.has,
          e_unit_type::native_convert, Coord::from_gfx( P ) );
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
    REQUIRE( f() == 3 );
    S().road = true;
    REQUIRE( f() == 5 );
    S().river = e_river::major;
    REQUIRE( f() == 9 );
    S().river = e_river::minor;
    REQUIRE( f() == 7 );
    S().irrigation = true;
    REQUIRE( f() == 7 );
    S().forest_resource = e_natural_resource::minerals;
    REQUIRE( f() == 7 );
    S().forest_resource = e_natural_resource::beaver;
    REQUIRE( f() == 10 );
    S().overlay = e_land_overlay::mountains;
    REQUIRE( f() == 0 );
    S().overlay = e_land_overlay::hills;
    REQUIRE( f() == 0 );
  }

  SECTION( "expert" ) {
    auto f = [&] {
      return production_on_square(
          job, W.terrain(), W.default_player().fathers.has,
          e_unit_type::expert_fur_trapper,
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
      return production_on_square(
          job, W.terrain(), W.default_player().fathers.has,
          e_unit_type::petty_criminal, Coord::from_gfx( P ) );
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
      return production_on_square(
          job, W.terrain(), W.default_player().fathers.has,
          e_unit_type::free_colonist, Coord::from_gfx( P ) );
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

  SECTION( "native_convert" ) {
    auto f = [&] {
      return production_on_square(
          job, W.terrain(), W.default_player().fathers.has,
          e_unit_type::native_convert, Coord::from_gfx( P ) );
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
          job, W.terrain(), W.default_player().fathers.has,
          e_unit_type::expert_lumberjack, Coord::from_gfx( P ) );
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
    S().ground  = e_ground_terrain::grassland;
    S().overlay = e_land_overlay::forest;
    REQUIRE( f() == 12 );
    S().road = true;
    REQUIRE( f() == 16 );
    S().river = e_river::major;
    REQUIRE( f() == 24 );
    S().river = e_river::minor;
    REQUIRE( f() == 20 );
    S().irrigation = true;
    REQUIRE( f() == 20 );
    S().forest_resource = e_natural_resource::minerals;
    REQUIRE( f() == 20 );
    S().forest_resource = e_natural_resource::tree;
    REQUIRE( f() == 28 );
    S().river = e_river::major;
    REQUIRE( f() == 32 );
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
      return production_on_square(
          job, W.terrain(), W.default_player().fathers.has,
          e_unit_type::petty_criminal, Coord::from_gfx( P ) );
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
      return production_on_square(
          job, W.terrain(), W.default_player().fathers.has,
          e_unit_type::free_colonist, Coord::from_gfx( P ) );
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

  SECTION( "native_convert" ) {
    auto f = [&] {
      return production_on_square(
          job, W.terrain(), W.default_player().fathers.has,
          e_unit_type::native_convert, Coord::from_gfx( P ) );
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
      return production_on_square(
          job, W.terrain(), W.default_player().fathers.has,
          e_unit_type::expert_ore_miner, Coord::from_gfx( P ) );
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
        return production_on_square(
            job, W.terrain(), W.default_player().fathers.has,
            e_unit_type::petty_criminal, Coord::from_gfx( P ) );
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
        return production_on_square(
            job, W.terrain(), W.default_player().fathers.has,
            e_unit_type::free_colonist, Coord::from_gfx( P ) );
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

    SECTION( "native_convert" ) {
      auto f = [&] {
        return production_on_square(
            job, W.terrain(), W.default_player().fathers.has,
            e_unit_type::native_convert, Coord::from_gfx( P ) );
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
      REQUIRE( f() == 3 );
      S().road = true;
      REQUIRE( f() == 3 );
      S().river = e_river::major;
      REQUIRE( f() == 5 );
      S().river = e_river::minor;
      REQUIRE( f() == 4 );
      S().ground_resource = e_natural_resource::fish;
      REQUIRE( f() == 7 );
      S().overlay = e_land_overlay::mountains;
      REQUIRE( f() == 7 );
    }

    SECTION( "expert" ) {
      auto f = [&] {
        return production_on_square(
            job, W.terrain(), W.default_player().fathers.has,
            e_unit_type::expert_fisherman,
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
        return production_on_square(
            job, W.terrain(), W.default_player().fathers.has,
            e_unit_type::petty_criminal, Coord::from_gfx( P ) );
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
        return production_on_square(
            job, W.terrain(), W.default_player().fathers.has,
            e_unit_type::free_colonist, Coord::from_gfx( P ) );
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

    SECTION( "native_convert" ) {
      auto f = [&] {
        return production_on_square(
            job, W.terrain(), W.default_player().fathers.has,
            e_unit_type::native_convert, Coord::from_gfx( P ) );
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
      REQUIRE( f() == 5 );
      S().road = true;
      REQUIRE( f() == 5 );
      S().river = e_river::major;
      REQUIRE( f() == 7 );
      S().river = e_river::minor;
      REQUIRE( f() == 6 );
      S().ground_resource = e_natural_resource::fish;
      REQUIRE( f() == 9 );
      S().overlay = e_land_overlay::mountains;
      REQUIRE( f() == 9 );
    }

    SECTION( "expert" ) {
      auto f = [&] {
        return production_on_square(
            job, W.terrain(), W.default_player().fathers.has,
            e_unit_type::expert_fisherman,
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

TEST_CASE( "[production] food_production_on_center_square" ) {
  MapSquare const* square     = nullptr;
  e_difficulty     difficulty = {};
  int              expected   = 0;

  auto f = [&] {
    return food_production_on_center_square( *square,
                                             difficulty );
  };

  MapSquare const grassland =
      World::make_terrain( e_terrain::grassland );
  MapSquare const conifer =
      World::make_terrain( e_terrain::conifer );
  MapSquare const hills =
      World::make_terrain( e_terrain::hills );
  MapSquare const arctic =
      World::make_terrain( e_terrain::arctic );
  MapSquare const plains =
      World::make_terrain( e_terrain::plains );

  MapSquare plains_plowed  = plains;
  plains_plowed.irrigation = true;

  MapSquare plains_plowed_wheat = plains_plowed;
  plains_plowed_wheat.ground_resource =
      e_natural_resource::wheat;

  MapSquare plains_plowed_wheat_river = plains_plowed_wheat;
  plains_plowed_wheat_river.river     = e_river::minor;

  square     = &grassland;
  difficulty = e_difficulty::discoverer;
  expected   = 5;
  REQUIRE( f() == expected );

  square     = &grassland;
  difficulty = e_difficulty::explorer;
  expected   = 4;
  REQUIRE( f() == expected );

  square     = &grassland;
  difficulty = e_difficulty::conquistador;
  expected   = 3;
  REQUIRE( f() == expected );

  square     = &grassland;
  difficulty = e_difficulty::governor;
  expected   = 3;
  REQUIRE( f() == expected );

  square     = &grassland;
  difficulty = e_difficulty::viceroy;
  expected   = 3;
  REQUIRE( f() == expected );

  square     = &arctic;
  difficulty = e_difficulty::discoverer;
  expected   = 2;
  REQUIRE( f() == expected );

  square     = &arctic;
  difficulty = e_difficulty::explorer;
  expected   = 1;
  REQUIRE( f() == expected );

  square     = &arctic;
  difficulty = e_difficulty::conquistador;
  expected   = 0;
  REQUIRE( f() == expected );

  square     = &arctic;
  difficulty = e_difficulty::governor;
  expected   = 0;
  REQUIRE( f() == expected );

  square     = &arctic;
  difficulty = e_difficulty::viceroy;
  expected   = 0;
  REQUIRE( f() == expected );

  square     = &conifer;
  difficulty = e_difficulty::discoverer;
  expected   = 4;
  REQUIRE( f() == expected );

  square     = &conifer;
  difficulty = e_difficulty::explorer;
  expected   = 3;
  REQUIRE( f() == expected );

  square     = &conifer;
  difficulty = e_difficulty::conquistador;
  expected   = 2;
  REQUIRE( f() == expected );

  square     = &conifer;
  difficulty = e_difficulty::governor;
  expected   = 2;
  REQUIRE( f() == expected );

  square     = &conifer;
  difficulty = e_difficulty::viceroy;
  expected   = 2;
  REQUIRE( f() == expected );

  // This one differs from what it would be on a non-center
  // square.

  square     = &plains;
  difficulty = e_difficulty::discoverer;
  expected   = 5;
  REQUIRE( f() == expected );

  square     = &plains;
  difficulty = e_difficulty::explorer;
  expected   = 4;
  REQUIRE( f() == expected );

  square     = &plains;
  difficulty = e_difficulty::conquistador;
  expected   = 3;
  REQUIRE( f() == expected );

  square     = &plains;
  difficulty = e_difficulty::governor;
  expected   = 3;
  REQUIRE( f() == expected );

  square     = &plains;
  difficulty = e_difficulty::viceroy;
  expected   = 3;
  REQUIRE( f() == expected );

  square     = &plains_plowed;
  difficulty = e_difficulty::discoverer;
  expected   = 6;
  REQUIRE( f() == expected );

  square     = &plains_plowed;
  difficulty = e_difficulty::explorer;
  expected   = 5;
  REQUIRE( f() == expected );

  square     = &plains_plowed;
  difficulty = e_difficulty::conquistador;
  expected   = 4;
  REQUIRE( f() == expected );

  square     = &plains_plowed;
  difficulty = e_difficulty::governor;
  expected   = 4;
  REQUIRE( f() == expected );

  square     = &plains_plowed;
  difficulty = e_difficulty::viceroy;
  expected   = 4;
  REQUIRE( f() == expected );

  square     = &plains_plowed_wheat;
  difficulty = e_difficulty::discoverer;
  expected   = 8;
  REQUIRE( f() == expected );

  square     = &plains_plowed_wheat;
  difficulty = e_difficulty::explorer;
  expected   = 7;
  REQUIRE( f() == expected );

  square     = &plains_plowed_wheat;
  difficulty = e_difficulty::conquistador;
  expected   = 6;
  REQUIRE( f() == expected );

  square     = &plains_plowed_wheat;
  difficulty = e_difficulty::governor;
  expected   = 6;
  REQUIRE( f() == expected );

  square     = &plains_plowed_wheat;
  difficulty = e_difficulty::viceroy;
  expected   = 6;
  REQUIRE( f() == expected );

  square     = &plains_plowed_wheat_river;
  difficulty = e_difficulty::discoverer;
  expected   = 8;
  REQUIRE( f() == expected );

  square     = &plains_plowed_wheat_river;
  difficulty = e_difficulty::explorer;
  expected   = 7;
  REQUIRE( f() == expected );

  square     = &plains_plowed_wheat_river;
  difficulty = e_difficulty::conquistador;
  expected   = 6;
  REQUIRE( f() == expected );

  square     = &plains_plowed_wheat_river;
  difficulty = e_difficulty::governor;
  expected   = 6;
  REQUIRE( f() == expected );

  square     = &plains_plowed_wheat_river;
  difficulty = e_difficulty::viceroy;
  expected   = 6;
  REQUIRE( f() == expected );

  square     = &hills;
  difficulty = e_difficulty::discoverer;
  expected   = 4;
  REQUIRE( f() == expected );

  square     = &hills;
  difficulty = e_difficulty::explorer;
  expected   = 3;
  REQUIRE( f() == expected );

  square     = &hills;
  difficulty = e_difficulty::conquistador;
  expected   = 2;
  REQUIRE( f() == expected );

  square     = &hills;
  difficulty = e_difficulty::governor;
  expected   = 2;
  REQUIRE( f() == expected );

  square     = &hills;
  difficulty = e_difficulty::viceroy;
  expected   = 2;
  REQUIRE( f() == expected );
}

TEST_CASE(
    "[production] commodity_production_on_center_square" ) {
  World                           W;
  MapSquare const*                square     = nullptr;
  e_difficulty                    difficulty = {};
  e_outdoor_commons_secondary_job job        = {};
  int                             expected   = 0;

  auto f = [&] {
    return commodity_production_on_center_square(
        job, *square, W.default_player(), difficulty );
  };

  MapSquare const plains =
      World::make_terrain( e_terrain::plains );

  MapSquare plains_plowed  = plains;
  plains_plowed.irrigation = true;

  MapSquare plains_plowed_cotton = plains_plowed;
  plains_plowed_cotton.ground_resource =
      e_natural_resource::cotton;

  MapSquare plains_plowed_cotton_river = plains_plowed_cotton;
  plains_plowed_cotton_river.river     = e_river::minor;

  MapSquare plains_plowed_cotton_river_road =
      plains_plowed_cotton_river;
  plains_plowed_cotton_river.road = true;

  square     = &plains;
  difficulty = e_difficulty::discoverer;
  job        = e_outdoor_commons_secondary_job::cotton;
  expected   = 3;
  REQUIRE( f() == expected );

  square     = &plains;
  difficulty = e_difficulty::explorer;
  job        = e_outdoor_commons_secondary_job::furs;
  expected   = 0;
  REQUIRE( f() == expected );

  square     = &plains;
  difficulty = e_difficulty::conquistador;
  job        = e_outdoor_commons_secondary_job::ore;
  expected   = 1;
  REQUIRE( f() == expected );

  square     = &plains;
  difficulty = e_difficulty::governor;
  job        = e_outdoor_commons_secondary_job::sugar;
  expected   = 0;
  REQUIRE( f() == expected );

  square     = &plains;
  difficulty = e_difficulty::viceroy;
  job        = e_outdoor_commons_secondary_job::tobacco;
  expected   = 0;
  REQUIRE( f() == expected );

  square     = &plains_plowed;
  difficulty = e_difficulty::discoverer;
  job        = e_outdoor_commons_secondary_job::cotton;
  expected   = 3;
  REQUIRE( f() == expected );

  square     = &plains_plowed;
  difficulty = e_difficulty::explorer;
  job        = e_outdoor_commons_secondary_job::furs;
  expected   = 0;
  REQUIRE( f() == expected );

  square     = &plains_plowed;
  difficulty = e_difficulty::conquistador;
  job        = e_outdoor_commons_secondary_job::ore;
  expected   = 1;
  REQUIRE( f() == expected );

  square     = &plains_plowed;
  difficulty = e_difficulty::governor;
  job        = e_outdoor_commons_secondary_job::sugar;
  expected   = 0;
  REQUIRE( f() == expected );

  square     = &plains_plowed;
  difficulty = e_difficulty::viceroy;
  job        = e_outdoor_commons_secondary_job::tobacco;
  expected   = 0;
  REQUIRE( f() == expected );

  square     = &plains_plowed_cotton;
  difficulty = e_difficulty::discoverer;
  job        = e_outdoor_commons_secondary_job::cotton;
  expected   = 5;
  REQUIRE( f() == expected );

  square     = &plains_plowed_cotton;
  difficulty = e_difficulty::explorer;
  job        = e_outdoor_commons_secondary_job::furs;
  expected   = 0;
  REQUIRE( f() == expected );

  square     = &plains_plowed_cotton;
  difficulty = e_difficulty::conquistador;
  job        = e_outdoor_commons_secondary_job::ore;
  expected   = 1;
  REQUIRE( f() == expected );

  square     = &plains_plowed_cotton;
  difficulty = e_difficulty::governor;
  job        = e_outdoor_commons_secondary_job::sugar;
  expected   = 0;
  REQUIRE( f() == expected );

  square     = &plains_plowed_cotton;
  difficulty = e_difficulty::viceroy;
  job        = e_outdoor_commons_secondary_job::tobacco;
  expected   = 0;
  REQUIRE( f() == expected );

  square     = &plains_plowed_cotton_river;
  difficulty = e_difficulty::discoverer;
  job        = e_outdoor_commons_secondary_job::cotton;
  expected   = 6;
  REQUIRE( f() == expected );

  square     = &plains_plowed_cotton_river;
  difficulty = e_difficulty::explorer;
  job        = e_outdoor_commons_secondary_job::furs;
  expected   = 0;
  REQUIRE( f() == expected );

  square     = &plains_plowed_cotton_river;
  difficulty = e_difficulty::conquistador;
  job        = e_outdoor_commons_secondary_job::ore;
  expected   = 2;
  REQUIRE( f() == expected );

  square     = &plains_plowed_cotton_river;
  difficulty = e_difficulty::governor;
  job        = e_outdoor_commons_secondary_job::sugar;
  expected   = 0;
  REQUIRE( f() == expected );

  square     = &plains_plowed_cotton_river;
  difficulty = e_difficulty::viceroy;
  job        = e_outdoor_commons_secondary_job::tobacco;
  expected   = 0;
  REQUIRE( f() == expected );

  square     = &plains_plowed_cotton_river_road;
  difficulty = e_difficulty::discoverer;
  job        = e_outdoor_commons_secondary_job::cotton;
  expected   = 6;
  REQUIRE( f() == expected );

  square     = &plains_plowed_cotton_river_road;
  difficulty = e_difficulty::explorer;
  job        = e_outdoor_commons_secondary_job::furs;
  expected   = 0;
  REQUIRE( f() == expected );

  square     = &plains_plowed_cotton_river_road;
  difficulty = e_difficulty::conquistador;
  job        = e_outdoor_commons_secondary_job::ore;
  expected   = 2;
  REQUIRE( f() == expected );

  square     = &plains_plowed_cotton_river_road;
  difficulty = e_difficulty::governor;
  job        = e_outdoor_commons_secondary_job::sugar;
  expected   = 0;
  REQUIRE( f() == expected );

  square     = &plains_plowed_cotton_river_road;
  difficulty = e_difficulty::viceroy;
  job        = e_outdoor_commons_secondary_job::tobacco;
  expected   = 0;
  REQUIRE( f() == expected );
}

TEST_CASE( "[production] choose_secondary_job" ) {
  World W;
  using J = e_outdoor_commons_secondary_job;
  using T = e_terrain;

  auto m = World::make_terrain;

  e_difficulty const difficulty = GENERATE(
      e_difficulty::discoverer, e_difficulty::viceroy );
  INFO( fmt::format( "difficulty: {}", difficulty ) );

  auto f = [&]( MapSquare const& square ) {
    return choose_secondary_job( W.default_player(), square,
                                 difficulty );
  };

  REQUIRE( f( m( T::desert ) ) == J::ore );
  REQUIRE( f( m( T::scrub ) ) == J::furs );
  REQUIRE( f( m( T::grassland ) ) == J::tobacco );
  REQUIRE( f( m( T::conifer ) ) == J::furs );
  REQUIRE( f( m( T::marsh ) ) == J::tobacco );
  REQUIRE( f( m( T::wetland ) ) == J::furs );
  REQUIRE( f( m( T::plains ) ) == J::cotton );
  REQUIRE( f( m( T::mixed ) ) == J::furs );
  REQUIRE( f( m( T::prairie ) ) == J::cotton );
  REQUIRE( f( m( T::broadleaf ) ) == J::furs );
  REQUIRE( f( m( T::savannah ) ) == J::sugar );
  REQUIRE( f( m( T::tropical ) ) == J::furs );
  REQUIRE( f( m( T::swamp ) ) == J::sugar );
  REQUIRE( f( m( T::rain ) ) == J::sugar );
  REQUIRE( f( m( T::tundra ) ) == J::ore );
  REQUIRE( f( m( T::boreal ) ) == J::furs );
  REQUIRE( f( m( T::arctic ) ) == nothing );
  REQUIRE( f( m( T::hills ) ) == J::ore );
  REQUIRE( f( m( T::mountains ) ) == J::ore );
}

TEST_CASE(
    "[production] silver production on minerals squares" ) {
  World W;
  W.create_default_map();

  e_unit_type unit_type = e_unit_type::free_colonist;

  gfx::point const P{ .x = 1, .y = 1 };
  auto S = [&]() -> decltype( auto ) { return W.square( P ); };
  CHECK( S().surface == e_surface::land );

  auto f = [&] {
    return production_on_square(
        e_outdoor_job::silver, W.terrain(),
        W.default_player().fathers.has, unit_type,
        Coord::from_gfx( P ) );
  };

  S() = World::make_terrain( e_terrain::rain );

  unit_type = e_unit_type::free_colonist;
  REQUIRE( f() == 0 );

  unit_type = e_unit_type::expert_silver_miner;
  REQUIRE( f() == 0 );

  S()                 = World::make_terrain( e_terrain::rain );
  S().forest_resource = e_natural_resource::minerals;

  unit_type = e_unit_type::free_colonist;
  REQUIRE( f() == 1 );

  unit_type = e_unit_type::expert_silver_miner;
  REQUIRE( f() == 2 );
}

TEST_CASE( "[production] fur trappers with hudson" ) {
  World W;
  W.create_default_map();

  e_unit_type unit_type = e_unit_type::free_colonist;

  gfx::point const P{ .x = 1, .y = 1 };
  auto S = [&]() -> decltype( auto ) { return W.square( P ); };
  CHECK( S().surface == e_surface::land );

  auto f = [&] {
    return production_on_square(
        e_outdoor_job::furs, W.terrain(),
        W.default_player().fathers.has, unit_type,
        Coord::from_gfx( P ) );
  };

  auto g = [&] {
    return commodity_production_on_center_square(
        e_outdoor_commons_secondary_job::furs, S(),
        W.default_player(), e_difficulty::conquistador );
  };

  S() = World::make_terrain( e_terrain::grassland );

  REQUIRE( g() == 0 );

  unit_type = e_unit_type::free_colonist;
  REQUIRE( f() == 0 );

  unit_type = e_unit_type::expert_fur_trapper;
  REQUIRE( f() == 0 );

  S() = World::make_terrain( e_terrain::conifer );

  REQUIRE( g() == 2 );

  unit_type = e_unit_type::free_colonist;
  REQUIRE( f() == 2 );

  unit_type = e_unit_type::expert_fur_trapper;
  REQUIRE( f() == 4 );

  W.default_player()
      .fathers.has[e_founding_father::henry_hudson] = true;

  REQUIRE( g() == 4 );

  unit_type = e_unit_type::free_colonist;
  REQUIRE( f() == 4 );

  unit_type = e_unit_type::expert_fur_trapper;
  REQUIRE( f() == 8 );
}

} // namespace
} // namespace rn
