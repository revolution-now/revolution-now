/****************************************************************
**lcr.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-05-23.
*
* Description: Unit tests for the src/lcr.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/lcr.hpp"

// Testing
#include "test/fake/world.hpp"
#include "test/mocking.hpp"
#include "test/mocks/iagent.hpp"
#include "test/mocks/irand.hpp"
#include "test/mocks/land-view-plane.hpp"

// Revolution Now
#include "src/imap-search.rds.hpp"
#include "src/imap-updater.hpp"
#include "src/plane-stack.hpp"
#include "src/unit-mgr.hpp"

// ss
#include "src/ss/players.hpp"
#include "src/ss/ref.hpp"
#include "src/ss/settings.hpp"
#include "src/ss/terrain.hpp"
#include "src/ss/tribe.rds.hpp"
#include "src/ss/units.hpp"

// refl
#include "src/refl/to-str.hpp"

// base
#include "src/base/cc-specific.hpp"
#include "src/base/keyval.hpp"
#include "src/base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp"

RDS_DEFINE_MOCK( IMapSearch );

namespace rn {
namespace {

using namespace ::std;
using namespace ::rn::signal;

using ::gfx::point;
using ::mock::matchers::_;
using ::mock::matchers::AllOf;
using ::mock::matchers::Field;
using ::mock::matchers::StrContains;
using ::mock::matchers::Type;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct world : testing::World {
  using Base = testing::World;
  world() : Base() {
    create_default_map();
    add_default_player();

    planes().get().set_bottom<ILandViewPlane>( mock_land_view );
  }

  void create_default_map() {
    MapSquare const L = make_grassland();
    vector<MapSquare> tiles{ L };
    build_map( std::move( tiles ), 1 );
  }

  MockLandViewPlane mock_land_view;
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[lcr] de soto means no negative results" ) {
  world W;
  Player& player = W.default_player();
  MockIMapSearch mock_map_search;
  e_unit_type unit_type = e_unit_type::scout;

  auto f = [&] {
    return compute_lcr( W.ss(), player, W.rand(),
                        mock_map_search, unit_type,
                        { .x = 0, .y = 0 } );
  };

  player.fathers.has[e_founding_father::hernando_de_soto] = true;

  // The rumor type weights are integral and are required to sum
  // to 100, but the logic zeroes out the unit_lost weight and
  // the holy_shrines weight, which currently total to four, so
  // the total weight will be 96.
  int const kUpperLimit = 96;
  W.rand()
      .EXPECT__uniform_int( 0, kUpperLimit - 1 )
      .returns( 9 );
  REQUIRE( f() == LostCityRumor::none{} );
}

TEST_CASE( "[lcr] REF player always yields none" ) {
  world W;
  W.add_player( e_player::ref_french );
  Player& player = W.ref_french();
  MockIMapSearch mock_map_search;
  e_unit_type unit_type = e_unit_type::scout;

  auto f = [&] {
    return compute_lcr( W.ss(), player, W.rand(),
                        mock_map_search, unit_type,
                        { .x = 0, .y = 0 } );
  };

  REQUIRE( f() == LostCityRumor::none{} );
}

TEST_CASE( "[lcr] run_lcr, none" ) {
  world W;

  // Set players.
  Player& player = W.default_player();
  REQUIRE( player.money == 0 );
  MockIAgent& agent = W.agent();

  // Create unit on map.
  UnitId unit_id =
      W.add_unit_on_map( e_unit_type::free_colonist, Coord{} )
          .id();
  REQUIRE( W.units().all().size() == 1 );

  MapSquare& square = W.square( Coord{} );
  // NOTE: do this after placing the unit so that it doesn't
  // cause the LCR to disappear.
  square.lost_city_rumor = true;

  // Set outcome types.
  LostCityRumor const rumor = LostCityRumor::none{};

  // Mock function calls.

  agent.EXPECT__message_box(
      StrContains( "nothing but rumors" ) );

  // Go
  wait<LostCityRumorUnitChange> lcr_res = run_lcr(
      W.ss(), W.mock_land_view, W.map_updater(), W.rand(),
      player, W.agent(), W.units().unit_for( unit_id ),
      /*move_dst=*/Coord{}, rumor );

  // Make sure that we finished at all.
  REQUIRE( lcr_res.ready() );

  // Make sure that we have the correct result and side effects.
  REQUIRE( lcr_res->holds<LostCityRumorUnitChange::other>() );
  REQUIRE( player.money == 0 );
  REQUIRE( W.units().exists( unit_id ) );
  REQUIRE( W.units().all().size() == 1 );
}

TEST_CASE( "[lcr] run_lcr, chief gift" ) {
  world W;

  // Set players.
  Player& player = W.default_player();
  REQUIRE( player.money == 0 );
  MockIAgent& agent = W.agent();

  // Create unit on map.
  UnitId unit_id =
      W.add_unit_on_map( e_unit_type::free_colonist, Coord{} )
          .id();
  REQUIRE( W.units().all().size() == 1 );

  MapSquare& square = W.square( Coord{} );
  // NOTE: do this after placing the unit so that it doesn't
  // cause the LCR to disappear.
  square.lost_city_rumor = true;

  // Set outcome types.
  LostCityRumor const rumor =
      LostCityRumor::chief_gift{ .gold = 32 };

  // Mock function calls.
  agent.EXPECT__message_box(
      StrContains( "You happen upon a small village" ) );

  // Go
  wait<LostCityRumorUnitChange> lcr_res = run_lcr(
      W.ss(), W.mock_land_view, W.map_updater(), W.rand(),
      player, W.agent(), W.units().unit_for( unit_id ),
      /*move_dst=*/Coord{}, rumor );

  // Make sure that we finished at all.
  REQUIRE( lcr_res.ready() );

  // Make sure that we have the correct result and side effects.
  REQUIRE( lcr_res->holds<LostCityRumorUnitChange::other>() );
  REQUIRE( player.money == 32 );
  REQUIRE( W.units().exists( unit_id ) );
  REQUIRE( W.units().all().size() == 1 );
}

TEST_CASE( "[lcr] run_lcr, ruins" ) {
  world W;
  Player& player = W.default_player();
  REQUIRE( player.money == 0 );
  MockIAgent& agent = W.agent();

  // Create unit on map.
  UnitId unit_id =
      W.add_unit_on_map( e_unit_type::free_colonist, Coord{} )
          .id();
  REQUIRE( W.units().all().size() == 1 );

  MapSquare& square = W.square( Coord{} );
  // NOTE: do this after placing the unit so that it doesn't
  // cause the LCR to disappear.
  square.lost_city_rumor = true;

  // Set outcome types.
  LostCityRumor const rumor = LostCityRumor::ruins{ .gold = 90 };

  // Mock function calls.

  agent.EXPECT__message_box(
      StrContains( "ruins of a lost colony" ) );

  // Go
  wait<LostCityRumorUnitChange> lcr_res = run_lcr(
      W.ss(), W.mock_land_view, W.map_updater(), W.rand(),
      player, W.agent(), W.units().unit_for( unit_id ),
      /*move_dst=*/Coord{}, rumor );

  // Make sure that we finished at all.
  REQUIRE( lcr_res.ready() );

  // Make sure that we have the correct result and side effects.
  REQUIRE( lcr_res->holds<LostCityRumorUnitChange::other>() );
  REQUIRE( player.money == 90 );
  REQUIRE( W.units().exists( unit_id ) );
  REQUIRE( W.units().all().size() == 1 );
}

TEST_CASE( "[lcr] run_lcr, fountain of youth" ) {
  world W;
  Player& player    = W.default_player();
  MockIAgent& agent = W.agent();
  agent.EXPECT__human().by_default().returns( true );
  REQUIRE( player.money == 0 );

  // Create unit on map.
  UnitId unit_id =
      W.add_unit_on_map( e_unit_type::free_colonist, Coord{} )
          .id();
  REQUIRE( W.units().all().size() == 1 );

  MapSquare& square = W.square( Coord{} );
  // NOTE: do this after placing the unit so that it doesn't
  // cause the LCR to disappear.
  square.lost_city_rumor = true;

  // Set outcome types.
  LostCityRumor const rumor = LostCityRumor::fountain_of_youth{};

  // Mock function calls.
  agent.EXPECT__message_box(
      StrContains( "You've discovered a Fountain of Youth!" ) );
  // Need to do this in a loop because we need a separate return
  // object for each one (since they are moved).
  for( int i = 0; i < 8; ++i ) {
    INFO( fmt::format( "i: {}", i ) );
    agent
        .EXPECT__handle( Type<ChooseImmigrant>(
            Field( &ChooseImmigrant::msg,
                   StrContains( fmt::format(
                       "Who shall we choose as immigrant number "
                       "[{}] out of 8",
                       i + 1 ) ) ) ) )
        .returns( 1 );
    agent.EXPECT__wait_for( chrono::milliseconds( 100 ) );
    // This one is to choose that unit's replacement in the pool,
    // which is always done randomly. 9960.0 was found by summing
    // all of the unit type weights for all units on the discov-
    // erer level.
    W.rand()
        .EXPECT__uniform_double(
            0, mock::matchers::Approx( 9960.0, .00001 ) )
        .returns( 5000.0 ); // chosen to give scout.
  }

  // Go
  wait<LostCityRumorUnitChange> lcr_res = run_lcr(
      W.ss(), W.mock_land_view, W.map_updater(), W.rand(),
      player, W.agent(), W.units().unit_for( unit_id ),
      /*move_dst=*/Coord{}, rumor );

  // Make sure that we finished at all.
  REQUIRE( lcr_res.ready() );

  // Make sure that we have the correct result and side effects.
  REQUIRE( lcr_res->holds<LostCityRumorUnitChange::other>() );
  REQUIRE( player.money == 0 );
  REQUIRE( W.units().exists( unit_id ) );
  REQUIRE( W.units().all().size() == 9 );
  // The unit that entered the LCR; sanity check.
  REQUIRE( W.units().unit_for( UnitId{ 1 } ).type() ==
           e_unit_type::free_colonist );
  // The 8 new units created must all be scouts except for the
  // first, which was just the default petty_criminal in the im-
  // migrant pool in that slot.
  REQUIRE( W.units().unit_for( UnitId{ 2 } ).type() ==
           e_unit_type::petty_criminal );
  for( int i = 3; i <= 9; ++i ) { // unit idxs start at 1.
    INFO( fmt::format( "i: {}", i ) );
    REQUIRE( W.units().unit_for( UnitId{ i } ).type() ==
             e_unit_type::scout );
  }
}

TEST_CASE( "[lcr] run_lcr, free colonist" ) {
  world W;
  Player& player = W.default_player();
  REQUIRE( player.money == 0 );
  MockIAgent& agent = W.agent();

  // Create unit on map.
  UnitId unit_id =
      W.add_unit_on_map( e_unit_type::free_colonist, Coord{} )
          .id();
  REQUIRE( W.units().all().size() == 1 );

  MapSquare& square = W.square( Coord{} );
  // NOTE: do this after placing the unit so that it doesn't
  // cause the LCR to disappear.
  square.lost_city_rumor = true;

  // Set outcome types.
  LostCityRumor const rumor = LostCityRumor::free_colonist{};

  // Mock function calls.

  agent.EXPECT__message_box(
      StrContains( "happen upon the survivors" ) );

  // Go
  wait<LostCityRumorUnitChange> lcr_res = run_lcr(
      W.ss(), W.mock_land_view, W.map_updater(), W.rand(),
      player, W.agent(), W.units().unit_for( unit_id ),
      /*move_dst=*/Coord{}, rumor );

  // Make sure that we finished at all.
  REQUIRE( lcr_res.ready() );

  // Make sure that we have the correct result and side effects.
  REQUIRE(
      lcr_res->holds<LostCityRumorUnitChange::unit_created>() );
  REQUIRE(
      lcr_res->get<LostCityRumorUnitChange::unit_created>().id ==
      UnitId{ 2 } );
  REQUIRE( player.money == 0 );
  REQUIRE( W.units().exists( unit_id ) );
  REQUIRE( W.units().all().size() == 2 );
}

TEST_CASE( "[lcr] run_lcr, unit lost" ) {
  world W;
  Player& player = W.default_player();
  REQUIRE( player.money == 0 );
  MockIAgent& agent = W.agent();

  // Create unit on map.
  UnitId unit_id =
      W.add_unit_on_map( e_unit_type::free_colonist, Coord{} )
          .id();
  REQUIRE( W.units().all().size() == 1 );

  MapSquare& square = W.square( Coord{} );
  // NOTE: do this after placing the unit so that it doesn't
  // cause the LCR to disappear.
  square.lost_city_rumor = true;

  // Set outcome types.
  LostCityRumor const rumor = LostCityRumor::unit_lost{};

  // Mock function calls.

  agent.EXPECT__message_box(
      StrContains( "vanished without a trace" ) );

  // Go
  wait<LostCityRumorUnitChange> lcr_res = run_lcr(
      W.ss(), W.mock_land_view, W.map_updater(), W.rand(),
      player, W.agent(), W.units().unit_for( unit_id ),
      /*move_dst=*/Coord{}, rumor );

  // Make sure that we finished at all.
  REQUIRE( lcr_res.ready() );

  // Make sure that we have the correct result and side effects.
  REQUIRE(
      lcr_res->holds<LostCityRumorUnitChange::unit_lost>() );
  REQUIRE( player.money == 0 );
  REQUIRE_FALSE( W.units().exists( unit_id ) );
  REQUIRE( W.units().all().size() == 0 );
}

TEST_CASE( "[lcr] run_lcr, cibola" ) {
  world W;
  MockLandViewPlane land_view_plane;
  W.planes().get().set_bottom<ILandViewPlane>( land_view_plane );
  Player& player = W.default_player();
  REQUIRE( player.money == 0 );
  MockIAgent& agent = W.agent();

  // Create unit on map.
  UnitId unit_id =
      W.add_unit_on_map( e_unit_type::free_colonist, Coord{} )
          .id();
  REQUIRE( W.units().all().size() == 1 );

  MapSquare& square = W.square( Coord{} );
  // NOTE: do this after placing the unit so that it doesn't
  // cause the LCR to disappear.
  square.lost_city_rumor = true;

  // Set outcome types.
  LostCityRumor const rumor =
      LostCityRumor::cibola{ .gold = 5500 };

  // Mock function calls.

  agent.EXPECT__message_box(
      StrContains( "Seven Cities of Cibola" ) );
  // Enpixelate the treasure.
  W.mock_land_view.EXPECT__animate_if_visible( _ );

  // Go
  wait<LostCityRumorUnitChange> lcr_res = run_lcr(
      W.ss(), W.mock_land_view, W.map_updater(), W.rand(),
      player, W.agent(), W.units().unit_for( unit_id ),
      /*move_dst=*/Coord{}, rumor );

  // Make sure that we finished at all.
  REQUIRE( lcr_res.ready() );

  // Make sure that we have the correct result and side effects.
  REQUIRE(
      lcr_res->holds<LostCityRumorUnitChange::unit_created>() );
  REQUIRE(
      lcr_res->get<LostCityRumorUnitChange::unit_created>().id ==
      UnitId{ 2 } );
  Unit const& unit = W.units().unit_for( UnitId{ 2 } );
  REQUIRE( unit.type() == e_unit_type::treasure );
  refl::enum_map<e_unit_inventory, int> const& inventory =
      unit.composition().inventory();
  int const gold = inventory[e_unit_inventory::gold];
  // These number come from the config files for the min/max
  // amount of a treasure train for a non-scout on the lowest
  // difficulty mode.
  REQUIRE( gold == 5500 );
  // Money is zero because the gold is on the treasure train.
  REQUIRE( player.money == 0 );
  REQUIRE( W.units().exists( unit_id ) );
  REQUIRE( W.units().all().size() == 2 );
}

TEST_CASE( "[lcr] run_lcr, burial mounds, treasure" ) {
  world W;
  MockLandViewPlane land_view_plane;
  W.planes().get().set_bottom<ILandViewPlane>( land_view_plane );
  Player& player = W.default_player();
  REQUIRE( player.money == 0 );
  MockIAgent& agent = W.agent();

  // Create unit on map.
  UnitId unit_id =
      W.add_unit_on_map( e_unit_type::free_colonist, Coord{} )
          .id();
  REQUIRE( W.units().all().size() == 1 );

  MapSquare& square = W.square( Coord{} );
  // NOTE: do this after placing the unit so that it doesn't
  // cause the LCR to disappear.
  square.lost_city_rumor = true;

  // Set outcome types.
  LostCityRumor const rumor = LostCityRumor::burial_mounds{
    .mounds         = BurialMounds::treasure{ .gold = 2200 },
    .burial_grounds = nothing };

  // Mock function calls.
  agent.EXPECT__should_explore_ancient_burial_mounds().returns(
      ui::e_confirm::yes );
  agent.EXPECT__message_box(
      StrContains( "recovered a treasure worth" ) );
  // Enpixelate the treasure.
  W.mock_land_view.EXPECT__animate_if_visible( _ );

  // Go
  wait<LostCityRumorUnitChange> lcr_res = run_lcr(
      W.ss(), W.mock_land_view, W.map_updater(), W.rand(),
      player, W.agent(), W.units().unit_for( unit_id ),
      /*move_dst=*/Coord{}, rumor );

  // Make sure that we finished at all.
  REQUIRE( lcr_res.ready() );

  // Make sure that we have the correct result and side effects.
  REQUIRE(
      lcr_res->holds<LostCityRumorUnitChange::unit_created>() );
  REQUIRE(
      lcr_res->get<LostCityRumorUnitChange::unit_created>().id ==
      UnitId{ 2 } );
  Unit const& unit = W.units().unit_for( UnitId{ 2 } );
  REQUIRE( unit.type() == e_unit_type::treasure );
  refl::enum_map<e_unit_inventory, int> const& inventory =
      unit.composition().inventory();
  int const gold = inventory[e_unit_inventory::gold];
  // These number come from the config files for the min/max
  // amount of a treasure train for a non-scout on the lowest
  // difficulty mode.
  REQUIRE( gold == 2200 );
  // Money is zero because the gold is on the treasure train.
  REQUIRE( player.money == 0 );
  REQUIRE( W.units().exists( unit_id ) );
  REQUIRE( W.units().all().size() == 2 );
}

TEST_CASE( "[lcr] run_lcr, burial mounds, cold and empty" ) {
  world W;
  Player& player = W.default_player();
  REQUIRE( player.money == 0 );
  MockIAgent& agent = W.agent();

  // Create unit on map.
  UnitId unit_id =
      W.add_unit_on_map( e_unit_type::free_colonist, Coord{} )
          .id();
  REQUIRE( W.units().all().size() == 1 );

  MapSquare& square = W.square( Coord{} );
  // NOTE: do this after placing the unit so that it doesn't
  // cause the LCR to disappear.
  square.lost_city_rumor = true;

  // Set outcome types.
  LostCityRumor const rumor = LostCityRumor::burial_mounds{
    .mounds         = BurialMounds::cold_and_empty{},
    .burial_grounds = nothing };

  // Mock function calls.
  agent.EXPECT__should_explore_ancient_burial_mounds().returns(
      ui::e_confirm::yes );
  agent.EXPECT__message_box( StrContains( "cold and empty" ) );

  // Go
  wait<LostCityRumorUnitChange> lcr_res = run_lcr(
      W.ss(), W.mock_land_view, W.map_updater(), W.rand(),
      player, W.agent(), W.units().unit_for( unit_id ),
      /*move_dst=*/Coord{}, rumor );

  // Make sure that we finished at all.
  REQUIRE( lcr_res.ready() );

  // Make sure that we have the correct result and side effects.
  REQUIRE( lcr_res->holds<LostCityRumorUnitChange::other>() );
  REQUIRE( player.money == 0 );
  REQUIRE( W.units().exists( unit_id ) );
  REQUIRE( W.units().all().size() == 1 );
}

TEST_CASE( "[lcr] run_lcr, burial mounds, trinkets" ) {
  world W;
  Player& player = W.default_player();
  REQUIRE( player.money == 0 );
  MockIAgent& agent = W.agent();

  // Create unit on map.
  UnitId unit_id =
      W.add_unit_on_map( e_unit_type::free_colonist, Coord{} )
          .id();
  REQUIRE( W.units().all().size() == 1 );

  MapSquare& square = W.square( Coord{} );
  // NOTE: do this after placing the unit so that it doesn't
  // cause the LCR to disappear.
  square.lost_city_rumor = true;

  // Set outcome types.
  LostCityRumor const rumor = LostCityRumor::burial_mounds{
    .mounds         = BurialMounds::trinkets{ .gold = 150 },
    .burial_grounds = nothing };

  // Mock function calls.
  agent.EXPECT__should_explore_ancient_burial_mounds().returns(
      ui::e_confirm::yes );

  agent.EXPECT__message_box(
      StrContains( "found some trinkets" ) );

  // Go
  wait<LostCityRumorUnitChange> lcr_res = run_lcr(
      W.ss(), W.mock_land_view, W.map_updater(), W.rand(),
      player, W.agent(), W.units().unit_for( unit_id ),
      /*move_dst=*/Coord{}, rumor );

  // Make sure that we finished at all.
  REQUIRE( lcr_res.ready() );

  // Make sure that we have the correct result and side effects.
  REQUIRE( lcr_res->holds<LostCityRumorUnitChange::other>() );
  // These number come from the config files for the min/max
  // amount of a trinkets gift to a non-scout on the lowest dif-
  // ficulty mode.
  REQUIRE( player.money == 150 );
  REQUIRE( W.units().exists( unit_id ) );
  REQUIRE( W.units().all().size() == 1 );
}

TEST_CASE( "[lcr] run_lcr, burial mounds, no explore" ) {
  world W;
  Player& player = W.default_player();
  REQUIRE( player.money == 0 );
  MockIAgent& agent = W.agent();

  // Create unit on map.
  UnitId unit_id =
      W.add_unit_on_map( e_unit_type::free_colonist, Coord{} )
          .id();
  REQUIRE( W.units().all().size() == 1 );

  MapSquare& square = W.square( Coord{} );
  // NOTE: do this after placing the unit so that it doesn't
  // cause the LCR to disappear.
  square.lost_city_rumor = true;

  // Set outcome types.
  LostCityRumor const rumor = LostCityRumor::burial_mounds{
    .mounds         = BurialMounds::trinkets{},
    .burial_grounds = nothing };

  // Mock function calls.
  agent.EXPECT__should_explore_ancient_burial_mounds().returns(
      ui::e_confirm::no );

  // Go
  wait<LostCityRumorUnitChange> lcr_res = run_lcr(
      W.ss(), W.mock_land_view, W.map_updater(), W.rand(),
      player, W.agent(), W.units().unit_for( unit_id ),
      /*move_dst=*/Coord{}, rumor );

  // Make sure that we finished at all.
  REQUIRE( lcr_res.ready() );

  // Make sure that we have the correct result and side effects.
  REQUIRE( lcr_res->holds<LostCityRumorUnitChange::other>() );
  REQUIRE( player.money == 0 );
  REQUIRE( W.units().exists( unit_id ) );
  REQUIRE( W.units().all().size() == 1 );
}

TEST_CASE(
    "[lcr] run_lcr, burial mounds, trinkets with burial "
    "grounds" ) {
  world W;
  Player& player = W.default_player();
  REQUIRE( player.money == 0 );
  MockIAgent& agent = W.agent();

  W.add_tribe( e_tribe::aztec );

  // Create unit on map.
  UnitId unit_id =
      W.add_unit_on_map( e_unit_type::free_colonist, Coord{} )
          .id();
  REQUIRE( W.units().all().size() == 1 );

  MapSquare& square = W.square( Coord{} );
  // NOTE: do this after placing the unit so that it doesn't
  // cause the LCR to disappear.
  square.lost_city_rumor = true;

  // Set outcome types.
  LostCityRumor const rumor = LostCityRumor::burial_mounds{
    .mounds         = BurialMounds::trinkets{ .gold = 150 },
    .burial_grounds = e_tribe::aztec };

  // Mock function calls.
  agent.EXPECT__should_explore_ancient_burial_mounds().returns(
      ui::e_confirm::yes );

  agent.EXPECT__message_box(
      StrContains( "found some trinkets" ) );

  agent.EXPECT__message_box(
      AllOf( StrContains( "[Aztec]" ),
             StrContains( "prepare for WAR" ) ) );

  // Go
  wait<LostCityRumorUnitChange> lcr_res = run_lcr(
      W.ss(), W.mock_land_view, W.map_updater(), W.rand(),
      player, W.agent(), W.units().unit_for( unit_id ),
      /*move_dst=*/Coord{}, rumor );

  // Make sure that we finished at all.
  REQUIRE( lcr_res.ready() );

  // Make sure that we have the correct result and side effects.
  REQUIRE( lcr_res->holds<LostCityRumorUnitChange::other>() );
  // These number come from the config files for the min/max
  // amount of a trinkets gift to a non-scout on the lowest dif-
  // ficulty mode.
  REQUIRE( player.money == 150 );
  REQUIRE( W.units().exists( unit_id ) );
  REQUIRE( W.units().all().size() == 1 );
  REQUIRE( W.aztec()
               .relationship[W.default_player_type()]
               .tribal_alarm == 99 );
}

TEST_CASE( "[lcr] run_lcr, holy shrines" ) {
  world W;
  Player& player = W.default_player();
  REQUIRE( player.money == 0 );
  MockIAgent& agent = W.agent();

  W.add_tribe( e_tribe::aztec );
  W.aztec().relationship[W.default_player_type()].tribal_alarm =
      10;

  // Create unit on map.
  UnitId unit_id =
      W.add_unit_on_map( e_unit_type::free_colonist, Coord{} )
          .id();
  REQUIRE( W.units().all().size() == 1 );

  MapSquare& square = W.square( Coord{} );
  // NOTE: do this after placing the unit so that it doesn't
  // cause the LCR to disappear.
  square.lost_city_rumor = true;

  // Set outcome types.
  LostCityRumor const rumor = LostCityRumor::holy_shrines{
    .tribe = e_tribe::aztec, .alarm_increase = 5 };

  // Mock function calls.
  agent.EXPECT__message_box( AllOf( StrContains( "[Aztec]" ),
                                    StrContains( "angered" ) ) );

  // Go
  wait<LostCityRumorUnitChange> lcr_res = run_lcr(
      W.ss(), W.mock_land_view, W.map_updater(), W.rand(),
      player, W.agent(), W.units().unit_for( unit_id ),
      /*move_dst=*/Coord{}, rumor );

  // Make sure that we finished at all.
  REQUIRE( lcr_res.ready() );

  // Make sure that we have the correct result and side effects.
  REQUIRE( lcr_res->holds<LostCityRumorUnitChange::other>() );
  // These number come from the config files for the min/max
  // amount of a trinkets gift to a non-scout on the lowest dif-
  // ficulty mode.
  REQUIRE( player.money == 0 );
  REQUIRE( W.units().exists( unit_id ) );
  REQUIRE( W.units().all().size() == 1 );
  // FIXME: improve these changes, make them more precise.
  REQUIRE( W.aztec()
               .relationship[W.default_player_type()]
               .tribal_alarm == 15 );
}

TEST_CASE( "[lcr] compute_lcr, type=none" ) {
  world W;
  Player& player = W.default_player();
  MockIMapSearch mock_map_search;
  e_unit_type unit_type = e_unit_type::scout;
  LostCityRumor expected;

  W.settings().game_setup_options.difficulty =
      e_difficulty::conquistador;

  W.add_tribe( e_tribe::aztec );
  W.add_tribe( e_tribe::tupi );

  auto f = [&] {
    return compute_lcr( W.ss(), player, W.rand(),
                        mock_map_search, unit_type,
                        { .x = 0, .y = 0 } );
  };

  SECTION( "type=none" ) {
    expected = LostCityRumor::none{};
    W.rand().EXPECT__uniform_int( 0, 99 ).returns( 0 );
    REQUIRE( f() == expected );
  }

  SECTION( "type=fountain_of_youth" ) {
    expected = LostCityRumor::fountain_of_youth{};
    W.rand().EXPECT__uniform_int( 0, 99 ).returns( 48 );
    REQUIRE( f() == expected );
  }

  SECTION( "no fountain_of_youth allowed" ) {
    player.revolution.status = e_revolution_status::declared;
    expected                 = LostCityRumor::free_colonist{};
    W.rand().EXPECT__uniform_int( 0, 94 ).returns( 48 );
    REQUIRE( f() == expected );
  }

  SECTION( "type=free_colonist" ) {
    expected = LostCityRumor::free_colonist{};
    W.rand().EXPECT__uniform_int( 0, 99 ).returns( 53 );
    REQUIRE( f() == expected );
  }

  SECTION( "type=ruins" ) {
    expected = LostCityRumor::ruins{ .gold = 120 };
    W.rand().EXPECT__uniform_int( 0, 99 ).returns( 65 );
    // Will be rounded down to the nearest 10.
    W.rand().EXPECT__uniform_int( 120, 300 ).returns( 123 );
    REQUIRE( f() == expected );
  }

  SECTION( "type=burial_mounds, treasure, no grounds" ) {
    expected = LostCityRumor::burial_mounds{
      .mounds         = BurialMounds::treasure{ .gold = 3300 },
      .burial_grounds = nothing };
    W.rand().EXPECT__uniform_int( 0, 99 ).returns( 76 );
    W.rand().EXPECT__uniform_int( 0, 99 ).returns( 0 );
    // Will be rounded down to the nearest 100.
    W.rand().EXPECT__uniform_int( 2500, 4000 ).returns( 3333 );
    mock_map_search
        .EXPECT__find_close_encountered_tribe(
            player.type, gfx::point{ .x = 0, .y = 0 }, 15 )
        .returns( e_tribe::sioux );
    // Burial grounds?
    W.rand().EXPECT__bernoulli( .05 ).returns( false );
    REQUIRE( f() == expected );
  }

  SECTION( "type=burial_mounds, trinkets, no grounds" ) {
    expected = LostCityRumor::burial_mounds{
      .mounds         = BurialMounds::trinkets{ .gold = 110 },
      .burial_grounds = nothing };
    W.rand().EXPECT__uniform_int( 0, 99 ).returns( 76 );
    W.rand().EXPECT__uniform_int( 0, 99 ).returns( 50 );
    // Will be rounded down to the nearest 10.
    W.rand().EXPECT__uniform_int( 70, 200 ).returns( 111 );
    mock_map_search
        .EXPECT__find_close_encountered_tribe(
            player.type, gfx::point{ .x = 0, .y = 0 }, 15 )
        .returns( e_tribe::sioux );
    // Burial grounds?
    W.rand().EXPECT__bernoulli( .05 ).returns( false );
    REQUIRE( f() == expected );
  }

  SECTION( "type=burial_mounds, cold-and-empty, no grounds" ) {
    expected = LostCityRumor::burial_mounds{
      .mounds         = BurialMounds::cold_and_empty{},
      .burial_grounds = nothing };
    W.rand().EXPECT__uniform_int( 0, 99 ).returns( 76 );
    W.rand().EXPECT__uniform_int( 0, 99 ).returns( 95 );
    mock_map_search
        .EXPECT__find_close_encountered_tribe(
            player.type, gfx::point{ .x = 0, .y = 0 }, 15 )
        .returns( e_tribe::sioux );
    // Burial grounds?
    W.rand().EXPECT__bernoulli( .05 ).returns( false );
    REQUIRE( f() == expected );
  }

  SECTION( "type=burial_mounds, cold-and-empty, with grounds" ) {
    expected = LostCityRumor::burial_mounds{
      .mounds         = BurialMounds::cold_and_empty{},
      .burial_grounds = e_tribe::sioux };
    W.rand().EXPECT__uniform_int( 0, 99 ).returns( 76 );
    W.rand().EXPECT__uniform_int( 0, 99 ).returns( 95 );
    mock_map_search
        .EXPECT__find_close_encountered_tribe(
            player.type, gfx::point{ .x = 0, .y = 0 }, 15 )
        .returns( e_tribe::sioux );
    // Burial grounds?
    W.rand().EXPECT__bernoulli( .05 ).returns( true );
    REQUIRE( f() == expected );
  }

  SECTION( "type=chief gift" ) {
    expected = LostCityRumor::chief_gift{ .gold = 23 };
    W.rand().EXPECT__uniform_int( 0, 99 ).returns( 82 );
    W.rand().EXPECT__uniform_int( 15, 70 ).returns( 23 );
    REQUIRE( f() == expected );
  }

  SECTION( "type=unit_lost" ) {
    expected = LostCityRumor::unit_lost{};
    W.rand().EXPECT__uniform_int( 0, 99 ).returns( 94 );
    REQUIRE( f() == expected );
  }

  SECTION( "type=no unit_lost with de soto" ) {
    // This also tests that the holy_shrines was removed (kind
    // of) given that we need to specify the total probability
    // weight below.
    player.fathers.has[e_founding_father::hernando_de_soto] =
        true;
    expected = LostCityRumor::cibola{ .gold = 5500 };
    W.rand().EXPECT__uniform_int( 0, 95 ).returns( 94 );
    // Will be rounded down to nearest 100.
    W.rand().EXPECT__uniform_int( 2500, 12000 ).returns( 5555 );
    REQUIRE( f() == expected );
  }

  SECTION( "type=cibola" ) {
    expected = LostCityRumor::cibola{ .gold = 5500 };
    W.rand().EXPECT__uniform_int( 0, 99 ).returns( 97 );
    // Will be rounded down to nearest 100.
    W.rand().EXPECT__uniform_int( 2500, 12000 ).returns( 5555 );
    REQUIRE( f() == expected );
  }

  SECTION( "type=holy_shrines, no tribes" ) {
    expected = LostCityRumor::none{};
    W.rand().EXPECT__uniform_int( 0, 99 ).returns( 99 );
    mock_map_search
        .EXPECT__find_close_encountered_tribe(
            player.type, gfx::point{ .x = 0, .y = 0 }, 15 )
        .returns( nothing );
    REQUIRE( f() == expected );
  }

  SECTION( "type=holy_shrines" ) {
    expected = LostCityRumor::holy_shrines{
      .tribe = e_tribe::sioux, .alarm_increase = 16 };
    W.rand().EXPECT__uniform_int( 0, 99 ).returns( 99 );
    mock_map_search
        .EXPECT__find_close_encountered_tribe(
            player.type, gfx::point{ .x = 0, .y = 0 }, 15 )
        .returns( e_tribe::sioux );
    // Alarm increase.
    W.rand().EXPECT__uniform_int( 14, 18 ).returns( 16 );
    REQUIRE( f() == expected );
  }
}

TEST_CASE( "[lcr] remove_lcr" ) {
  world w;

  MapSquare& square      = w.square( Coord{} );
  square.lost_city_rumor = true;

  REQUIRE( square.lost_city_rumor );
  remove_lcr( w.map_updater(), point{} );
  REQUIRE_FALSE( square.lost_city_rumor );
}

} // namespace
} // namespace rn
