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

  inline static constexpr Coord kBordersWater =
      Coord{ .x = 1, .y = 1 };
  inline static constexpr Coord kNoBordersWater =
      Coord{ .x = 4, .y = 1 };
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[construction] select building, borders water" ) {
  World   W;
  Colony& colony = W.add_colony( World::kBordersWater );
  wait<>  w      = make_wait<>();

  auto f = [&] {
    return select_colony_construction( W.ss(), colony, W.gui() );
  };

  REQUIRE( !colony.construction.has_value() );

  SECTION( "population=0, fathers=0" ) {
    ChoiceConfig const config{
        .msg               = "Select One",
        .options           = { ChoiceConfigOption{
                      .key = "none", .display_name = "(no production)" } },
        .sort              = false,
        .initial_selection = nothing };

    EXPECT_CALL( W.gui(),
                 choice( config, e_input_required::no ) )
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

    EXPECT_CALL( W.gui(),
                 choice( config, e_input_required::no ) )
        .returns(
            make_wait<maybe<string>>( "tobacconists_shop" ) );
    w = f();
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    REQUIRE(
        colony.construction ==
        Construction_t{ Construction::building{
            .what = e_colony_building::tobacconists_shop } } );
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

    EXPECT_CALL( W.gui(),
                 choice( config, e_input_required::no ) )
        .returns(
            make_wait<maybe<string>>( "tobacconists_shop" ) );
    w = f();
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    REQUIRE(
        colony.construction ==
        Construction_t{ Construction::building{
            .what = e_colony_building::tobacconists_shop } } );
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

    EXPECT_CALL( W.gui(),
                 choice( config, e_input_required::no ) )
        .returns(
            make_wait<maybe<string>>( "fur_trading_post" ) );
    w = f();
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    REQUIRE(
        colony.construction ==
        Construction_t{ Construction::building{
            .what = e_colony_building::fur_trading_post } } );
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

    EXPECT_CALL( W.gui(),
                 choice( config, e_input_required::no ) )
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
    return select_colony_construction( W.ss(), colony, W.gui() );
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

    EXPECT_CALL( W.gui(),
                 choice( config, e_input_required::no ) )
        .returns(
            make_wait<maybe<string>>( "tobacconists_shop" ) );
    w = f();
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    REQUIRE(
        colony.construction ==
        Construction_t{ Construction::building{
            .what = e_colony_building::tobacconists_shop } } );
  }
}

} // namespace
} // namespace rn
