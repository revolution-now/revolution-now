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
#include "test/mocking.hpp"
#include "test/testing.hpp"

// Under test.
#include "src/lcr.hpp"

// Testing
#include "test/fake/world.hpp"
#include "test/mocks/igui.hpp"
#include "test/mocks/irand.hpp"
#include "test/mocks/land-view-plane.hpp"

// Revolution Now
#include "imap-updater.hpp"
#include "plane-stack.hpp"
#include "unit-mgr.hpp"

// ss
#include "ss/players.hpp"
#include "ss/settings.hpp"
#include "ss/terrain.hpp"
#include "ss/units.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/cc-specific.hpp"
#include "base/keyval.hpp"
#include "base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace ::std;

using ::mock::matchers::_;
using ::mock::matchers::Field;
using ::mock::matchers::StrContains;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() {
    create_default_map();
    add_default_player();
  }

  void create_default_map() {
    MapSquare const   L = make_grassland();
    vector<MapSquare> tiles{ L };
    build_map( std::move( tiles ), 1 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[lcr] has_lost_city_rumor" ) {
  TerrainState terrain_state;
  terrain_state.modify_entire_map( []( Matrix<MapSquare>& m ) {
    m = Matrix<MapSquare>( Delta{ .w = 1, .h = 1 } );
  } );

  REQUIRE_FALSE( has_lost_city_rumor( terrain_state, Coord{} ) );
  MapSquare& square = terrain_state.mutable_square_at( Coord{} );
  square.surface    = e_surface::land;
  square.lost_city_rumor = true;
  REQUIRE( has_lost_city_rumor( terrain_state, Coord{} ) );
}

TEST_CASE( "[lcr] de soto means no unit lost" ) {
  World   W;
  Player& player = W.default_player();
  player.fathers.has[e_founding_father::hernando_de_soto] = true;
  // The rumor type weights are integral and are required to sum
  // to 100, but the logic zeroes out the unit-lost weight, which
  // is currently three, so the total weight will be 97.
  int const kUpperLimit = 97;
  for( int i = 0; i < kUpperLimit; i += 1 ) {
    INFO( fmt::format( "i: {}", i ) );

    W.rand()
        .EXPECT__between_ints( 0, kUpperLimit,
                               e_interval::half_open )
        .returns( i );
    e_rumor_type type = pick_rumor_type_result(
        W.rand(), e_lcr_explorer_category::other, player );
    REQUIRE( type != e_rumor_type::unit_lost );
  }
}

TEST_CASE( "[lcr] nothing but rumors" ) {
  World W;

  // Set players.
  Player& player = W.default_player();
  REQUIRE( player.money == 0 );

  MapSquare& square      = W.square( Coord{} );
  square.lost_city_rumor = true;

  // Create unit on map.
  UnitId unit_id =
      W.add_unit_on_map( e_unit_type::free_colonist, Coord{} )
          .id();
  REQUIRE( W.units().all().size() == 1 );

  // Set outcome types.
  e_rumor_type         rumor_type = e_rumor_type::none;
  e_burial_mounds_type burial_type =
      e_burial_mounds_type::cold_and_empty; // not relevant.
  bool has_burial_grounds = false;

  // Mock function calls.

  W.gui()
      .EXPECT__message_box( StrContains( "nothing but rumors" ) )
      .returns( make_wait() );

  // Go
  wait<LostCityRumorResult> lcr_res = run_lost_city_rumor_result(
      W.ss(), W.ts(), player, unit_id,
      /*move_dst=*/Coord{}, rumor_type, burial_type,
      has_burial_grounds );

  // Make sure that we finished at all.
  REQUIRE( lcr_res.ready() );

  // Make sure that we have the correct result and side effects.
  REQUIRE( lcr_res->holds<LostCityRumorResult::other>() );
  REQUIRE( player.money == 0 );
  REQUIRE( W.units().exists( unit_id ) );
  REQUIRE( W.units().all().size() == 1 );
}

TEST_CASE( "[lcr] small village, chief gift" ) {
  World W;

  // Set players.
  Player& player = W.default_player();
  REQUIRE( player.money == 0 );

  MapSquare& square      = W.square( Coord{} );
  square.lost_city_rumor = true;

  // Create unit on map.
  UnitId unit_id =
      W.add_unit_on_map( e_unit_type::free_colonist, Coord{} )
          .id();
  REQUIRE( W.units().all().size() == 1 );

  // Set outcome types.
  e_rumor_type         rumor_type = e_rumor_type::chief_gift;
  e_burial_mounds_type burial_type =
      e_burial_mounds_type::cold_and_empty; // not relevant.
  bool has_burial_grounds = false;

  // Mock function calls.
  W.gui()
      .EXPECT__message_box(
          StrContains( "You happen upon a small village" ) )
      .returns( make_wait() );
  // Get quantity of chief gift.
  W.rand()
      .EXPECT__between_ints( 15, 70, e_interval::closed )
      .returns( 32 );

  // Go
  wait<LostCityRumorResult> lcr_res = run_lost_city_rumor_result(
      W.ss(), W.ts(), player, unit_id,
      /*move_dst=*/Coord{}, rumor_type, burial_type,
      has_burial_grounds );

  // Make sure that we finished at all.
  REQUIRE( lcr_res.ready() );

  // Make sure that we have the correct result and side effects.
  REQUIRE( lcr_res->holds<LostCityRumorResult::other>() );
  REQUIRE( player.money == 32 );
  REQUIRE( W.units().exists( unit_id ) );
  REQUIRE( W.units().all().size() == 1 );
}

TEST_CASE( "[lcr] small village, ruins of lost colony" ) {
  World   W;
  Player& player = W.default_player();
  REQUIRE( player.money == 0 );

  MapSquare& square      = W.square( Coord{} );
  square.lost_city_rumor = true;

  // Create unit on map.
  UnitId unit_id =
      W.add_unit_on_map( e_unit_type::free_colonist, Coord{} )
          .id();
  REQUIRE( W.units().all().size() == 1 );

  // Set outcome types.
  e_rumor_type         rumor_type = e_rumor_type::ruins;
  e_burial_mounds_type burial_type =
      e_burial_mounds_type::cold_and_empty; // not relevant.
  bool has_burial_grounds = false;

  // Mock function calls.

  W.gui()
      .EXPECT__message_box(
          StrContains( "ruins of a lost colony" ) )
      .returns( make_wait() );
  // Get quantity of gift.
  W.rand()
      .EXPECT__between_ints( 80, 220, e_interval::closed )
      .returns( 95 );

  // Go
  wait<LostCityRumorResult> lcr_res = run_lost_city_rumor_result(
      W.ss(), W.ts(), player, unit_id,
      /*move_dst=*/Coord{}, rumor_type, burial_type,
      has_burial_grounds );

  // Make sure that we finished at all.
  REQUIRE( lcr_res.ready() );

  // Make sure that we have the correct result and side effects.
  REQUIRE( lcr_res->holds<LostCityRumorResult::other>() );
  REQUIRE( player.money == 90 ); // rounded down to nearest 10.
  REQUIRE( W.units().exists( unit_id ) );
  REQUIRE( W.units().all().size() == 1 );
}

TEST_CASE( "[lcr] fountain of youth" ) {
  World   W;
  Player& player = W.default_player();
  REQUIRE( player.money == 0 );

  MapSquare& square      = W.square( Coord{} );
  square.lost_city_rumor = true;

  // Create unit on map.
  UnitId unit_id =
      W.add_unit_on_map( e_unit_type::free_colonist, Coord{} )
          .id();
  REQUIRE( W.units().all().size() == 1 );

  // Set outcome types.
  e_rumor_type rumor_type = e_rumor_type::fountain_of_youth;
  e_burial_mounds_type burial_type =
      e_burial_mounds_type::cold_and_empty; // not relevant.
  bool has_burial_grounds = false;

  // Mock function calls.
  W.gui()
      .EXPECT__message_box( StrContains(
          "You've discovered a Fountain of Youth!" ) )
      .returns( make_wait() );
  // Need to do this in a loop because we need a separate return
  // object for each one (since they are moved).
  for( int i = 0; i < 8; ++i ) {
    W.gui()
        .EXPECT__choice(
            Field( &ChoiceConfig::msg,
                   StrContains( "Who shall we next choose" ) ),
            e_input_required::no )
        .returns( make_wait<maybe<string>>( "1" ) );
    W.gui()
        .EXPECT__wait_for( chrono::milliseconds( 300 ) )
        .returns( chrono::microseconds{} );
    // This one is to choose that unit's replacement in the pool,
    // which is always done randomly. 9960.0 was found by summing
    // all of the unit type weights for all units on the discov-
    // erer level.
    W.rand()
        .EXPECT__between_doubles(
            0, mock::matchers::Approx( 9960.0, .00001 ) )
        .returns( 5000.0 ); // chosen to give scout.
  }

  // Go
  wait<LostCityRumorResult> lcr_res = run_lost_city_rumor_result(
      W.ss(), W.ts(), player, unit_id,
      /*move_dst=*/Coord{}, rumor_type, burial_type,
      has_burial_grounds );

  // Make sure that we finished at all.
  REQUIRE( lcr_res.ready() );

  // Make sure that we have the correct result and side effects.
  REQUIRE( lcr_res->holds<LostCityRumorResult::other>() );
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

TEST_CASE( "[lcr] free colonist" ) {
  World   W;
  Player& player = W.default_player();
  REQUIRE( player.money == 0 );

  MapSquare& square      = W.square( Coord{} );
  square.lost_city_rumor = true;

  // Create unit on map.
  UnitId unit_id =
      W.add_unit_on_map( e_unit_type::free_colonist, Coord{} )
          .id();
  REQUIRE( W.units().all().size() == 1 );

  // Set outcome types.
  e_rumor_type         rumor_type = e_rumor_type::free_colonist;
  e_burial_mounds_type burial_type =
      e_burial_mounds_type::cold_and_empty; // not relevant.
  bool has_burial_grounds = false;

  // Mock function calls.

  W.gui()
      .EXPECT__message_box(
          StrContains( "happen upon the survivors" ) )
      .returns( make_wait() );

  // Go
  wait<LostCityRumorResult> lcr_res = run_lost_city_rumor_result(
      W.ss(), W.ts(), player, unit_id,
      /*move_dst=*/Coord{}, rumor_type, burial_type,
      has_burial_grounds );

  // Make sure that we finished at all.
  REQUIRE( lcr_res.ready() );

  // Make sure that we have the correct result and side effects.
  REQUIRE( lcr_res->holds<LostCityRumorResult::unit_created>() );
  REQUIRE(
      lcr_res->get<LostCityRumorResult::unit_created>().id ==
      UnitId{ 2 } );
  REQUIRE( player.money == 0 );
  REQUIRE( W.units().exists( unit_id ) );
  REQUIRE( W.units().all().size() == 2 );
}

TEST_CASE( "[lcr] unit lost" ) {
  World   W;
  Player& player = W.default_player();
  REQUIRE( player.money == 0 );

  MapSquare&              square = W.square( Coord{} );
  maybe<FogSquare> const& player_square =
      W.player_square( Coord{} );
  square.lost_city_rumor = true;
  REQUIRE( !player_square.has_value() );

  // Create unit on map.
  UnitId unit_id =
      W.add_unit_on_map( e_unit_type::free_colonist, Coord{} )
          .id();
  REQUIRE( W.units().all().size() == 1 );
  REQUIRE( player_square.has_value() );
  REQUIRE( player_square->square.lost_city_rumor == true );

  // Set outcome types.
  e_rumor_type         rumor_type = e_rumor_type::unit_lost;
  e_burial_mounds_type burial_type =
      e_burial_mounds_type::cold_and_empty; // not relevant.
  bool has_burial_grounds = false;

  // Mock function calls.

  W.gui()
      .EXPECT__message_box(
          StrContains( "vanished without a trace" ) )
      .returns( make_wait() );

  // Go
  wait<LostCityRumorResult> lcr_res = run_lost_city_rumor_result(
      W.ss(), W.ts(), player, unit_id,
      /*move_dst=*/Coord{}, rumor_type, burial_type,
      has_burial_grounds );

  // Make sure that we finished at all.
  REQUIRE( lcr_res.ready() );

  // Make sure that we have the correct result and side effects.
  REQUIRE( lcr_res->holds<LostCityRumorResult::unit_lost>() );
  REQUIRE( player.money == 0 );
  REQUIRE_FALSE( W.units().exists( unit_id ) );
  REQUIRE( W.units().all().size() == 0 );
  // Make sure that, even though the unit was lost, that the
  // player map was updated to remove the LCR. Otherwise the tile
  // will still appear to be there to the player until they move
  // another unit near it, at which point it will disappear mys-
  // teriously.
  REQUIRE( player_square->square.lost_city_rumor == false );
}

TEST_CASE( "[lcr] cibola / treasure" ) {
  // FIXME
#ifdef COMPILER_GCC
  return;
#endif
  World             W;
  MockLandViewPlane land_view_plane;
  W.planes().back().land_view = &land_view_plane;
  Player& player              = W.default_player();
  REQUIRE( player.money == 0 );

  MapSquare& square      = W.square( Coord{} );
  square.lost_city_rumor = true;

  // Create unit on map.
  UnitId unit_id =
      W.add_unit_on_map( e_unit_type::free_colonist, Coord{} )
          .id();
  REQUIRE( W.units().all().size() == 1 );

  // Set outcome types.
  e_rumor_type rumor_type = e_rumor_type::cibola;

  // Mock function calls.

  W.gui()
      .EXPECT__message_box(
          StrContains( "Seven Cities of Cibola" ) )
      .returns( make_wait() );
  // Get quantity of treasure.
  W.rand()
      .EXPECT__between_ints( 2000, 10500, e_interval::closed )
      .returns( 5555 );
  // Enpixelate the treasure.
  land_view_plane.EXPECT__animate( _ ).returns<monostate>();

  // Go
  wait<LostCityRumorResult> lcr_res = run_lost_city_rumor_result(
      W.ss(), W.ts(), player, unit_id,
      /*move_dst=*/Coord{}, rumor_type, /*burial_type=*/{},
      /*has_burial_grounds=*/false );

  // Make sure that we finished at all.
  REQUIRE( lcr_res.ready() );

  // Make sure that we have the correct result and side effects.
  REQUIRE( lcr_res->holds<LostCityRumorResult::unit_created>() );
  REQUIRE(
      lcr_res->get<LostCityRumorResult::unit_created>().id ==
      UnitId{ 2 } );
  Unit const& unit = W.units().unit_for( UnitId{ 2 } );
  REQUIRE( unit.type() == e_unit_type::treasure );
  refl::enum_map<e_unit_inventory, int> const& inventory =
      unit.composition().inventory();
  int const gold = inventory[e_unit_inventory::gold];
  // These number come from the config files for the min/max
  // amount of a treasure train for a non-scout on the lowest
  // difficulty mode.
  REQUIRE( gold == 5500 ); // rounded down to nearest 100.
  // Money is zero because the gold is on the treasure train.
  REQUIRE( player.money == 0 );
  REQUIRE( W.units().exists( unit_id ) );
  REQUIRE( W.units().all().size() == 2 );
}

TEST_CASE( "[lcr] burial mounds / treasure" ) {
  // FIXME
#ifdef COMPILER_GCC
  return;
#endif
  World             W;
  MockLandViewPlane land_view_plane;
  W.planes().back().land_view = &land_view_plane;
  Player& player              = W.default_player();
  REQUIRE( player.money == 0 );

  MapSquare& square      = W.square( Coord{} );
  square.lost_city_rumor = true;

  // Create unit on map.
  UnitId unit_id =
      W.add_unit_on_map( e_unit_type::free_colonist, Coord{} )
          .id();
  REQUIRE( W.units().all().size() == 1 );

  // Set outcome types.
  e_rumor_type         rumor_type = e_rumor_type::burial_mounds;
  e_burial_mounds_type burial_type =
      e_burial_mounds_type::treasure_train;
  bool has_burial_grounds = false;

  // Mock function calls.
  W.gui()
      .EXPECT__choice( _, e_input_required::yes )
      .returns( make_wait<maybe<string>>( "yes" ) );
  W.gui()
      .EXPECT__message_box(
          StrContains( "recovered a treasure worth" ) )
      .returns( make_wait() );
  // Get quantity of treasure.
  W.rand()
      .EXPECT__between_ints( 2000, 3500, e_interval::closed )
      .returns( 2222 );
  // Enpixelate the treasure.
  land_view_plane.EXPECT__animate( _ ).returns<monostate>();

  // Go
  wait<LostCityRumorResult> lcr_res = run_lost_city_rumor_result(
      W.ss(), W.ts(), player, unit_id,
      /*move_dst=*/Coord{}, rumor_type, burial_type,
      has_burial_grounds );

  // Make sure that we finished at all.
  REQUIRE( lcr_res.ready() );

  // Make sure that we have the correct result and side effects.
  REQUIRE( lcr_res->holds<LostCityRumorResult::unit_created>() );
  REQUIRE(
      lcr_res->get<LostCityRumorResult::unit_created>().id ==
      UnitId{ 2 } );
  Unit const& unit = W.units().unit_for( UnitId{ 2 } );
  REQUIRE( unit.type() == e_unit_type::treasure );
  refl::enum_map<e_unit_inventory, int> const& inventory =
      unit.composition().inventory();
  int const gold = inventory[e_unit_inventory::gold];
  // These number come from the config files for the min/max
  // amount of a treasure train for a non-scout on the lowest
  // difficulty mode.
  REQUIRE( gold == 2200 ); // rounded down to nearest 100.
  // Money is zero because the gold is on the treasure train.
  REQUIRE( player.money == 0 );
  REQUIRE( W.units().exists( unit_id ) );
  REQUIRE( W.units().all().size() == 2 );
}

TEST_CASE( "[lcr] burial mounds / cold and empty" ) {
  // FIXME
#ifdef COMPILER_GCC
  return;
#endif
  World   W;
  Player& player = W.default_player();
  REQUIRE( player.money == 0 );

  MapSquare& square      = W.square( Coord{} );
  square.lost_city_rumor = true;

  // Create unit on map.
  UnitId unit_id =
      W.add_unit_on_map( e_unit_type::free_colonist, Coord{} )
          .id();
  REQUIRE( W.units().all().size() == 1 );

  // Set outcome types.
  e_rumor_type         rumor_type = e_rumor_type::burial_mounds;
  e_burial_mounds_type burial_type =
      e_burial_mounds_type::cold_and_empty;
  bool has_burial_grounds = false;

  // Mock function calls.
  W.gui()
      .EXPECT__choice( _, e_input_required::yes )
      .returns( make_wait<maybe<string>>( "yes" ) );
  W.gui()
      .EXPECT__message_box( StrContains( "cold and empty" ) )
      .returns( make_wait() );

  // Go
  wait<LostCityRumorResult> lcr_res = run_lost_city_rumor_result(
      W.ss(), W.ts(), player, unit_id,
      /*move_dst=*/Coord{}, rumor_type, burial_type,
      has_burial_grounds );

  // Make sure that we finished at all.
  REQUIRE( lcr_res.ready() );

  // Make sure that we have the correct result and side effects.
  REQUIRE( lcr_res->holds<LostCityRumorResult::other>() );
  REQUIRE( player.money == 0 );
  REQUIRE( W.units().exists( unit_id ) );
  REQUIRE( W.units().all().size() == 1 );
}

TEST_CASE( "[lcr] burial mounds / trinkets" ) {
  // FIXME
#ifdef COMPILER_GCC
  return;
#endif
  World   W;
  Player& player = W.default_player();
  REQUIRE( player.money == 0 );

  MapSquare& square      = W.square( Coord{} );
  square.lost_city_rumor = true;

  // Create unit on map.
  UnitId unit_id =
      W.add_unit_on_map( e_unit_type::free_colonist, Coord{} )
          .id();
  REQUIRE( W.units().all().size() == 1 );

  // Set outcome types.
  e_rumor_type         rumor_type = e_rumor_type::burial_mounds;
  e_burial_mounds_type burial_type =
      e_burial_mounds_type::trinkets;
  bool has_burial_grounds = false;

  // Mock function calls.
  W.gui()
      .EXPECT__choice( _, e_input_required::yes )
      .returns( make_wait<maybe<string>>( "yes" ) );

  W.gui()
      .EXPECT__message_box(
          StrContains( "found some trinkets" ) )
      .returns( make_wait() );
  // Get quantity of the gift.
  W.rand()
      .EXPECT__between_ints( 70, 200, e_interval::closed )
      .returns( 155 );

  // Go
  wait<LostCityRumorResult> lcr_res = run_lost_city_rumor_result(
      W.ss(), W.ts(), player, unit_id,
      /*move_dst=*/Coord{}, rumor_type, burial_type,
      has_burial_grounds );

  // Make sure that we finished at all.
  REQUIRE( lcr_res.ready() );

  // Make sure that we have the correct result and side effects.
  REQUIRE( lcr_res->holds<LostCityRumorResult::other>() );
  // These number come from the config files for the min/max
  // amount of a trinkets gift to a non-scout on the lowest dif-
  // ficulty mode.
  REQUIRE( player.money == 150 ); // rounded down to nearest 10.
  REQUIRE( W.units().exists( unit_id ) );
  REQUIRE( W.units().all().size() == 1 );
}

TEST_CASE( "[lcr] burial mounds / no explore" ) {
  // FIXME
#ifdef COMPILER_GCC
  return;
#endif
  World   W;
  Player& player = W.default_player();
  REQUIRE( player.money == 0 );

  MapSquare& square      = W.square( Coord{} );
  square.lost_city_rumor = true;

  // Create unit on map.
  UnitId unit_id =
      W.add_unit_on_map( e_unit_type::free_colonist, Coord{} )
          .id();
  REQUIRE( W.units().all().size() == 1 );

  // Set outcome types.
  e_rumor_type         rumor_type = e_rumor_type::burial_mounds;
  e_burial_mounds_type burial_type =
      e_burial_mounds_type::trinkets;
  bool has_burial_grounds = false;

  // Mock function calls.
  W.gui()
      .EXPECT__choice( _, e_input_required::yes )
      .returns( make_wait<maybe<string>>( "no" ) );

  // Go
  wait<LostCityRumorResult> lcr_res = run_lost_city_rumor_result(
      W.ss(), W.ts(), player, unit_id,
      /*move_dst=*/Coord{}, rumor_type, burial_type,
      has_burial_grounds );

  // Make sure that we finished at all.
  REQUIRE( lcr_res.ready() );

  // Make sure that we have the correct result and side effects.
  REQUIRE( lcr_res->holds<LostCityRumorResult::other>() );
  REQUIRE( player.money == 0 );
  REQUIRE( W.units().exists( unit_id ) );
  REQUIRE( W.units().all().size() == 1 );
}

TEST_CASE(
    "[lcr] burial mounds / trinkets with burial grounds" ) {
  // FIXME
#ifdef COMPILER_GCC
  return;
#endif
  World   W;
  Player& player = W.default_player();
  REQUIRE( player.money == 0 );

  MapSquare& square      = W.square( Coord{} );
  square.lost_city_rumor = true;

  // Create unit on map.
  UnitId unit_id =
      W.add_unit_on_map( e_unit_type::free_colonist, Coord{} )
          .id();
  REQUIRE( W.units().all().size() == 1 );

  // Set outcome types.
  e_rumor_type         rumor_type = e_rumor_type::burial_mounds;
  e_burial_mounds_type burial_type =
      e_burial_mounds_type::trinkets;
  bool has_burial_grounds = true;

  // Mock function calls.
  W.gui()
      .EXPECT__choice( _, e_input_required::yes )
      .returns( make_wait<maybe<string>>( "yes" ) );

  W.gui()
      .EXPECT__message_box(
          StrContains( "found some trinkets" ) )
      .returns( make_wait() );

  W.gui()
      .EXPECT__message_box(
          StrContains( "native burial grounds" ) )
      .returns( make_wait() );
  // Get quantity of the gift.
  W.rand()
      .EXPECT__between_ints( 70, 200, e_interval::closed )
      .returns( 155 );

  // Go
  wait<LostCityRumorResult> lcr_res = run_lost_city_rumor_result(
      W.ss(), W.ts(), player, unit_id,
      /*move_dst=*/Coord{}, rumor_type, burial_type,
      has_burial_grounds );

  // Make sure that we finished at all.
  REQUIRE( lcr_res.ready() );

  // Make sure that we have the correct result and side effects.
  REQUIRE( lcr_res->holds<LostCityRumorResult::other>() );
  // These number come from the config files for the min/max
  // amount of a trinkets gift to a non-scout on the lowest dif-
  // ficulty mode.
  REQUIRE( player.money == 150 ); // rounded down to nearest 10.
  REQUIRE( W.units().exists( unit_id ) );
  REQUIRE( W.units().all().size() == 1 );
}

} // namespace
} // namespace rn
