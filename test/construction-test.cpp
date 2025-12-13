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

// config
#include "src/config/options.rds.hpp"

// ss
#include "src/ss/old-world-state.rds.hpp"
#include "src/ss/player.hpp"
#include "src/ss/ref.hpp"
#include "src/ss/settings.rds.hpp"
#include "src/ss/unit-composition.hpp"
#include "src/ss/units.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

using ::gfx::point;
using ::refl::enum_values;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct world : testing::World {
  using Base = testing::World;
  world() : Base() {
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
  world W;
  Colony& colony = W.add_colony( world::kBordersOcean );
  wait<> w       = make_wait<>();

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

    W.gui().EXPECT__choice( config ).returns(
        make_wait<maybe<string>>( "none" ) );
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
          { { .key = "none", .display_name = "(no production)" },
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

    W.gui().EXPECT__choice( config ).returns(
        make_wait<maybe<string>>( "tobacconists_shop" ) );
    w = f();
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    REQUIRE( colony.construction ==
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
          { { .key = "none", .display_name = "(no production)" },
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

    W.gui().EXPECT__choice( config ).returns(
        make_wait<maybe<string>>( "tobacconists_shop" ) );
    w = f();
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    REQUIRE( colony.construction ==
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
          { { .key = "none", .display_name = "(no production)" },
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

    W.gui().EXPECT__choice( config ).returns(
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
          { { .key = "none", .display_name = "(no production)" },
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
              .display_name = "Custom House           160 "
                              "hammers,  50 tools" },
            { .key = "wagon_train",
              .display_name =
                  "Wagon Train             40 hammers" } },
      .sort              = false,
      .initial_selection = nothing };

    W.gui().EXPECT__choice( config ).returns(
        make_wait<maybe<string>>( "none" ) );
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
  world W;
  Colony& colony =
      W.add_colony( world::kNoBordersWaterButNoOcean );
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

    W.gui().EXPECT__choice( config ).returns(
        make_wait<maybe<string>>( "none" ) );
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
          { { .key = "none", .display_name = "(no production)" },
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

    W.gui().EXPECT__choice( config ).returns(
        make_wait<maybe<string>>( "tobacconists_shop" ) );
    w = f();
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    REQUIRE( colony.construction ==
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
          { { .key = "none", .display_name = "(no production)" },
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

    W.gui().EXPECT__choice( config ).returns(
        make_wait<maybe<string>>( "tobacconists_shop" ) );
    w = f();
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    REQUIRE( colony.construction ==
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
          { { .key = "none", .display_name = "(no production)" },
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

    W.gui().EXPECT__choice( config ).returns(
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
          { { .key = "none", .display_name = "(no production)" },
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
              .display_name = "Custom House           160 "
                              "hammers,  50 tools" },
            { .key = "wagon_train",
              .display_name =
                  "Wagon Train             40 hammers" } },
      .sort              = false,
      .initial_selection = nothing };

    W.gui().EXPECT__choice( config ).returns(
        make_wait<maybe<string>>( "none" ) );
    w = f();
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    REQUIRE( !colony.construction.has_value() );
  }

  // required building
}

TEST_CASE(
    "[construction] select building, doesn't border water" ) {
  world W;
  Colony& colony = W.add_colony( world::kNoBordersWater );
  wait<> w       = make_wait<>();

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
          { { .key = "none", .display_name = "(no production)" },
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

    W.gui().EXPECT__choice( config ).returns(
        make_wait<maybe<string>>( "tobacconists_shop" ) );
    w = f();
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    REQUIRE( colony.construction ==
             Construction::building{
               .what = e_colony_building::tobacconists_shop } );
  }
}

TEST_CASE( "[construction] rush_construction_cost" ) {
  world W;
  auto [colony, founder] =
      W.found_colony_with_new_unit( Coord{ .x = 1, .y = 1 } );
  Player& player = W.default_player();

  maybe<RushConstruction> expected;

  auto f = [&] {
    return rush_construction_cost( W.ss(), colony );
  };

  auto set_tools = [&]( int n ) {
    colony.commodities[e_commodity::tools] = n;
  };
  auto set_tools_ask = [&]( int ask ) {
    W.old_world( player )
        .market.commodities[e_commodity::tools]
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
                          .cost    = 52 * 13 * 2,
                          .total_hammers            = 52,
                          .total_tools              = 0,
                          .needed_hammers           = 52,
                          .needed_tools             = 0,
                          .blocked_by_tools_boycott = false };
    REQUIRE( f() == expected );

    colony.hammers = 10;
    expected =
        RushConstruction{ .project = colony.construction.value(),
                          .cost    = 42 * 13,
                          .total_hammers            = 52,
                          .total_tools              = 0,
                          .needed_hammers           = 42,
                          .needed_tools             = 0,
                          .blocked_by_tools_boycott = false };
    REQUIRE( f() == expected );

    colony.hammers = 51;
    expected =
        RushConstruction{ .project = colony.construction.value(),
                          .cost    = 1 * 13,
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
    expected = RushConstruction{
      .project        = colony.construction.value(),
      .cost           = ( 512 * 13 + 200 * ( 4 + 2 ) ) * 2,
      .total_hammers  = 512,
      .total_tools    = 200,
      .needed_hammers = 512,
      .needed_tools   = 200,
      .blocked_by_tools_boycott = false };
    REQUIRE( f() == expected );

    colony.hammers = 100;
    set_tools( 0 );
    set_tools_ask( 2 );
    expected =
        RushConstruction{ .project = colony.construction.value(),
                          .cost    = 412 * 13 + 200 * ( 4 + 2 ),
                          .total_hammers            = 512,
                          .total_tools              = 200,
                          .needed_hammers           = 412,
                          .needed_tools             = 200,
                          .blocked_by_tools_boycott = false };
    REQUIRE( f() == expected );

    colony.hammers = 0;
    set_tools( 100 );
    set_tools_ask( 2 );
    expected = RushConstruction{
      .project        = colony.construction.value(),
      .cost           = ( 512 * 13 + 100 * ( 4 + 2 ) ) * 2,
      .total_hammers  = 512,
      .total_tools    = 200,
      .needed_hammers = 512,
      .needed_tools   = 100,
      .blocked_by_tools_boycott = false };
    REQUIRE( f() == expected );

    colony.hammers = 0;
    set_tools( 100 );
    set_tools_ask( 6 );
    expected = RushConstruction{
      .project        = colony.construction.value(),
      .cost           = ( 512 * 13 + 100 * ( 4 + 6 ) ) * 2,
      .total_hammers  = 512,
      .total_tools    = 200,
      .needed_hammers = 512,
      .needed_tools   = 100,
      .blocked_by_tools_boycott = false };
    REQUIRE( f() == expected );

    colony.hammers = 200;
    set_tools( 100 );
    set_tools_ask( 6 );
    expected =
        RushConstruction{ .project = colony.construction.value(),
                          .cost    = 312 * 13 + 100 * ( 4 + 6 ),
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
                          .cost    = 1 * 13 + 100 * ( 4 + 6 ),
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
                          .cost    = 0 * 13 + 100 * ( 4 + 6 ),
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
                          .cost    = 0 * 13 + 1 * ( 4 + 3 ),
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
                          .cost    = 0 * 13 + 0 * ( 4 + 3 ),
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
  world W;
  auto [colony, founder] =
      W.found_colony_with_new_unit( Coord{ .x = 1, .y = 1 } );
  Player& player = W.default_player();

  Construction project;
  RushConstruction invoice;
  string msg;
  ChoiceConfig config;

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
      ChoiceConfigOption{ .key          = "yes",
                          .display_name = "Complete it." } } };
  W.gui().EXPECT__choice( config ).returns(
      make_wait<maybe<string>>( nothing ) );
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
      ChoiceConfigOption{ .key          = "yes",
                          .display_name = "Complete it." } } };
  W.gui().EXPECT__choice( config ).returns(
      make_wait<maybe<string>>( "no" ) );
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
      ChoiceConfigOption{ .key          = "yes",
                          .display_name = "Complete it." } } };
  W.gui().EXPECT__choice( config ).returns(
      make_wait<maybe<string>>( "yes" ) );
  {
    wait<> w = f();
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    REQUIRE( player.money == 10 );
    REQUIRE( colony.hammers == 20 );
    REQUIRE( colony.commodities[e_commodity::tools] == 30 );
  }
}

TEST_WORLD( "[construction] wagon_train_limit_exceeded" ) {
  auto const exceeded = [&] [[clang::noinline]] {
    return wagon_train_limit_exceeded( ss(), default_player() );
  };
  auto const not_exceeded = [&] [[clang::noinline]] {
    return !exceeded();
  };

  auto& mode = settings()
                   .game_setup_options.customized_rules
                   .wagon_train_limit_mode;
  using enum config::options::e_wagon_train_limit_mode;

  // _, L, _, L, L, L,
  // L, L, L, L, L, L,
  // _, L, L, L, L, L,
  static vector<point> const kColonyTileList = {
    { .x = 5, .y = 2 }, //
    { .x = 5, .y = 0 }, //
    { .x = 3, .y = 2 }, //
    { .x = 3, .y = 0 }, //
    { .x = 1, .y = 2 }, //
    { .x = 1, .y = 0 }, //
  };
  auto add_colony = [this,
                     list = kColonyTileList] mutable -> Colony& {
    BASE_CHECK( !list.empty(), "ran out of colony tiles" );
    Colony& colony = this->add_colony( list.back() );
    list.pop_back();
    return colony;
  };

  auto const add_population = [&]( Colony& colony ) {
    for( e_direction const d : enum_values<e_direction> ) {
      if( colony.outdoor_jobs[d].has_value() ) continue;
      add_unit_outdoors( colony.id, d, e_outdoor_job::food );
      return;
    }
    for( e_indoor_job const job : enum_values<e_indoor_job> ) {
      if( !colony.indoor_jobs[job].empty() ) continue;
      add_unit_indoors( colony.id, job );
      return;
    }
    FATAL( "cannot find a job for colonist" );
  };

  auto const add_wagon_train = [&] {
    add_unit_on_map( e_unit_type::wagon_train,
                     { .x = 1, .y = 1 } );
  };

  // Verify default is OG behavior.
  REQUIRE( mode == classic );

  // Default.
  REQUIRE( exceeded() );

  SECTION( "limit mode=classic" ) {
    mode = classic;
    REQUIRE( exceeded() );
    Colony& c1 = add_colony();
    REQUIRE( not_exceeded() );
    add_wagon_train();
    REQUIRE( exceeded() );
    add_population( c1 );
    add_population( c1 );
    add_population( c1 );
    add_population( c1 );
    add_population( c1 );
    add_population( c1 );
    add_population( c1 );
    add_population( c1 );
    add_population( c1 );
    add_population( c1 );
    add_population( c1 );
    add_population( c1 );
    add_population( c1 );
    add_population( c1 );
    REQUIRE( exceeded() );
    add_colony();
    REQUIRE( not_exceeded() );
    add_wagon_train();
    REQUIRE( exceeded() );
    add_colony();
    REQUIRE( not_exceeded() );
    add_wagon_train();
    REQUIRE( exceeded() );
  }

  SECTION( "limit mode=population" ) {
    mode = population;
    REQUIRE( not_exceeded() );
    add_wagon_train();
    REQUIRE( exceeded() );
    add_colony();
    add_colony();
    add_colony();
    add_colony();
    add_colony();
    Colony& c1 = add_colony();
    REQUIRE( exceeded() );
    add_population( c1 );
    REQUIRE( exceeded() );
    add_population( c1 );
    REQUIRE( exceeded() );
    add_population( c1 );
    REQUIRE( exceeded() );
    add_population( c1 );
    REQUIRE( not_exceeded() );
    add_wagon_train();
    REQUIRE( exceeded() );
    add_population( c1 );
    REQUIRE( exceeded() );
    add_population( c1 );
    REQUIRE( exceeded() );
    add_population( c1 );
    REQUIRE( exceeded() );
    add_population( c1 );
    REQUIRE( not_exceeded() );
  }

  SECTION( "limit mode=none" ) {
    mode = none;
    REQUIRE( not_exceeded() );
    add_wagon_train();
    REQUIRE( not_exceeded() );
    add_wagon_train();
    REQUIRE( not_exceeded() );
    add_wagon_train();
    REQUIRE( not_exceeded() );
    add_wagon_train();
    REQUIRE( not_exceeded() );
    add_wagon_train();
    REQUIRE( not_exceeded() );
    add_wagon_train();
    REQUIRE( not_exceeded() );
    add_wagon_train();
    REQUIRE( not_exceeded() );
    add_wagon_train();
    REQUIRE( not_exceeded() );
    add_wagon_train();
    REQUIRE( not_exceeded() );
    add_wagon_train();
    REQUIRE( not_exceeded() );
    add_wagon_train();
    REQUIRE( not_exceeded() );
  }
}

TEST_CASE( "[construction] evolve_colony_construction" ) {
  world w;
  ColonyBuilt expected;
  vector<ColonyNotification> notifications;
  vector<ColonyNotification> expected_notifications;

  Colony& colony = w.add_colony( { .x = 1, .y = 1 } );

  auto const f = [&] [[clang::noinline]] {
    notifications.clear();
    return evolve_colony_construction( w.ss(), w.ts(),
                                       w.default_player(),
                                       colony, notifications );
  };

  using N = ColonyNotification;

  // Default.
  expected_notifications = {};
  REQUIRE( f() == nothing );
  REQUIRE( notifications == expected_notifications );

  // Already has docks (no carpenters).
  colony.buildings[e_colony_building::docks] = true;
  colony.construction =
      Construction::building{ .what = e_colony_building::docks };
  expected_notifications = {};
  REQUIRE( f() == nothing );
  REQUIRE( notifications == expected_notifications );
  REQUIRE( colony.buildings[e_colony_building::docks] == true );
  colony.buildings[e_colony_building::docks] = false;

  // Already has docks (with carpenters).
  colony.buildings[e_colony_building::docks] = true;
  w.add_unit_indoors( colony.id, e_indoor_job::hammers );
  colony.construction =
      Construction::building{ .what = e_colony_building::docks };
  expected_notifications = {
    N::construction_already_finished{
      .what =
          Construction::building{
            .what = e_colony_building::docks } },
  };
  REQUIRE( f() == nothing );
  REQUIRE( notifications == expected_notifications );
  REQUIRE( colony.buildings[e_colony_building::docks] == true );
  colony.buildings[e_colony_building::docks] = false;

  // Lacking population.
  colony.construction = Construction::building{
    .what = e_colony_building::stockade };
  expected_notifications = {
    N::construction_lacking_population{
      .what =
          Construction::building{
            .what = e_colony_building::stockade },
      .required_population = 3 },
  };
  REQUIRE( f() == nothing );
  REQUIRE( notifications == expected_notifications );
  REQUIRE( colony.buildings[e_colony_building::stockade] ==
           false );

  // Lacking buiding.
  colony.construction = Construction::building{
    .what = e_colony_building::warehouse_expansion };
  expected_notifications = {
    N::construction_lacking_building{
      .what =
          Construction::building{
            .what = e_colony_building::warehouse_expansion },
      .required_building = e_colony_building::warehouse },
  };
  REQUIRE( f() == nothing );
  REQUIRE( notifications == expected_notifications );
  REQUIRE(
      colony.buildings[e_colony_building::warehouse_expansion] ==
      false );

  // Lacking hammers.
  colony.buildings[e_colony_building::warehouse] = true;
  colony.construction = Construction::building{
    .what = e_colony_building::warehouse_expansion };
  expected_notifications = {};
  REQUIRE( f() == nothing );
  REQUIRE( notifications == expected_notifications );
  REQUIRE(
      colony.buildings[e_colony_building::warehouse_expansion] ==
      false );

  // Lacking hammers still.
  colony.hammers      = 79;
  colony.construction = Construction::building{
    .what = e_colony_building::warehouse_expansion };
  expected_notifications = {};
  REQUIRE( f() == nothing );
  REQUIRE( notifications == expected_notifications );
  REQUIRE(
      colony.buildings[e_colony_building::warehouse_expansion] ==
      false );

  // Lacking tools.
  colony.hammers                         = 80;
  colony.commodities[e_commodity::tools] = 10;
  colony.construction = Construction::building{
    .what = e_colony_building::warehouse_expansion };
  expected_notifications = { N::construction_missing_tools{
    .what =
        Construction::building{
          .what = e_colony_building::warehouse_expansion },
    .have_tools = 10,
    .need_tools = 20 } };
  REQUIRE( f() == nothing );
  REQUIRE( notifications == expected_notifications );
  REQUIRE( colony.hammers == 80 );
  REQUIRE( colony.commodities[e_commodity::tools] == 10 );
  REQUIRE(
      colony.buildings[e_colony_building::warehouse_expansion] ==
      false );

  // Built building.
  colony.hammers                         = 82;
  colony.commodities[e_commodity::tools] = 23;
  colony.construction = Construction::building{
    .what = e_colony_building::warehouse_expansion };
  expected_notifications = { N::construction_complete{
    .what = Construction::building{
      .what = e_colony_building::warehouse_expansion } } };
  expected               = ColonyBuilt::building{
                  .what = e_colony_building::warehouse_expansion };
  REQUIRE( f() == expected );
  REQUIRE( notifications == expected_notifications );
  REQUIRE( colony.hammers == 0 ); // NOTE: not 2.
  REQUIRE( colony.commodities[e_commodity::tools] == 3 );
  REQUIRE(
      colony.buildings[e_colony_building::warehouse_expansion] ==
      true );

  // Build wagon train (lacking hammers).
  colony.hammers                         = 39;
  colony.commodities[e_commodity::tools] = 0;
  colony.construction =
      Construction::unit{ .type = e_unit_type::wagon_train };
  expected_notifications = {};
  REQUIRE( f() == nothing );
  REQUIRE( notifications == expected_notifications );
  REQUIRE( w.units().all().size() == 1 ); // nothing built.
  REQUIRE( colony.hammers == 39 );
  REQUIRE( colony.commodities[e_commodity::tools] == 0 );

  // Build wagon train.
  colony.hammers                         = 40;
  colony.commodities[e_commodity::tools] = 0;
  colony.construction =
      Construction::unit{ .type = e_unit_type::wagon_train };
  expected_notifications = { N::construction_complete{
    .what = Construction::unit{
      .type = e_unit_type::wagon_train } } };
  expected = ColonyBuilt::unit{ .unit_id = UnitId{ 2 } };
  REQUIRE( f() == expected );
  REQUIRE( notifications == expected_notifications );
  REQUIRE( w.units().all().size() == 2 ); // one built.
  REQUIRE( colony.hammers == 0 );
  REQUIRE( colony.commodities[e_commodity::tools] == 0 );

  // Build wagon train (limit exceeded).
  colony.hammers                         = 40;
  colony.commodities[e_commodity::tools] = 0;
  colony.construction =
      Construction::unit{ .type = e_unit_type::wagon_train };
  expected_notifications = { N::wagon_train_limit_reached{} };
  REQUIRE( f() == nothing );
  REQUIRE( notifications == expected_notifications );
  REQUIRE( w.units().all().size() == 2 ); // nothing built.
  REQUIRE( colony.hammers == 40 );
  REQUIRE( colony.commodities[e_commodity::tools] == 0 );

  // Build caraval.
  colony.hammers                                = 128;
  colony.commodities[e_commodity::tools]        = 40;
  colony.buildings[e_colony_building::shipyard] = true;
  colony.construction =
      Construction::unit{ .type = e_unit_type::caravel };
  expected_notifications = { N::construction_complete{
    .what =
        Construction::unit{ .type = e_unit_type::caravel } } };
  expected = ColonyBuilt::unit{ .unit_id = UnitId{ 3 } };
  REQUIRE( f() == expected );
  REQUIRE( notifications == expected_notifications );
  REQUIRE( w.units().all().size() == 3 ); // one built.
  REQUIRE( colony.hammers == 0 );
  REQUIRE( colony.commodities[e_commodity::tools] == 0 );
}

} // namespace
} // namespace rn
