/****************************************************************
**fog-conv.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-18.
*
* Description: Unit tests for the src/fog-conv.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/fog-conv.hpp"

// Testing
#include "test/fake/world.hpp"

// ss
#include "src/ss/dwelling.rds.hpp"
#include "src/ss/players.rds.hpp"
#include "src/ss/ref.hpp"
#include "src/ss/unit-type.rds.hpp"
#include "src/ss/unit.hpp"

// refl
#include "src/refl/to-str.hpp"

// base
#include "src/base/to-str-ext-std.hpp"

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
  World() : Base() {
    add_player( e_nation::dutch );
    add_player( e_nation::spanish );
    add_player( e_nation::french );
    set_default_player( e_nation::dutch );
    create_default_map();
  }

  void create_default_map() {
    MapSquare const   _ = make_ocean();
    MapSquare const   L = make_grassland();
    vector<MapSquare> tiles{
      _, L, _, //
      L, L, L, //
      _, L, L, //
    };
    build_map( std::move( tiles ), 3 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[fog-conv] colony_to_frozen_colony" ) {
  World        W;
  FrozenColony expected = {};

  Colony& colony =
      W.add_colony( { .x = 1, .y = 1 }, e_nation::spanish );

  auto f = [&] {
    return colony_to_frozen_colony( W.ss(), colony );
  };

  colony.location = Coord{ .x = 1, .y = 1 };

  expected = FrozenColony{};
  REQUIRE( f() == expected );

  colony.indoor_jobs[e_indoor_job::bells] = { UnitId{ 1 },
                                              UnitId{ 2 } };
  colony.outdoor_jobs[e_direction::nw]    = OutdoorUnit{
       .unit_id = UnitId{ 3 }, .job = e_outdoor_job::food };

  expected =
      FrozenColony{ .sons_of_liberty_integral_percent = 67 };
  colony.sons_of_liberty.num_rebels_from_bells_only = 2.0;
  REQUIRE( f() == expected );

  W.spanish().fathers.has[e_founding_father::simon_bolivar] =
      true;
  expected =
      FrozenColony{ .sons_of_liberty_integral_percent = 87 };
  REQUIRE( f() == expected );
}

TEST_CASE( "[fog-conv] dwelling_to_frozen_dwelling" ) {
  World     W;
  Dwelling& dwelling =
      W.add_dwelling( { .x = 1, .y = 0 }, e_tribe::cherokee );
  FrozenDwelling expected;

  auto f = [&] {
    return dwelling_to_frozen_dwelling( W.ss(), dwelling );
  };

  expected = { .tribe    = e_tribe::cherokee,
               .location = { .x = 1, .y = 0 } };
  REQUIRE( f() == expected );

  dwelling.is_capital = true;
  expected            = { .tribe    = e_tribe::cherokee,
                          .location = { .x = 1, .y = 0 } };
  REQUIRE( f() == expected );

  Unit& missionary = W.add_missionary_in_dwelling(
      e_unit_type::jesuit_missionary, dwelling.id,
      e_nation::spanish );
  expected = { .tribe    = e_tribe::cherokee,
               .location = { .x = 1, .y = 0 },
               .mission  = FrozenMission{
                  .nation = e_nation::spanish,
                  .level  = e_missionary_type::jesuit } };
  REQUIRE( f() == expected );

  change_unit_type( W.ss(), W.ts(), missionary,
                    e_unit_type::missionary );
  expected = { .tribe    = e_tribe::cherokee,
               .location = { .x = 1, .y = 0 },
               .mission  = FrozenMission{
                  .nation = e_nation::spanish,
                  .level  = e_missionary_type::normal } };
  REQUIRE( f() == expected );

  change_unit_nation( W.ss(), W.ts(), missionary,
                      e_nation::french );
  expected = { .tribe    = e_tribe::cherokee,
               .location = { .x = 1, .y = 0 },
               .mission  = FrozenMission{
                  .nation = e_nation::french,
                  .level  = e_missionary_type::normal } };
  REQUIRE( f() == expected );
}

TEST_CASE( "[visibility] copy_real_square_to_frozen_square" ) {
  World        W;
  Coord        coord;
  FrozenSquare output, expected;

  auto f = [&] {
    copy_real_square_to_frozen_square( W.ss(), coord, output );
  };

  coord    = { .x = 0, .y = 0 };
  expected = {};
  f();
  REQUIRE( output == expected );

  coord    = { .x = 1, .y = 0 };
  expected = { .square = MapSquare{
                 .surface = e_surface::land,
                 .ground  = e_ground_terrain::grassland } };
  f();
  REQUIRE( output == expected );

  Dwelling& dwelling =
      W.add_dwelling( { .x = 1, .y = 0 }, e_tribe::cherokee );
  coord    = { .x = 1, .y = 0 };
  expected = {
    .square   = MapSquare{ .surface = e_surface::land,
                           .ground  = e_ground_terrain::grassland,
                           .road    = true },
    .dwelling = Dwelling{ .id         = 0,
                          .population = 5,
                          .frozen     = FrozenDwelling{
                                .tribe    = e_tribe::cherokee,
                                .location = { .x = 1, .y = 0 } } } };
  f();
  REQUIRE( output == expected );

  dwelling.is_capital = true;
  coord               = { .x = 1, .y = 0 };

  expected = {
    .square   = MapSquare{ .surface = e_surface::land,
                           .ground  = e_ground_terrain::grassland,
                           .road    = true },
    .dwelling = Dwelling{ .id         = 0,
                          .is_capital = true,
                          .population = 5,
                          .frozen     = FrozenDwelling{
                                .tribe    = e_tribe::cherokee,
                                .location = { .x = 1, .y = 0 } } } };
  f();
  REQUIRE( output == expected );

  coord    = { .x = 0, .y = 1 };
  expected = { .square = MapSquare{
                 .surface = e_surface::land,
                 .ground  = e_ground_terrain::grassland } };
  f();
  REQUIRE( output == expected );

  Colony& colony =
      W.add_colony( { .x = 0, .y = 1 }, e_nation::spanish );
  coord    = { .x = 0, .y = 1 };
  expected = {
    .square = MapSquare{ .surface = e_surface::land,
                         .ground  = e_ground_terrain::grassland,
                         .road    = true },
    .colony = Colony{ .id        = 0,
                      .nation    = e_nation::spanish,
                      .name      = "1",
                      .location  = { .x = 0, .y = 1 },
                      .buildings = colony.buildings,
                      .frozen    = FrozenColony{} } };
  f();
  REQUIRE( output == expected );

  colony.name = "hello";
  coord       = { .x = 0, .y = 1 };
  expected    = {
       .square = MapSquare{ .surface = e_surface::land,
                            .ground  = e_ground_terrain::grassland,
                            .road    = true },
       .colony = Colony{ .id        = 0,
                         .nation    = e_nation::spanish,
                         .name      = "hello",
                         .location  = { .x = 0, .y = 1 },
                         .buildings = colony.buildings,
                         .frozen    = FrozenColony{} } };
  f();
  REQUIRE( output == expected );

  coord    = { .x = 1, .y = 0 };
  expected = {
    .square   = MapSquare{ .surface = e_surface::land,
                           .ground  = e_ground_terrain::grassland,
                           .road    = true },
    .dwelling = Dwelling{ .id         = 0,
                          .is_capital = true,
                          .population = 5,
                          .frozen     = FrozenDwelling{
                                .tribe    = e_tribe::cherokee,
                                .location = { .x = 1, .y = 0 } } } };
  f();
  REQUIRE( output == expected );

  coord    = { .x = 1, .y = 0 };
  expected = {
    .square   = MapSquare{ .surface = e_surface::land,
                           .ground  = e_ground_terrain::grassland,
                           .road    = true },
    .dwelling = Dwelling{ .id         = 0,
                          .is_capital = true,
                          .population = 5,
                          .frozen     = FrozenDwelling{
                                .tribe    = e_tribe::cherokee,
                                .location = { .x = 1, .y = 0 } } } };
  f();
  REQUIRE( output == expected );
}

} // namespace
} // namespace rn
