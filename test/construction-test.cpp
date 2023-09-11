/****************************************************************
**construction.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-20.
*
* Description: Unit tests for the src/construction.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/construction.hpp"

// Testing
#include "test/fake/world.hpp"
#include "test/mocks/igui.hpp"

// ss
#include "src/ss/player.hpp"
#include "src/ss/ref.hpp"

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
    add_default_player();
    create_default_map();
  }

  void create_default_map() {
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
    // clang-format off
    vector<MapSquare> tiles{
      _, L, _, L, L, L,
      L, L, L, L, L, L,
      _, L, L, L, L, L,
    };
    // clang-format on
    build_map( std::move( tiles ), 6 );
  }

  inline static constexpr Coord kBordersOcean =
      Coord{ .x = 1, .y = 1 };
  inline static constexpr Coord kNoBordersWaterButNoOcean =
      Coord{ .x = 2, .y = 1 };
  inline static constexpr Coord kNoBordersWater =
      Coord{ .x = 4, .y = 1 };
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[construction] select building, borders ocean" ) {
  World W;
  W.update_terrain_connectivity();
  Colony& colony = W.add_colony( World::kBordersOcean );
  wait<>  w      = make_wait<>();

  auto f = [&] {
    return select_colony_construction( W.ss(), W.ts(), colony );
  };

  REQUIRE( !colony.construction.has_value() );

  SECTION( "population=0, fathers=0" ) {
    ChoiceConfig const config{
        .msg               = "Select One",
        .options           = { ChoiceConfigOption{
                      .key = "none", .display_name = "(no production)" } },
        .sort              = false,
        .initial_selection = nothing };

    W.gui()
        .EXPECT__choice( config, e_input_required::no )
        .returns( make_wait<maybe<string>>( "none" ) );
    w = f();
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    REQUIRE( !colony.construction.has_value() );
  }

  SECTION( "population=1, fathers=0, some buildings" ) {
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    colony.buildings[e_colony_building::printing_press]   = true;
    colony.buildings[e_colony_building::docks]            = true;
    colony.buildings[e_colony_building::stable]           = true;
    colony.buildings[e_colony_building::blacksmiths_shop] = true;
    ChoiceConfig const config{
        .msg = "Select One",
        .options =
            { { .key          = "none",
                .display_name = "(no production)" },
              { .key = "armory",
                .display_name =
                    "Armory                  52 hammers" },
              { .key          = "fur_trading_post",
                .display_name = "Fur Trading Post        56 "
                                "hammers,  20 tools" },
              { .key          = "rum_distillery",
                .display_name = "Rum Distillery          64 "
                                "hammers,  20 tools" },
              { .key          = "tobacconists_shop",
                .display_name = "Tobacconist's Shop      64 "
                                "hammers,  20 tools" },
              { .key          = "weavers_shop",
                .display_name = "Weaver's Shop           64 "
                                "hammers,  20 tools" },
              { .key = "warehouse",
                .display_name =
                    "Warehouse               80 hammers" },
              { .key = "wagon_train",
                .display_name =
                    "Wagon Train             40 hammers" } },
        .sort              = false,
        .initial_selection = nothing };

    W.gui()
        .EXPECT__choice( config, e_input_required::no )
        .returns(
            make_wait<maybe<string>>( "tobacconists_shop" ) );
    w = f();
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    REQUIRE(
        colony.construction ==
        Construction::building{
            .what = e_colony_building::tobacconists_shop } );
  }

  SECTION(
      "population=1, fathers=0, some buildings, initial "
      "selection" ) {
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    colony.buildings[e_colony_building::printing_press]   = true;
    colony.buildings[e_colony_building::docks]            = true;
    colony.buildings[e_colony_building::stable]           = true;
    colony.buildings[e_colony_building::blacksmiths_shop] = true;
    colony.construction = Construction::building{
        .what = e_colony_building::rum_distillery };
    ChoiceConfig const config{
        .msg = "Select One",
        .options =
            { { .key          = "none",
                .display_name = "(no production)" },
              { .key = "armory",
                .display_name =
                    "Armory                  52 hammers" },
              { .key          = "fur_trading_post",
                .display_name = "Fur Trading Post        56 "
                                "hammers,  20 tools" },
              { .key          = "rum_distillery",
                .display_name = "Rum Distillery          64 "
                                "hammers,  20 tools" },
              { .key          = "tobacconists_shop",
                .display_name = "Tobacconist's Shop      64 "
                                "hammers,  20 tools" },
              { .key          = "weavers_shop",
                .display_name = "Weaver's Shop           64 "
                                "hammers,  20 tools" },
              { .key = "warehouse",
                .display_name =
                    "Warehouse               80 hammers" },
              { .key = "wagon_train",
                .display_name =
                    "Wagon Train             40 hammers" } },
        .sort              = false,
        .initial_selection = 3 };

    W.gui()
        .EXPECT__choice( config, e_input_required::no )
        .returns(
            make_wait<maybe<string>>( "tobacconists_shop" ) );
    w = f();
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    REQUIRE(
        colony.construction ==
        Construction::building{
            .what = e_colony_building::tobacconists_shop } );
  }

  SECTION( "population=8, fathers=0, some buildings" ) {
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    W.add_unit_indoors( colony.id, e_indoor_job::tools );
    W.add_unit_indoors( colony.id, e_indoor_job::tools );
    W.add_unit_indoors( colony.id, e_indoor_job::tools );
    W.add_unit_indoors( colony.id, e_indoor_job::hammers );
    W.add_unit_indoors( colony.id, e_indoor_job::hammers );
    colony.buildings[e_colony_building::printing_press]   = true;
    colony.buildings[e_colony_building::docks]            = true;
    colony.buildings[e_colony_building::stable]           = true;
    colony.buildings[e_colony_building::blacksmiths_shop] = true;
    ChoiceConfig const config{
        .msg = "Select One",
        .options =
            { { .key          = "none",
                .display_name = "(no production)" },
              { .key = "armory",
                .display_name =
                    "Armory                  52 hammers" },
              { .key          = "fur_trading_post",
                .display_name = "Fur Trading Post        56 "
                                "hammers,  20 tools" },
              { .key = "lumber_mill",
                .display_name =
                    "Lumber Mill             52 hammers" },
              { .key          = "rum_distillery",
                .display_name = "Rum Distillery          64 "
                                "hammers,  20 tools" },
              { .key          = "tobacconists_shop",
                .display_name = "Tobacconist's Shop      64 "
                                "hammers,  20 tools" },
              { .key          = "weavers_shop",
                .display_name = "Weaver's Shop           64 "
                                "hammers,  20 tools" },
              { .key          = "newspaper",
                .display_name = "Newspaper              120 "
                                "hammers,  50 tools" },
              { .key = "schoolhouse",
                .display_name =
                    "Schoolhouse             64 hammers" },
              { .key          = "drydock",
                .display_name = "Drydock                 80 "
                                "hammers,  50 tools" },
              { .key = "stockade",
                .display_name =
                    "Stockade                64 hammers" },
              { .key = "warehouse",
                .display_name =
                    "Warehouse               80 hammers" },
              { .key = "church",
                .display_name =
                    "Church                  64 hammers" },
              { .key = "wagon_train",
                .display_name =
                    "Wagon Train             40 hammers" } },
        .sort              = false,
        .initial_selection = nothing };

    W.gui()
        .EXPECT__choice( config, e_input_required::no )
        .returns(
            make_wait<maybe<string>>( "fur_trading_post" ) );
    w = f();
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    REQUIRE( colony.construction ==
             Construction::building{
                 .what = e_colony_building::fur_trading_post } );
  }

  SECTION( "population=8, fathers=all, some buildings" ) {
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    W.add_unit_indoors( colony.id, e_indoor_job::tools );
    W.add_unit_indoors( colony.id, e_indoor_job::tools );
    W.add_unit_indoors( colony.id, e_indoor_job::tools );
    W.add_unit_indoors( colony.id, e_indoor_job::hammers );
    W.add_unit_indoors( colony.id, e_indoor_job::hammers );
    colony.buildings[e_colony_building::printing_press]   = true;
    colony.buildings[e_colony_building::docks]            = true;
    colony.buildings[e_colony_building::stable]           = true;
    colony.buildings[e_colony_building::blacksmiths_shop] = true;
    for( e_founding_father father :
         refl::enum_values<e_founding_father> )
      W.default_player().fathers.has[father] = true;
    ChoiceConfig const config{
        .msg = "Select One",
        .options =
            { { .key          = "none",
                .display_name = "(no production)" },
              { .key = "armory",
                .display_name =
                    "Armory                  52 hammers" },
              { .key          = "fur_trading_post",
                .display_name = "Fur Trading Post        56 "
                                "hammers,  20 tools" },
              { .key = "lumber_mill",
                .display_name =
                    "Lumber Mill             52 hammers" },
              { .key          = "rum_distillery",
                .display_name = "Rum Distillery          64 "
                                "hammers,  20 tools" },
              { .key          = "tobacconists_shop",
                .display_name = "Tobacconist's Shop      64 "
                                "hammers,  20 tools" },
              { .key          = "weavers_shop",
                .display_name = "Weaver's Shop           64 "
                                "hammers,  20 tools" },
              { .key          = "iron_works",
                .display_name = "Iron Works             240 "
                                "hammers, 100 tools" },
              { .key          = "newspaper",
                .display_name = "Newspaper              120 "
                                "hammers,  50 tools" },
              { .key = "schoolhouse",
                .display_name =
                    "Schoolhouse             64 hammers" },
              { .key          = "drydock",
                .display_name = "Drydock                 80 "
                                "hammers,  50 tools" },
              { .key = "stockade",
                .display_name =
                    "Stockade                64 hammers" },
              { .key = "warehouse",
                .display_name =
                    "Warehouse               80 hammers" },
              { .key = "church",
                .display_name =
                    "Church                  64 hammers" },
              { .key          = "custom_house",
                .display_name = "Custom House           150 "
                                "hammers,  50 tools" },
              { .key = "wagon_train",
                .display_name =
                    "Wagon Train             40 hammers" } },
        .sort              = false,
        .initial_selection = nothing };

    W.gui()
        .EXPECT__choice( config, e_input_required::no )
        .returns( make_wait<maybe<string>>( "none" ) );
    w = f();
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    REQUIRE( !colony.construction.has_value() );
  }

  // required building
}

TEST_CASE(
    "[construction] select building, borders water but not "
    "ocean" ) {
  World W;
  W.update_terrain_connectivity();
  Colony& colony =
      W.add_colony( World::kNoBordersWaterButNoOcean );
  wait<> w = make_wait<>();

  auto f = [&] {
    return select_colony_construction( W.ss(), W.ts(), colony );
  };

  REQUIRE( !colony.construction.has_value() );

  SECTION( "population=0, fathers=0" ) {
    ChoiceConfig const config{
        .msg               = "Select One",
        .options           = { ChoiceConfigOption{
                      .key = "none", .display_name = "(no production)" } },
        .sort              = false,
        .initial_selection = nothing };

    W.gui()
        .EXPECT__choice( config, e_input_required::no )
        .returns( make_wait<maybe<string>>( "none" ) );
    w = f();
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    REQUIRE( !colony.construction.has_value() );
  }

  SECTION( "population=1, fathers=0, some buildings" ) {
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    colony.buildings[e_colony_building::printing_press]   = true;
    colony.buildings[e_colony_building::stable]           = true;
    colony.buildings[e_colony_building::blacksmiths_shop] = true;
    ChoiceConfig const config{
        .msg = "Select One",
        .options =
            { { .key          = "none",
                .display_name = "(no production)" },
              { .key = "armory",
                .display_name =
                    "Armory                  52 hammers" },
              { .key          = "fur_trading_post",
                .display_name = "Fur Trading Post        56 "
                                "hammers,  20 tools" },
              { .key          = "rum_distillery",
                .display_name = "Rum Distillery          64 "
                                "hammers,  20 tools" },
              { .key          = "tobacconists_shop",
                .display_name = "Tobacconist's Shop      64 "
                                "hammers,  20 tools" },
              { .key          = "weavers_shop",
                .display_name = "Weaver's Shop           64 "
                                "hammers,  20 tools" },
              { .key = "docks",
                .display_name =
                    "Docks                   52 hammers" },
              { .key = "warehouse",
                .display_name =
                    "Warehouse               80 hammers" },
              { .key = "wagon_train",
                .display_name =
                    "Wagon Train             40 hammers" } },
        .sort              = false,
        .initial_selection = nothing };

    W.gui()
        .EXPECT__choice( config, e_input_required::no )
        .returns(
            make_wait<maybe<string>>( "tobacconists_shop" ) );
    w = f();
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    REQUIRE(
        colony.construction ==
        Construction::building{
            .what = e_colony_building::tobacconists_shop } );
  }

  SECTION(
      "population=1, fathers=0, some buildings, initial "
      "selection" ) {
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    colony.buildings[e_colony_building::printing_press]   = true;
    colony.buildings[e_colony_building::docks]            = true;
    colony.buildings[e_colony_building::stable]           = true;
    colony.buildings[e_colony_building::blacksmiths_shop] = true;
    colony.construction = Construction::building{
        .what = e_colony_building::rum_distillery };
    ChoiceConfig const config{
        .msg = "Select One",
        .options =
            { { .key          = "none",
                .display_name = "(no production)" },
              { .key = "armory",
                .display_name =
                    "Armory                  52 hammers" },
              { .key          = "fur_trading_post",
                .display_name = "Fur Trading Post        56 "
                                "hammers,  20 tools" },
              { .key          = "rum_distillery",
                .display_name = "Rum Distillery          64 "
                                "hammers,  20 tools" },
              { .key          = "tobacconists_shop",
                .display_name = "Tobacconist's Shop      64 "
                                "hammers,  20 tools" },
              { .key          = "weavers_shop",
                .display_name = "Weaver's Shop           64 "
                                "hammers,  20 tools" },
              { .key = "warehouse",
                .display_name =
                    "Warehouse               80 hammers" },
              { .key = "wagon_train",
                .display_name =
                    "Wagon Train             40 hammers" } },
        .sort              = false,
        .initial_selection = 3 };

    W.gui()
        .EXPECT__choice( config, e_input_required::no )
        .returns(
            make_wait<maybe<string>>( "tobacconists_shop" ) );
    w = f();
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    REQUIRE(
        colony.construction ==
        Construction::building{
            .what = e_colony_building::tobacconists_shop } );
  }

  SECTION( "population=8, fathers=0, some buildings" ) {
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    W.add_unit_indoors( colony.id, e_indoor_job::tools );
    W.add_unit_indoors( colony.id, e_indoor_job::tools );
    W.add_unit_indoors( colony.id, e_indoor_job::tools );
    W.add_unit_indoors( colony.id, e_indoor_job::hammers );
    W.add_unit_indoors( colony.id, e_indoor_job::hammers );
    colony.buildings[e_colony_building::printing_press]   = true;
    colony.buildings[e_colony_building::docks]            = true;
    colony.buildings[e_colony_building::stable]           = true;
    colony.buildings[e_colony_building::blacksmiths_shop] = true;
    ChoiceConfig const config{
        .msg = "Select One",
        .options =
            { { .key          = "none",
                .display_name = "(no production)" },
              { .key = "armory",
                .display_name =
                    "Armory                  52 hammers" },
              { .key          = "fur_trading_post",
                .display_name = "Fur Trading Post        56 "
                                "hammers,  20 tools" },
              { .key = "lumber_mill",
                .display_name =
                    "Lumber Mill             52 hammers" },
              { .key          = "rum_distillery",
                .display_name = "Rum Distillery          64 "
                                "hammers,  20 tools" },
              { .key          = "tobacconists_shop",
                .display_name = "Tobacconist's Shop      64 "
                                "hammers,  20 tools" },
              { .key          = "weavers_shop",
                .display_name = "Weaver's Shop           64 "
                                "hammers,  20 tools" },
              { .key          = "newspaper",
                .display_name = "Newspaper              120 "
                                "hammers,  50 tools" },
              { .key = "schoolhouse",
                .display_name =
                    "Schoolhouse             64 hammers" },
              { .key = "stockade",
                .display_name =
                    "Stockade                64 hammers" },
              { .key = "warehouse",
                .display_name =
                    "Warehouse               80 hammers" },
              { .key = "church",
                .display_name =
                    "Church                  64 hammers" },
              { .key = "wagon_train",
                .display_name =
                    "Wagon Train             40 hammers" } },
        .sort              = false,
        .initial_selection = nothing };

    W.gui()
        .EXPECT__choice( config, e_input_required::no )
        .returns(
            make_wait<maybe<string>>( "fur_trading_post" ) );
    w = f();
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    REQUIRE( colony.construction ==
             Construction::building{
                 .what = e_colony_building::fur_trading_post } );
  }

  SECTION( "population=8, fathers=all, some buildings" ) {
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    W.add_unit_indoors( colony.id, e_indoor_job::tools );
    W.add_unit_indoors( colony.id, e_indoor_job::tools );
    W.add_unit_indoors( colony.id, e_indoor_job::tools );
    W.add_unit_indoors( colony.id, e_indoor_job::hammers );
    W.add_unit_indoors( colony.id, e_indoor_job::hammers );
    colony.buildings[e_colony_building::printing_press]   = true;
    colony.buildings[e_colony_building::docks]            = true;
    colony.buildings[e_colony_building::stable]           = true;
    colony.buildings[e_colony_building::blacksmiths_shop] = true;
    // This colony shouldn't normally have a drydock since it
    // does not have ocean access, but we'll give it one anyway
    // to make sure that the Shipyard is also suppressed due to
    // lack of ocean access (Shipyard requires Drydock).
    colony.buildings[e_colony_building::drydock] = true;
    for( e_founding_father father :
         refl::enum_values<e_founding_father> )
      W.default_player().fathers.has[father] = true;
    ChoiceConfig const config{
        .msg = "Select One",
        .options =
            { { .key          = "none",
                .display_name = "(no production)" },
              { .key = "armory",
                .display_name =
                    "Armory                  52 hammers" },
              { .key          = "fur_trading_post",
                .display_name = "Fur Trading Post        56 "
                                "hammers,  20 tools" },
              { .key = "lumber_mill",
                .display_name =
                    "Lumber Mill             52 hammers" },
              { .key          = "rum_distillery",
                .display_name = "Rum Distillery          64 "
                                "hammers,  20 tools" },
              { .key          = "tobacconists_shop",
                .display_name = "Tobacconist's Shop      64 "
                                "hammers,  20 tools" },
              { .key          = "weavers_shop",
                .display_name = "Weaver's Shop           64 "
                                "hammers,  20 tools" },
              { .key          = "iron_works",
                .display_name = "Iron Works             240 "
                                "hammers, 100 tools" },
              { .key          = "newspaper",
                .display_name = "Newspaper              120 "
                                "hammers,  50 tools" },
              { .key = "schoolhouse",
                .display_name =
                    "Schoolhouse             64 hammers" },
              { .key = "stockade",
                .display_name =
                    "Stockade                64 hammers" },
              { .key = "warehouse",
                .display_name =
                    "Warehouse               80 hammers" },
              { .key = "church",
                .display_name =
                    "Church                  64 hammers" },
              { .key          = "custom_house",
                .display_name = "Custom House           150 "
                                "hammers,  50 tools" },
              { .key = "wagon_train",
                .display_name =
                    "Wagon Train             40 hammers" } },
        .sort              = false,
        .initial_selection = nothing };

    W.gui()
        .EXPECT__choice( config, e_input_required::no )
        .returns( make_wait<maybe<string>>( "none" ) );
    w = f();
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    REQUIRE( !colony.construction.has_value() );
  }

  // required building
}

TEST_CASE(
    "[construction] select building, doesn't border water" ) {
  World   W;
  Colony& colony = W.add_colony( World::kNoBordersWater );
  wait<>  w      = make_wait<>();

  auto f = [&] {
    return select_colony_construction( W.ss(), W.ts(), colony );
  };

  REQUIRE( !colony.construction.has_value() );

  SECTION( "population=1, fathers=0, some buildings" ) {
    W.add_unit_indoors( colony.id, e_indoor_job::bells );
    colony.buildings[e_colony_building::printing_press]   = true;
    colony.buildings[e_colony_building::stable]           = true;
    colony.buildings[e_colony_building::blacksmiths_shop] = true;
    // docks should not appear despite the fact that we don't
    // have them.
    REQUIRE_FALSE( colony.buildings[e_colony_building::docks] );
    ChoiceConfig const config{
        .msg = "Select One",
        .options =
            { { .key          = "none",
                .display_name = "(no production)" },
              { .key = "armory",
                .display_name =
                    "Armory                  52 hammers" },
              { .key          = "fur_trading_post",
                .display_name = "Fur Trading Post        56 "
                                "hammers,  20 tools" },
              { .key          = "rum_distillery",
                .display_name = "Rum Distillery          64 "
                                "hammers,  20 tools" },
              { .key          = "tobacconists_shop",
                .display_name = "Tobacconist's Shop      64 "
                                "hammers,  20 tools" },
              { .key          = "weavers_shop",
                .display_name = "Weaver's Shop           64 "
                                "hammers,  20 tools" },
              { .key = "warehouse",
                .display_name =
                    "Warehouse               80 hammers" },
              { .key = "wagon_train",
                .display_name =
                    "Wagon Train             40 hammers" } },
        .sort              = false,
        .initial_selection = nothing };

    W.gui()
        .EXPECT__choice( config, e_input_required::no )
        .returns(
            make_wait<maybe<string>>( "tobacconists_shop" ) );
    w = f();
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    REQUIRE(
        colony.construction ==
        Construction::building{
            .what = e_colony_building::tobacconists_shop } );
  }
}

TEST_CASE( "[construction] rush_construction_cost" ) {
  World W;
  auto [colony, founder] =
      W.add_colony_with_new_unit( Coord{ .x = 1, .y = 1 } );
  Player& player = W.default_player();

  maybe<RushConstruction> expected;

  auto f = [&] {
    return rush_construction_cost( W.ss(), colony );
  };

  auto set_tools = [&]( int n ) {
    colony.commodities[e_commodity::tools] = n;
  };
  auto set_tools_ask = [&]( int ask ) {
    player.old_world.market.commodities[e_commodity::tools]
        .bid_price = ask - 1;
  };

  SECTION( "no construction" ) {
    expected = nothing;
    REQUIRE( f() == expected );
  }

  SECTION( "already has building" ) {
    colony.construction = Construction::building{
        .what = e_colony_building::docks };
    colony.buildings[e_colony_building::docks] = true;
    expected                                   = nothing;
    REQUIRE( f() == expected );
  }

  SECTION( "docks" ) {
    colony.construction = Construction::building{
        .what = e_colony_building::docks };

    colony.hammers = 0;
    expected =
        RushConstruction{ .project = colony.construction.value(),
                          .cost    = 52 * 26,
                          .total_hammers            = 52,
                          .total_tools              = 0,
                          .needed_hammers           = 52,
                          .needed_tools             = 0,
                          .blocked_by_tools_boycott = false };
    REQUIRE( f() == expected );

    colony.hammers = 10;
    expected =
        RushConstruction{ .project = colony.construction.value(),
                          .cost    = 42 * 26,
                          .total_hammers            = 52,
                          .total_tools              = 0,
                          .needed_hammers           = 42,
                          .needed_tools             = 0,
                          .blocked_by_tools_boycott = false };
    REQUIRE( f() == expected );

    colony.hammers = 51;
    expected =
        RushConstruction{ .project = colony.construction.value(),
                          .cost    = 1 * 26,
                          .total_hammers            = 52,
                          .total_tools              = 0,
                          .needed_hammers           = 1,
                          .needed_tools             = 0,
                          .blocked_by_tools_boycott = false };
    REQUIRE( f() == expected );

    colony.hammers = 52;
    expected =
        RushConstruction{ .project = colony.construction.value(),
                          .cost    = 0,
                          .total_hammers            = 52,
                          .total_tools              = 0,
                          .needed_hammers           = 0,
                          .needed_tools             = 0,
                          .blocked_by_tools_boycott = false };
    REQUIRE( f() == expected );

    colony.hammers = 100;
    expected =
        RushConstruction{ .project = colony.construction.value(),
                          .cost    = 0,
                          .total_hammers            = 52,
                          .total_tools              = 0,
                          .needed_hammers           = 0,
                          .needed_tools             = 0,
                          .blocked_by_tools_boycott = false };
    REQUIRE( f() == expected );
  }

  SECTION( "frigate" ) {
    colony.construction =
        Construction::unit{ .type = e_unit_type::frigate };

    colony.hammers = 0;
    set_tools( 0 );
    set_tools_ask( 2 );
    expected =
        RushConstruction{ .project = colony.construction.value(),
                          .cost = 512 * 26 + 200 * ( 8 + 2 * 2 ),
                          .total_hammers            = 512,
                          .total_tools              = 200,
                          .needed_hammers           = 512,
                          .needed_tools             = 200,
                          .blocked_by_tools_boycott = false };
    REQUIRE( f() == expected );

    colony.hammers = 100;
    set_tools( 0 );
    set_tools_ask( 2 );
    expected =
        RushConstruction{ .project = colony.construction.value(),
                          .cost = 412 * 26 + 200 * ( 8 + 2 * 2 ),
                          .total_hammers            = 512,
                          .total_tools              = 200,
                          .needed_hammers           = 412,
                          .needed_tools             = 200,
                          .blocked_by_tools_boycott = false };
    REQUIRE( f() == expected );

    colony.hammers = 0;
    set_tools( 100 );
    set_tools_ask( 2 );
    expected =
        RushConstruction{ .project = colony.construction.value(),
                          .cost = 512 * 26 + 100 * ( 8 + 2 * 2 ),
                          .total_hammers            = 512,
                          .total_tools              = 200,
                          .needed_hammers           = 512,
                          .needed_tools             = 100,
                          .blocked_by_tools_boycott = false };
    REQUIRE( f() == expected );

    colony.hammers = 0;
    set_tools( 100 );
    set_tools_ask( 6 );
    expected =
        RushConstruction{ .project = colony.construction.value(),
                          .cost = 512 * 26 + 100 * ( 8 + 2 * 6 ),
                          .total_hammers            = 512,
                          .total_tools              = 200,
                          .needed_hammers           = 512,
                          .needed_tools             = 100,
                          .blocked_by_tools_boycott = false };
    REQUIRE( f() == expected );

    colony.hammers = 200;
    set_tools( 100 );
    set_tools_ask( 6 );
    expected =
        RushConstruction{ .project = colony.construction.value(),
                          .cost = 312 * 26 + 100 * ( 8 + 2 * 6 ),
                          .total_hammers            = 512,
                          .total_tools              = 200,
                          .needed_hammers           = 312,
                          .needed_tools             = 100,
                          .blocked_by_tools_boycott = false };
    REQUIRE( f() == expected );

    colony.hammers = 511;
    set_tools( 100 );
    set_tools_ask( 6 );
    expected =
        RushConstruction{ .project = colony.construction.value(),
                          .cost = 1 * 26 + 100 * ( 8 + 2 * 6 ),
                          .total_hammers            = 512,
                          .total_tools              = 200,
                          .needed_hammers           = 1,
                          .needed_tools             = 100,
                          .blocked_by_tools_boycott = false };
    REQUIRE( f() == expected );

    colony.hammers = 512;
    set_tools( 100 );
    set_tools_ask( 6 );
    expected =
        RushConstruction{ .project = colony.construction.value(),
                          .cost = 0 * 26 + 100 * ( 8 + 2 * 6 ),
                          .total_hammers            = 512,
                          .total_tools              = 200,
                          .needed_hammers           = 0,
                          .needed_tools             = 100,
                          .blocked_by_tools_boycott = false };
    REQUIRE( f() == expected );

    colony.hammers = 600;
    set_tools( 199 );
    set_tools_ask( 3 );
    expected =
        RushConstruction{ .project = colony.construction.value(),
                          .cost    = 0 * 26 + 1 * ( 8 + 2 * 3 ),
                          .total_hammers            = 512,
                          .total_tools              = 200,
                          .needed_hammers           = 0,
                          .needed_tools             = 1,
                          .blocked_by_tools_boycott = false };
    REQUIRE( f() == expected );

    colony.hammers = 600;
    set_tools( 200 );
    set_tools_ask( 3 );
    expected =
        RushConstruction{ .project = colony.construction.value(),
                          .cost    = 0 * 26 + 0 * ( 8 + 2 * 3 ),
                          .total_hammers            = 512,
                          .total_tools              = 200,
                          .needed_hammers           = 0,
                          .needed_tools             = 0,
                          .blocked_by_tools_boycott = false };
    REQUIRE( f() == expected );

    colony.hammers = 600;
    set_tools( 300 );
    set_tools_ask( 3 );
    expected =
        RushConstruction{ .project = colony.construction.value(),
                          .cost    = 0,
                          .total_hammers            = 512,
                          .total_tools              = 200,
                          .needed_hammers           = 0,
                          .needed_tools             = 0,
                          .blocked_by_tools_boycott = false };
    REQUIRE( f() == expected );
  }
}

TEST_CASE( "[construction] rush_construction_prompt" ) {
  // FIXME
#ifdef COMPILER_GCC
  return;
#endif
  World W;
  auto [colony, founder] =
      W.add_colony_with_new_unit( Coord{ .x = 1, .y = 1 } );
  Player& player = W.default_player();

  Construction     project;
  RushConstruction invoice;
  string           msg;
  ChoiceConfig     config;

  auto f = [&] {
    return rush_construction_prompt( player, colony, W.gui(),
                                     invoice );
  };

  // With boycott.
  project = Construction::unit{ .type = e_unit_type::frigate };
  invoice = RushConstruction{ .project        = project,
                              .cost           = 100,
                              .total_hammers  = 20,
                              .total_tools    = 30,
                              .needed_hammers = 10,
                              .needed_tools   = 5,
                              .blocked_by_tools_boycott = true };
  player.money = 99;
  msg =
      "Rushing the construction of the [Frigate] requires "
      "acquiring [5 tools], but this is not allowed "
      "because tools are currently boycotted in Amsterdam.";
  W.gui().EXPECT__message_box( msg ).returns( make_wait<>() );
  {
    wait<> w = f();
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    REQUIRE( player.money == 99 );
    REQUIRE( colony.hammers == 0 );
    REQUIRE( colony.commodities[e_commodity::tools] == 0 );
  }

  // Can't afford.
  project = Construction::unit{ .type = e_unit_type::frigate };
  invoice =
      RushConstruction{ .project                  = project,
                        .cost                     = 100,
                        .total_hammers            = 20,
                        .total_tools              = 30,
                        .needed_hammers           = 10,
                        .needed_tools             = 5,
                        .blocked_by_tools_boycott = false };
  player.money = 99;
  msg = "Cost to complete [Frigate]: 100.  Treasury: 99.";
  W.gui().EXPECT__message_box( msg ).returns( make_wait<>() );
  {
    wait<> w = f();
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    REQUIRE( player.money == 99 );
    REQUIRE( colony.hammers == 0 );
    REQUIRE( colony.commodities[e_commodity::tools] == 0 );
  }

  // Can afford, escapes.
  project = Construction::unit{ .type = e_unit_type::frigate };
  invoice =
      RushConstruction{ .project                  = project,
                        .cost                     = 100,
                        .total_hammers            = 20,
                        .total_tools              = 30,
                        .needed_hammers           = 10,
                        .needed_tools             = 5,
                        .blocked_by_tools_boycott = false };
  player.money = 110;
  msg    = "Cost to complete [Frigate]: 100.  Treasury: 110.";
  config = ChoiceConfig{
      .msg     = msg,
      .options = {
          ChoiceConfigOption{ .key          = "no",
                              .display_name = "Never mind." },
          ChoiceConfigOption{
              .key = "yes", .display_name = "Complete it." } } };
  W.gui()
      .EXPECT__choice( config, e_input_required::no )
      .returns( make_wait<maybe<string>>( nothing ) );
  {
    wait<> w = f();
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    REQUIRE( player.money == 110 );
    REQUIRE( colony.hammers == 0 );
    REQUIRE( colony.commodities[e_commodity::tools] == 0 );
  }

  // Can afford, chooses no.
  project = Construction::unit{ .type = e_unit_type::frigate };
  invoice =
      RushConstruction{ .project                  = project,
                        .cost                     = 100,
                        .total_hammers            = 20,
                        .total_tools              = 30,
                        .needed_hammers           = 10,
                        .needed_tools             = 5,
                        .blocked_by_tools_boycott = false };
  player.money = 110;
  msg    = "Cost to complete [Frigate]: 100.  Treasury: 110.";
  config = ChoiceConfig{
      .msg     = msg,
      .options = {
          ChoiceConfigOption{ .key          = "no",
                              .display_name = "Never mind." },
          ChoiceConfigOption{
              .key = "yes", .display_name = "Complete it." } } };
  W.gui()
      .EXPECT__choice( config, e_input_required::no )
      .returns( make_wait<maybe<string>>( "no" ) );
  {
    wait<> w = f();
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    REQUIRE( player.money == 110 );
    REQUIRE( colony.hammers == 0 );
    REQUIRE( colony.commodities[e_commodity::tools] == 0 );
  }

  // Can afford, chooses yes.
  project = Construction::unit{ .type = e_unit_type::frigate };
  invoice =
      RushConstruction{ .project                  = project,
                        .cost                     = 100,
                        .total_hammers            = 20,
                        .total_tools              = 30,
                        .needed_hammers           = 10,
                        .needed_tools             = 5,
                        .blocked_by_tools_boycott = false };
  colony.hammers                         = 10;
  colony.commodities[e_commodity::tools] = 5;
  player.money                           = 110;
  msg    = "Cost to complete [Frigate]: 100.  Treasury: 110.";
  config = ChoiceConfig{
      .msg     = msg,
      .options = {
          ChoiceConfigOption{ .key          = "no",
                              .display_name = "Never mind." },
          ChoiceConfigOption{
              .key = "yes", .display_name = "Complete it." } } };
  W.gui()
      .EXPECT__choice( config, e_input_required::no )
      .returns( make_wait<maybe<string>>( "yes" ) );
  {
    wait<> w = f();
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    REQUIRE( player.money == 10 );
    REQUIRE( colony.hammers == 20 );
    REQUIRE( colony.commodities[e_commodity::tools] == 30 );
  }
}

} // namespace
} // namespace rn
