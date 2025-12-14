/****************************************************************
**harbor-units.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-02.
*
* Description: Unit tests for the src/harbor-units.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Testing.
#include "test/fake/world.hpp"

// Under test.
#include "src/harbor-units.hpp"

// Revolution Now
#include "src/imap-updater.hpp"
#include "src/unit-ownership.hpp"

// ss
#include "src/ss/old-world-state.rds.hpp"
#include "src/ss/player.rds.hpp"
#include "src/ss/ref.hpp"
#include "src/ss/terrain.hpp"
#include "src/ss/units.hpp"

// gfx
#include "gfx/iter.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {

namespace {

using namespace std;

struct world : testing::World {
  world() {
    MapSquare const O = make_ocean();
    MapSquare const S = make_sea_lane();
    MapSquare const L = make_grassland();
    // clang-format off
    vector<MapSquare> tiles{
     /*     0  1  2  3  4  5  6  7  8  9  */
     /*0*/  O, O, O, O, O, O, O, O, S, S,
     /*1*/  O, O, O, O, O, O, O, O, S, S,
     /*2*/  O, O, L, L, L, L, L, O, S, S,
     /*3*/  O, O, L, L, L, L, L, O, S, S,
     /*4*/  O, O, L, L, L, L, O, O, S, S,
     /*5*/  O, O, L, L, L, L, O, O, S, S,
     /*6*/  O, O, L, L, L, L, L, O, S, S,
     /*7*/  O, O, O, O, O, O, O, O, S, S,
     /*8*/  O, O, O, O, O, O, O, O, S, S,
     /*9*/  O, O, O, O, O, O, O, O, S, S,
    };
    // clang-format on
    build_map( std::move( tiles ), 10 );
    add_player( e_player::dutch );
    add_player( e_player::french );
  }
};

TEST_CASE( "[harbor-units] is_unit_?" ) {
  world w;
  Player& player = w.default_player();
  Coord coord{ .x = 8, .y = 5 };

  UnitId caravel =
      w.add_unit_on_map( e_unit_type::caravel, coord ).id();
  REQUIRE( !is_unit_inbound( w.units(), caravel ) );
  REQUIRE( !is_unit_outbound( w.units(), caravel ) );
  REQUIRE( !is_unit_in_port( w.units(), caravel ) );

  REQUIRE( w.old_world( player ).harbor_state.selected_unit ==
           nothing );
  unit_sail_to_harbor( w.ss(), caravel );
  REQUIRE( w.old_world( player ).harbor_state.selected_unit ==
           nothing );
  int tries = 0;
  do {
    REQUIRE( is_unit_inbound( w.units(), caravel ) );
    REQUIRE( !is_unit_outbound( w.units(), caravel ) );
    REQUIRE( !is_unit_in_port( w.units(), caravel ) );
    e_high_seas_result res =
        advance_unit_on_high_seas( w.ss(), w.dutch(), caravel );
    if( res == e_high_seas_result::arrived_in_harbor ) break;
    REQUIRE( w.old_world( player ).harbor_state.selected_unit ==
             nothing );
    ++tries;
  } while( tries < 10 );
  REQUIRE( tries < 10 );
  REQUIRE( tries >= 1 );
  REQUIRE( !is_unit_inbound( w.units(), caravel ) );
  REQUIRE( !is_unit_outbound( w.units(), caravel ) );
  REQUIRE( is_unit_in_port( w.units(), caravel ) );

  unit_sail_to_new_world( w.ss(), caravel );
  REQUIRE( w.old_world( player ).harbor_state.selected_unit ==
           caravel );
  tries = 0;
  do {
    REQUIRE( !is_unit_inbound( w.units(), caravel ) );
    REQUIRE( is_unit_outbound( w.units(), caravel ) );
    REQUIRE( !is_unit_in_port( w.units(), caravel ) );
    e_high_seas_result res =
        advance_unit_on_high_seas( w.ss(), w.dutch(), caravel );
    if( res == e_high_seas_result::arrived_in_new_world ) break;
    REQUIRE( w.old_world( player ).harbor_state.selected_unit ==
             caravel );
    ++tries;
  } while( tries < 10 );
  REQUIRE( tries < 10 );
  REQUIRE( tries >= 1 );
  REQUIRE( !is_unit_inbound( w.units(), caravel ) );
  REQUIRE( is_unit_outbound( w.units(), caravel ) );
  REQUIRE( !is_unit_in_port( w.units(), caravel ) );
  // The unit is still selected because the function that ad-
  // vances it on the high seas will not move it onto the map
  // even when it finishes.
  REQUIRE( w.old_world( player ).harbor_state.selected_unit ==
           caravel );
  update_harbor_selected_unit( w.ss(), player );
  REQUIRE( w.old_world( player ).harbor_state.selected_unit ==
           caravel );

  REQUIRE( find_new_world_arrival_square(
               w.ss(), w.map_updater().connectivity(), w.dutch(),
               w.units()
                   .harbor_view_state_of( caravel )
                   .sailed_from ) == coord );
}

TEST_CASE( "[harbor-units] harbor_units_?" ) {
  world w;
  Player& player = w.default_player();
  REQUIRE( w.old_world( player ).harbor_state.selected_unit ==
           nothing );
  UnitId caravel1 =
      w.add_unit_in_port( e_unit_type::caravel ).id();
  REQUIRE( w.old_world( player ).harbor_state.selected_unit ==
           caravel1 );
  UnitId caravel2 =
      w.add_unit_in_port( e_unit_type::caravel ).id();
  REQUIRE( w.old_world( player ).harbor_state.selected_unit ==
           caravel1 );
  UnitId merchantman1 =
      w.add_unit_in_port( e_unit_type::merchantman ).id();
  unit_sail_to_new_world( w.ss(), merchantman1 );
  UnitId merchantman2 =
      w.add_unit_on_map( e_unit_type::merchantman,
                         Coord{ .x = 8, .y = 5 } )
          .id();
  unit_sail_to_harbor( w.ss(), merchantman2 );
  UnitId free_colonist1 =
      w.add_unit_in_port( e_unit_type::free_colonist ).id();
  UnitId free_colonist2 =
      w.add_unit_in_port( e_unit_type::free_colonist ).id();
  Coord coord{ .x = 4, .y = 4 };
  w.add_unit_on_map( e_unit_type::soldier, coord );
  w.add_unit_on_map( e_unit_type::soldier, coord );

  REQUIRE( harbor_units_on_dock( w.units(), e_player::dutch ) ==
           vector<UnitId>{ free_colonist2, free_colonist1 } );
  REQUIRE( harbor_units_in_port( w.units(), e_player::dutch ) ==
           vector<UnitId>{ caravel2, caravel1 } );
  REQUIRE( harbor_units_inbound( w.units(), e_player::dutch ) ==
           vector<UnitId>{ merchantman2 } );
  REQUIRE( harbor_units_outbound( w.units(), e_player::dutch ) ==
           vector<UnitId>{ merchantman1 } );

  REQUIRE( harbor_units_on_dock( w.units(), e_player::french ) ==
           vector<UnitId>{} );
  REQUIRE( harbor_units_in_port( w.units(), e_player::french ) ==
           vector<UnitId>{} );
  REQUIRE( harbor_units_inbound( w.units(), e_player::french ) ==
           vector<UnitId>{} );
  REQUIRE(
      harbor_units_outbound( w.units(), e_player::french ) ==
      vector<UnitId>{} );
}

TEST_CASE( "[harbor-units] create_unit_in_harbor" ) {
  world w;
  Player& player = w.default_player();
  UnitId id1 =
      create_unit_in_harbor( w.ss(), w.player( e_player::dutch ),
                             e_unit_type::soldier );
  UnitId id2 =
      create_unit_in_harbor( w.ss(), w.player( e_player::dutch ),
                             e_unit_type::free_colonist );
  REQUIRE( w.old_world( player ).harbor_state.selected_unit ==
           nothing );
  unordered_map<UnitId, UnitOwnership> expected;
  expected[id1] = UnitOwnership::harbor{
    .port_status = PortStatus::in_port{},
    .sailed_from = nothing };
  expected[id2] = UnitOwnership::harbor{
    .port_status = PortStatus::in_port{},
    .sailed_from = nothing };
  auto& all = w.units().all();
  REQUIRE( all.size() == 2 );
  REQUIRE(
      all.contains( GenericUnitId{ to_underlying( id1 ) } ) );
  REQUIRE(
      all.contains( GenericUnitId{ to_underlying( id2 ) } ) );
  REQUIRE( as_const( w.units() ).ownership_of( id1 ) ==
           expected[id1] );
  REQUIRE( as_const( w.units() ).ownership_of( id2 ) ==
           expected[id2] );
}

TEST_CASE( "[harbor-units] unit_sail_to_new_world" ) {
  world w;
  Player& player = w.default_player();
  REQUIRE( w.old_world( player ).harbor_state.selected_unit ==
           nothing );
  UnitId caravel1 =
      w.add_unit_in_port( e_unit_type::caravel ).id();
  REQUIRE( w.old_world( player ).harbor_state.selected_unit ==
           caravel1 );
  REQUIRE( w.units().harbor_view_state_of( caravel1 ) ==
           UnitOwnership::harbor{ .port_status =
                                      PortStatus::in_port{} } );
  unit_sail_to_new_world( w.ss(), caravel1 );
  REQUIRE(
      w.units().harbor_view_state_of( caravel1 ) ==
      UnitOwnership::harbor{
        .port_status = PortStatus::outbound{ .turns = 0 } } );
}

TEST_WORLD( "[harbor-units] unit_sail_to_harbor" ) {
  using enum e_unit_type;
  Player& player = default_player();
  Coord const coord{ .x = 8, .y = 5 };
  Unit const& ship = add_unit_on_map( galleon, coord );
  Unit& cargo1 = add_unit_in_cargo( free_colonist, ship.id() );
  Unit& cargo2 = add_unit_in_cargo( free_colonist, ship.id() );
  Unit& cargo3 = add_unit_in_cargo( free_colonist, ship.id() );
  Unit& cargo4 = add_unit_in_cargo( free_colonist, ship.id() );
  Unit& cargo5 = add_unit_in_cargo( free_colonist, ship.id() );
  cargo1.clear_orders();
  cargo2.sentry();
  cargo3.fortify();
  cargo4.orders() = unit_orders::go_to{};
  cargo5.orders() = unit_orders::trade_route{};
  REQUIRE( units().maybe_harbor_view_state_of( ship.id() ) ==
           nothing );
  unit_sail_to_harbor( ss(), ship.id() );
  REQUIRE( old_world( player ).harbor_state.selected_unit ==
           nothing );
  REQUIRE( units().harbor_view_state_of( ship.id() ) ==
           UnitOwnership::harbor{
             .port_status = PortStatus::inbound{ .turns = 0 },
             .sailed_from = coord } );
  REQUIRE( ship.orders().holds<unit_orders::none>() );
  REQUIRE( cargo1.orders().holds<unit_orders::sentry>() );
  REQUIRE( cargo2.orders().holds<unit_orders::sentry>() );
  REQUIRE( cargo3.orders().holds<unit_orders::sentry>() );
  REQUIRE( cargo4.orders().holds<unit_orders::sentry>() );
  REQUIRE( cargo5.orders().holds<unit_orders::sentry>() );
}

TEST_CASE( "[harbor-units] unit_move_to_port" ) {
  world w;
  Player& player = w.default_player();

  // Add units.
  REQUIRE( w.old_world( player ).harbor_state.selected_unit ==
           nothing );
  UnitId caravel1 =
      w.add_unit_in_port( e_unit_type::caravel ).id();
  REQUIRE( w.old_world( player ).harbor_state.selected_unit ==
           caravel1 );
  UnitId caravel2 =
      w.add_unit_in_port( e_unit_type::caravel ).id();
  REQUIRE( w.old_world( player ).harbor_state.selected_unit ==
           caravel1 );

  UnitId merchantman1 =
      w.add_unit_in_port( e_unit_type::merchantman ).id();
  unit_sail_to_new_world( w.ss(), merchantman1 );

  UnitId merchantman2 =
      w.add_unit_on_map( e_unit_type::merchantman,
                         Coord{ .x = 8, .y = 5 } )
          .id();
  unit_sail_to_harbor( w.ss(), merchantman2 );
  UnitId merchantman3 =
      w.add_unit_on_map( e_unit_type::merchantman,
                         Coord{ .x = 8, .y = 5 } )
          .id();
  // Make sure that we can sail from the goto orders state.
  w.units().unit_for( merchantman3 ).orders() =
      unit_orders::go_to{};
  unit_sail_to_harbor( w.ss(), merchantman3 );
  REQUIRE( w.units()
               .unit_for( merchantman3 )
               .orders()
               .holds<unit_orders::none>() );

  UnitId free_colonist1 =
      w.add_unit_in_port( e_unit_type::free_colonist ).id();
  UnitId free_colonist2 =
      w.add_unit_in_port( e_unit_type::free_colonist ).id();

  Coord coord{ .x = 4, .y = 4 };
  UnitId soldier1 =
      w.add_unit_on_map( e_unit_type::soldier, coord ).id();
  UnitId soldier2 =
      w.add_unit_on_map( e_unit_type::soldier, coord ).id();

  // Sanity checks.
  REQUIRE( w.old_world( player ).harbor_state.selected_unit ==
           caravel1 );
  REQUIRE( w.units().harbor_view_state_of( caravel1 ) ==
           UnitOwnership::harbor{ .port_status =
                                      PortStatus::in_port{} } );
  REQUIRE( w.units().harbor_view_state_of( caravel2 ) ==
           UnitOwnership::harbor{ .port_status =
                                      PortStatus::in_port{} } );
  REQUIRE(
      w.units().harbor_view_state_of( merchantman1 ) ==
      UnitOwnership::harbor{
        .port_status = PortStatus::outbound{ .turns = 0 } } );
  REQUIRE( w.units().harbor_view_state_of( merchantman2 ) ==
           UnitOwnership::harbor{
             .port_status = PortStatus::inbound{ .turns = 0 },
             .sailed_from = Coord{ .x = 8, .y = 5 } } );
  REQUIRE( w.units().harbor_view_state_of( free_colonist1 ) ==
           UnitOwnership::harbor{ .port_status =
                                      PortStatus::in_port{} } );
  REQUIRE( w.units().harbor_view_state_of( free_colonist2 ) ==
           UnitOwnership::harbor{ .port_status =
                                      PortStatus::in_port{} } );
  REQUIRE( w.units().maybe_harbor_view_state_of( soldier1 ) ==
           nothing );
  REQUIRE( w.units().maybe_harbor_view_state_of( soldier2 ) ==
           nothing );

  // Make some of them have non-clear orders to simulate a situa-
  // tion where they are e.g. fortified at sea and get attacked
  // and damaged and suddenly moved to port.
  w.units().unit_for( caravel2 ).fortify();
  w.units().unit_for( merchantman1 ).sentry();
  w.units().unit_for( merchantman2 ).orders() =
      unit_orders::damaged{ .turns_until_repair = 1 };
  w.units().unit_for( soldier1 ).fortify();

  // Move them to port.
  unit_move_to_port( w.ss(), caravel1 );
  unit_move_to_port( w.ss(), caravel2 );
  unit_move_to_port( w.ss(), merchantman1 );
  unit_move_to_port( w.ss(), merchantman2 );
  unit_move_to_port( w.ss(), free_colonist1 );
  unit_move_to_port( w.ss(), free_colonist2 );
  unit_move_to_port( w.ss(), soldier1 );
  unit_move_to_port( w.ss(), soldier2 );
  REQUIRE( w.old_world( player ).harbor_state.selected_unit ==
           caravel1 );

  // Verify they are in port.
  REQUIRE( w.units().harbor_view_state_of( caravel1 ) ==
           UnitOwnership::harbor{ .port_status =
                                      PortStatus::in_port{} } );
  REQUIRE( w.units().harbor_view_state_of( caravel2 ) ==
           UnitOwnership::harbor{ .port_status =
                                      PortStatus::in_port{} } );
  REQUIRE( w.units().harbor_view_state_of( merchantman1 ) ==
           UnitOwnership::harbor{ .port_status =
                                      PortStatus::in_port{} } );
  REQUIRE( w.units().harbor_view_state_of( merchantman2 ) ==
           UnitOwnership::harbor{
             .port_status = PortStatus::in_port{},
             .sailed_from = Coord{ .x = 8, .y = 5 } } );
  REQUIRE( w.units().harbor_view_state_of( free_colonist1 ) ==
           UnitOwnership::harbor{ .port_status =
                                      PortStatus::in_port{} } );
  REQUIRE( w.units().harbor_view_state_of( free_colonist2 ) ==
           UnitOwnership::harbor{ .port_status =
                                      PortStatus::in_port{} } );
  REQUIRE( w.units().harbor_view_state_of( soldier1 ) ==
           UnitOwnership::harbor{ .port_status =
                                      PortStatus::in_port{} } );
  REQUIRE( w.units().harbor_view_state_of( soldier1 ) ==
           UnitOwnership::harbor{ .port_status =
                                      PortStatus::in_port{} } );

  // Verify the ships have clear orders.
  REQUIRE( w.units()
               .unit_for( caravel1 )
               .orders()
               .holds<unit_orders::none>() );
  REQUIRE( w.units()
               .unit_for( caravel2 )
               .orders()
               .holds<unit_orders::none>() );
  REQUIRE( w.units()
               .unit_for( merchantman1 )
               .orders()
               .holds<unit_orders::none>() );
  REQUIRE( w.units()
               .unit_for( merchantman2 )
               .orders()
               .holds<unit_orders::damaged>() );
  REQUIRE( w.units()
               .unit_for( soldier1 )
               .orders()
               .holds<unit_orders::fortified>() );
}

TEST_CASE( "[harbor-units] advance_unit_on_high_seas" ) {
  world w;
  Player& player = w.default_player();
  Coord coord{ .x = 8, .y = 5 };

  REQUIRE( w.old_world( player ).harbor_state.selected_unit ==
           nothing );
  UnitId id =
      w.add_unit_on_map( e_unit_type::merchantman, coord ).id();
  REQUIRE( w.units().maybe_harbor_view_state_of( id ) ==
           nothing );

  unit_sail_to_harbor( w.ss(), id );
  REQUIRE( w.old_world( player ).harbor_state.selected_unit ==
           nothing );
  REQUIRE( w.units().maybe_harbor_view_state_of( id ) ==
           UnitOwnership::harbor{
             .port_status = PortStatus::inbound{ .turns = 0 },
             .sailed_from = coord } );

  // Let's turn it around.
  unit_sail_to_new_world( w.ss(), id );
  REQUIRE( advance_unit_on_high_seas( w.ss(), w.dutch(), id ) ==
           e_high_seas_result::arrived_in_new_world );
  REQUIRE( w.old_world( player ).harbor_state.selected_unit ==
           nothing );
  REQUIRE( w.units().maybe_harbor_view_state_of( id ) ==
           UnitOwnership::harbor{
             .port_status = PortStatus::outbound{ .turns = 2 },
             .sailed_from = coord } );
  REQUIRE(
      find_new_world_arrival_square(
          w.ss(), w.map_updater().connectivity(), w.dutch(),
          w.units().harbor_view_state_of( id ).sailed_from ) ==
      coord );

  // Sail once again.
  unit_sail_to_harbor( w.ss(), id );
  REQUIRE( w.old_world( player ).harbor_state.selected_unit ==
           nothing );
  REQUIRE( w.units().maybe_harbor_view_state_of( id ) ==
           UnitOwnership::harbor{
             .port_status = PortStatus::inbound{ .turns = 0 },
             .sailed_from = coord } );

  // Now advance.
  REQUIRE( advance_unit_on_high_seas( w.ss(), w.dutch(), id ) ==
           e_high_seas_result::still_traveling );
  REQUIRE( w.old_world( player ).harbor_state.selected_unit ==
           nothing );
  REQUIRE( w.units().maybe_harbor_view_state_of( id ) ==
           UnitOwnership::harbor{
             .port_status = PortStatus::inbound{ .turns = 1 },
             .sailed_from = coord } );

  // Turn around.
  unit_sail_to_new_world( w.ss(), id );
  REQUIRE( w.old_world( player ).harbor_state.selected_unit ==
           nothing );
  REQUIRE( w.units().maybe_harbor_view_state_of( id ) ==
           UnitOwnership::harbor{
             .port_status = PortStatus::outbound{ .turns = 1 },
             .sailed_from = coord } );

  // Turn back around and advance again.
  unit_sail_to_harbor( w.ss(), id );
  REQUIRE( advance_unit_on_high_seas( w.ss(), w.dutch(), id ) ==
           e_high_seas_result::arrived_in_harbor );
  REQUIRE( w.old_world( player ).harbor_state.selected_unit ==
           id );
  REQUIRE( w.units().maybe_harbor_view_state_of( id ) ==
           UnitOwnership::harbor{
             .port_status = PortStatus::in_port{},
             .sailed_from = coord } );
  REQUIRE(
      find_new_world_arrival_square(
          w.ss(), w.map_updater().connectivity(), w.dutch(),
          w.units().harbor_view_state_of( id ).sailed_from ) ==
      coord );

  // Sail back to the new world.
  unit_sail_to_new_world( w.ss(), id );
  REQUIRE( w.old_world( player ).harbor_state.selected_unit ==
           id );
  REQUIRE( w.units().maybe_harbor_view_state_of( id ) ==
           UnitOwnership::harbor{
             .port_status = PortStatus::outbound{ .turns = 0 },
             .sailed_from = coord } );

  // Now advance.
  REQUIRE( advance_unit_on_high_seas( w.ss(), w.dutch(), id ) ==
           e_high_seas_result::still_traveling );
  REQUIRE( w.old_world( player ).harbor_state.selected_unit ==
           id );
  REQUIRE( w.units().maybe_harbor_view_state_of( id ) ==
           UnitOwnership::harbor{
             .port_status = PortStatus::outbound{ .turns = 1 },
             .sailed_from = coord } );

  // Now advance.
  REQUIRE( advance_unit_on_high_seas( w.ss(), w.dutch(), id ) ==
           e_high_seas_result::arrived_in_new_world );
  REQUIRE( w.old_world( player ).harbor_state.selected_unit ==
           id );
  REQUIRE( w.units().maybe_harbor_view_state_of( id ) ==
           UnitOwnership::harbor{
             .port_status = PortStatus::outbound{ .turns = 2 },
             .sailed_from = coord } );

  // Now advance.
  REQUIRE( advance_unit_on_high_seas( w.ss(), w.dutch(), id ) ==
           e_high_seas_result::arrived_in_new_world );
  REQUIRE( w.old_world( player ).harbor_state.selected_unit ==
           id );
  REQUIRE( w.units().maybe_harbor_view_state_of( id ) ==
           UnitOwnership::harbor{
             .port_status = PortStatus::outbound{ .turns = 2 },
             .sailed_from = coord } );

  REQUIRE(
      find_new_world_arrival_square(
          w.ss(), w.map_updater().connectivity(), w.dutch(),
          w.units().harbor_view_state_of( id ).sailed_from ) ==
      coord );
}

TEST_CASE( "[harbor-units] find_new_world_arrival_square" ) {
  world w;
  Coord const starting{ .x = 8, .y = 3 };
  Coord const ship_loc{ .x = 8, .y = 5 };

  w.dutch().starting_position = starting;

  UnitId caravel1 =
      w.add_unit_on_map( e_unit_type::caravel, ship_loc ).id();

  // This unit was never in the old world, so will not have a
  // sailed_from set.  We're going to test where it gets placed
  // when it sails to the new world.
  UnitId caravel2 =
      w.add_unit_in_port( e_unit_type::caravel ).id();

  SECTION( "no ships sailed" ) {
    unit_sail_to_new_world( w.ss(), caravel2 );
    REQUIRE( find_new_world_arrival_square(
                 w.ss(), w.map_updater().connectivity(),
                 w.dutch(),
                 w.units()
                     .harbor_view_state_of( caravel2 )
                     .sailed_from ) == starting );
  }
  SECTION( "other ship sailed" ) {
    // This will cause caravel1's map position to be recorded and
    // it should then be used as the position to place caravel2,
    // since caravel2 never had a map position.
    unit_sail_to_harbor( w.ss(), caravel1 );
    unit_sail_to_new_world( w.ss(), caravel2 );
    REQUIRE( find_new_world_arrival_square(
                 w.ss(), w.map_updater().connectivity(),
                 w.dutch(),
                 w.units()
                     .harbor_view_state_of( caravel2 )
                     .sailed_from ) == ship_loc );
  }
  SECTION( "did sail from new world" ) {
    unit_sail_to_harbor( w.ss(), caravel1 );
    unit_move_to_port( w.ss(), caravel1 );
    unit_sail_to_new_world( w.ss(), caravel1 );
    int tries = 0;
    do {
      REQUIRE( !is_unit_inbound( w.units(), caravel1 ) );
      REQUIRE( is_unit_outbound( w.units(), caravel1 ) );
      REQUIRE( !is_unit_in_port( w.units(), caravel1 ) );
      e_high_seas_result res = advance_unit_on_high_seas(
          w.ss(), w.dutch(), caravel1 );
      if( res == e_high_seas_result::arrived_in_new_world )
        break;
      ++tries;
    } while( tries < 10 );
    REQUIRE( tries < 10 );
    REQUIRE( w.units().maybe_harbor_view_state_of( caravel1 ) ==
             UnitOwnership::harbor{
               .port_status = PortStatus::outbound{ .turns = 2 },
               .sailed_from = ship_loc } );
    REQUIRE( find_new_world_arrival_square(
                 w.ss(), w.map_updater().connectivity(),
                 w.dutch(),
                 w.units()
                     .harbor_view_state_of( caravel1 )
                     .sailed_from ) == ship_loc );
  }
}

TEST_CASE(
    "[harbor-units] find_new_world_arrival_square with foreign "
    "unit" ) {
  world w;

  SECTION( "friendly unit" ) {
    Coord const ship_loc{ .x = 8, .y = 5 };
    UnitId dutch_caravel =
        w.add_unit_on_map( e_unit_type::caravel, ship_loc,
                           e_player::dutch )
            .id();
    unit_sail_to_harbor( w.ss(), dutch_caravel );
    unit_move_to_port( w.ss(), dutch_caravel );
    REQUIRE( w.units().from_coord( ship_loc ).empty() );

    UnitId dutch_caravel2 =
        w.add_unit_on_map( e_unit_type::caravel, ship_loc,
                           e_player::dutch )
            .id();
    REQUIRE( find_new_world_arrival_square(
                 w.ss(), w.map_updater().connectivity(),
                 w.dutch(),
                 w.units()
                     .harbor_view_state_of( dutch_caravel )
                     .sailed_from ) == ship_loc );
    REQUIRE( w.units().from_coord( ship_loc ).size() == 1 );
    REQUIRE( w.units()
                 .from_coord( ship_loc )
                 .contains( GenericUnitId{
                   to_underlying( dutch_caravel2 ) } ) );
  }

  SECTION( "foreign unit" ) {
    Coord const ship_loc{ .x = 8, .y = 5 };
    UnitId dutch_caravel =
        w.add_unit_on_map( e_unit_type::caravel, ship_loc,
                           e_player::dutch )
            .id();
    unit_sail_to_harbor( w.ss(), dutch_caravel );
    unit_move_to_port( w.ss(), dutch_caravel );
    REQUIRE( w.units().from_coord( ship_loc ).empty() );

    UnitId french_caravel =
        w.add_unit_on_map( e_unit_type::caravel, ship_loc,
                           e_player::french )
            .id();
    REQUIRE( w.units().from_coord( ship_loc ).size() == 1 );
    REQUIRE( w.units()
                 .from_coord( ship_loc )
                 .contains( GenericUnitId{
                   to_underlying( french_caravel ) } ) );
    Coord const expected{ .x = 7, .y = 4 };
    REQUIRE( find_new_world_arrival_square(
                 w.ss(), w.map_updater().connectivity(),
                 w.dutch(),
                 w.units()
                     .harbor_view_state_of( dutch_caravel )
                     .sailed_from ) == expected );
  }

  SECTION( "map edge + foreign units" ) {
    Coord const ship_loc{ .x = 0, .y = 0 };
    UnitId dutch_caravel =
        w.add_unit_on_map( e_unit_type::caravel, ship_loc,
                           e_player::dutch )
            .id();
    unit_sail_to_harbor( w.ss(), dutch_caravel );
    unit_move_to_port( w.ss(), dutch_caravel );
    REQUIRE( w.units().from_coord( ship_loc ).empty() );

    w.add_unit_on_map( e_unit_type::caravel, ship_loc,
                       e_player::french );
    Coord const expected{ .x = 1, .y = 0 };
    REQUIRE( find_new_world_arrival_square(
                 w.ss(), w.map_updater().connectivity(),
                 w.dutch(),
                 w.units()
                     .harbor_view_state_of( dutch_caravel )
                     .sailed_from ) == expected );
  }

  SECTION( "map edge + a lot foreign units" ) {
    Coord const ship_loc{ .x = 0, .y = 0 };
    UnitId dutch_caravel =
        w.add_unit_on_map( e_unit_type::caravel, ship_loc,
                           e_player::dutch )
            .id();
    unit_sail_to_harbor( w.ss(), dutch_caravel );
    unit_move_to_port( w.ss(), dutch_caravel );
    REQUIRE( w.units().from_coord( ship_loc ).empty() );

    w.add_unit_on_map(
        e_unit_type::caravel,
        ship_loc + Delta{ .h = 0 } + Delta{ .w = 0 },
        e_player::french );
    w.add_unit_on_map(
        e_unit_type::caravel,
        ship_loc + Delta{ .h = 0 } + Delta{ .w = 1 },
        e_player::french );
    w.add_unit_on_map(
        e_unit_type::caravel,
        ship_loc + Delta{ .h = 0 } + Delta{ .w = 2 },
        e_player::french );
    w.add_unit_on_map(
        e_unit_type::caravel,
        ship_loc + Delta{ .h = 1 } + Delta{ .w = 0 },
        e_player::french );
    w.add_unit_on_map(
        e_unit_type::caravel,
        ship_loc + Delta{ .h = 1 } + Delta{ .w = 1 },
        e_player::french );
    w.add_unit_on_map(
        e_unit_type::caravel,
        ship_loc + Delta{ .h = 1 } + Delta{ .w = 2 },
        e_player::french );
    w.add_unit_on_map(
        e_unit_type::caravel,
        ship_loc + Delta{ .h = 2 } + Delta{ .w = 0 },
        e_player::french );
    w.add_unit_on_map(
        e_unit_type::caravel,
        ship_loc + Delta{ .h = 2 } + Delta{ .w = 1 },
        e_player::french );
    Coord const expected{ .x = 3, .y = 0 };
    REQUIRE( find_new_world_arrival_square(
                 w.ss(), w.map_updater().connectivity(),
                 w.dutch(),
                 w.units()
                     .harbor_view_state_of( dutch_caravel )
                     .sailed_from ) == expected );
  }

  SECTION( "no squares available due to foreign units" ) {
    Coord const ship_loc{ .x = 0, .y = 0 };
    UnitId dutch_caravel =
        w.add_unit_on_map( e_unit_type::caravel, ship_loc,
                           e_player::dutch )
            .id();
    unit_sail_to_harbor( w.ss(), dutch_caravel );
    unit_move_to_port( w.ss(), dutch_caravel );
    REQUIRE( w.units().from_coord( ship_loc ).empty() );

    // Fill all ocean squares with foreign units to test the case
    // where we cannot find any where to place the unit.
    for( Rect const r :
         gfx::subrects( w.terrain().world_rect_tiles() ) )
      if( w.terrain().square_at( r.upper_left() ).surface ==
          e_surface::water )
        w.add_unit_on_map( e_unit_type::caravel, r.upper_left(),
                           e_player::french );

    REQUIRE( find_new_world_arrival_square(
                 w.ss(), w.map_updater().connectivity(),
                 w.dutch(),
                 w.units()
                     .harbor_view_state_of( dutch_caravel )
                     .sailed_from ) == nothing );
  }

  SECTION( "no squares available due to inland lake" ) {
    Coord const ship_loc{ .x = 0, .y = 0 };
    UnitId dutch_caravel =
        w.add_unit_on_map( e_unit_type::caravel, ship_loc,
                           e_player::dutch )
            .id();
    unit_sail_to_harbor( w.ss(), dutch_caravel );
    unit_move_to_port( w.ss(), dutch_caravel );
    REQUIRE( w.units().from_coord( ship_loc ).empty() );

    REQUIRE( find_new_world_arrival_square(
                 w.ss(), w.map_updater().connectivity(),
                 w.dutch(),
                 w.units()
                     .harbor_view_state_of( dutch_caravel )
                     .sailed_from ) == Coord{ .x = 0, .y = 0 } );

    Delta const world_size = w.terrain().world_size_tiles();
    // Fill the perimeter of the map with land tiles so that all
    // of the ocean tiles are inland lake tiles.
    for( int y = 0; y < world_size.h; ++y ) {
      w.square( { .x = 0, .y = y } ).surface = e_surface::land;
      w.square( { .x = world_size.w - 1, .y = y } ).surface =
          e_surface::land;
    }
    for( int x = 0; x < world_size.w; ++x ) {
      w.square( { .x = x, .y = 0 } ).surface = e_surface::land;
      w.square( { .x = x, .y = world_size.h - 1 } ).surface =
          e_surface::land;
    }

    REQUIRE( find_new_world_arrival_square(
                 w.ss(), w.map_updater().connectivity(),
                 w.dutch(),
                 w.units()
                     .harbor_view_state_of( dutch_caravel )
                     .sailed_from ) == nothing );
  }
}

TEST_CASE( "[harbor-units] sail west edge" ) {
  world w;
  Coord const coord{ .x = 0, .y = 0 };
  UnitId id =
      w.add_unit_on_map( e_unit_type::caravel, coord ).id();
  REQUIRE( w.units().maybe_harbor_view_state_of( id ) ==
           nothing );
  unit_sail_to_harbor( w.ss(), id );
  REQUIRE( w.units().harbor_view_state_of( id ) ==
           UnitOwnership::harbor{
             .port_status = PortStatus::inbound{ .turns = 0 },
             .sailed_from = coord } );

  // Now advance.
  REQUIRE( advance_unit_on_high_seas( w.ss(), w.dutch(), id ) ==
           e_high_seas_result::still_traveling );
  REQUIRE( w.units().maybe_harbor_view_state_of( id ) ==
           UnitOwnership::harbor{
             .port_status = PortStatus::inbound{ .turns = 1 },
             .sailed_from = coord } );

  // Now advance.
  REQUIRE( advance_unit_on_high_seas( w.ss(), w.dutch(), id ) ==
           e_high_seas_result::still_traveling );
  REQUIRE( w.units().maybe_harbor_view_state_of( id ) ==
           UnitOwnership::harbor{
             .port_status = PortStatus::inbound{ .turns = 2 },
             .sailed_from = coord } );

  // Now advance.
  REQUIRE( advance_unit_on_high_seas( w.ss(), w.dutch(), id ) ==
           e_high_seas_result::still_traveling );
  REQUIRE( w.units().maybe_harbor_view_state_of( id ) ==
           UnitOwnership::harbor{
             .port_status = PortStatus::inbound{ .turns = 3 },
             .sailed_from = coord } );

  // Now advance.
  REQUIRE( advance_unit_on_high_seas( w.ss(), w.dutch(), id ) ==
           e_high_seas_result::arrived_in_harbor );
  REQUIRE( w.units().maybe_harbor_view_state_of( id ) ==
           UnitOwnership::harbor{
             .port_status = PortStatus::in_port{},
             .sailed_from = coord } );
}

TEST_CASE( "[harbor-units] sail east edge" ) {
  world w;
  Coord const coord{ .x = 9, .y = 0 };
  UnitId id =
      w.add_unit_on_map( e_unit_type::caravel, coord ).id();
  REQUIRE( w.units().maybe_harbor_view_state_of( id ) ==
           nothing );
  unit_sail_to_harbor( w.ss(), id );
  REQUIRE( w.units().harbor_view_state_of( id ) ==
           UnitOwnership::harbor{
             .port_status = PortStatus::inbound{ .turns = 0 },
             .sailed_from = coord } );

  // Now advance.
  REQUIRE( advance_unit_on_high_seas( w.ss(), w.dutch(), id ) ==
           e_high_seas_result::still_traveling );
  REQUIRE( w.units().maybe_harbor_view_state_of( id ) ==
           UnitOwnership::harbor{
             .port_status = PortStatus::inbound{ .turns = 1 },
             .sailed_from = coord } );

  // Now advance.
  REQUIRE( advance_unit_on_high_seas( w.ss(), w.dutch(), id ) ==
           e_high_seas_result::arrived_in_harbor );
  REQUIRE( w.units().maybe_harbor_view_state_of( id ) ==
           UnitOwnership::harbor{
             .port_status = PortStatus::in_port{},
             .sailed_from = coord } );
}

TEST_CASE(
    "[harbor-units] update_harbor_selected_unit idempotency" ) {
  world w;

  Player& player = w.default_player();

  maybe<UnitId>& selected_unit =
      w.old_world( player ).harbor_state.selected_unit;

  auto f = [&] {
    update_harbor_selected_unit( w.ss(), player );
  };

  UnitId const galleon_id =
      w.add_free_unit( e_unit_type::galleon ).id();

  REQUIRE( selected_unit == nothing );

  UnitId const privateer_id =
      w.add_unit_in_port( e_unit_type::privateer ).id();
  REQUIRE( selected_unit == privateer_id );

  f();
  REQUIRE( selected_unit == privateer_id );

  UnitId const caravel_id =
      w.add_unit_in_port( e_unit_type::caravel ).id();
  REQUIRE( selected_unit == privateer_id );

  f();
  REQUIRE( selected_unit == privateer_id );

  selected_unit = caravel_id;
  REQUIRE( selected_unit == caravel_id );

  f();
  REQUIRE( selected_unit == caravel_id );

  f();
  REQUIRE( selected_unit == caravel_id );

  selected_unit = privateer_id;
  REQUIRE( selected_unit == privateer_id );

  f();
  REQUIRE( selected_unit == privateer_id );

  // Now move the galleon which has a smaller id than the other
  // ships; make sure it doesn't get auto selected because of
  // that.
  BASE_CHECK( galleon_id < privateer_id );
  BASE_CHECK( galleon_id < caravel_id );
  unit_move_to_port( w.ss(), galleon_id );

  REQUIRE( selected_unit == privateer_id );
  f();
  REQUIRE( selected_unit == privateer_id );
}

TEST_CASE(
    "[harbor-units] update_harbor_selected_unit unit "
    "destruction" ) {
  world w;
  Player& player = w.default_player();
  REQUIRE( w.old_world( player ).harbor_state.selected_unit ==
           nothing );
  UnitId id1 = create_unit_in_harbor( w.ss(), player,
                                      e_unit_type::galleon );
  REQUIRE( w.old_world( player ).harbor_state.selected_unit ==
           id1 );
  UnitId id2 = create_unit_in_harbor( w.ss(), player,
                                      e_unit_type::galleon );
  REQUIRE( w.old_world( player ).harbor_state.selected_unit ==
           id1 );
  UnitOwnershipChanger( w.ss(), id1 ).destroy();
  REQUIRE( w.old_world( player ).harbor_state.selected_unit ==
           nothing );
  update_harbor_selected_unit( w.ss(), player );
  REQUIRE( w.old_world( player ).harbor_state.selected_unit ==
           id2 );
  UnitOwnershipChanger( w.ss(), id2 ).destroy();
  REQUIRE( w.old_world( player ).harbor_state.selected_unit ==
           nothing );
  update_harbor_selected_unit( w.ss(), player );
  REQUIRE( w.old_world( player ).harbor_state.selected_unit ==
           nothing );
}

TEST_CASE( "[harbor-units] unit ordering on dock" ) {
  world w;
  vector<UnitId> expected;

  auto const f = [&] {
    return harbor_units_on_dock( w.units(),
                                 w.default_player_type() );
  };

  UnitId const unit1 =
      w.add_free_unit( e_unit_type::free_colonist ).id();
  UnitId const unit2 =
      w.add_free_unit( e_unit_type::free_colonist ).id();
  UnitId const unit3 =
      w.add_free_unit( e_unit_type::free_colonist ).id();
  UnitId const unit4 =
      w.add_free_unit( e_unit_type::free_colonist ).id();
  UnitId const unit5 =
      w.add_free_unit( e_unit_type::free_colonist ).id();
  UnitId const unit6 =
      w.add_free_unit( e_unit_type::free_colonist ).id();
  UnitId const unit7 =
      w.add_free_unit( e_unit_type::free_colonist ).id();

  expected = {};
  REQUIRE( f() == expected );

  unit_move_to_port( w.ss(), unit4 );
  unit_move_to_port( w.ss(), unit6 );
  unit_move_to_port( w.ss(), unit1 );
  unit_move_to_port( w.ss(), unit3 );
  unit_move_to_port( w.ss(), unit2 );
  unit_move_to_port( w.ss(), unit7 );
  unit_move_to_port( w.ss(), unit5 );

  REQUIRE( w.units().unit_ordering( unit4 ) == 1 );
  REQUIRE( w.units().unit_ordering( unit6 ) == 2 );
  REQUIRE( w.units().unit_ordering( unit1 ) == 3 );
  REQUIRE( w.units().unit_ordering( unit3 ) == 4 );
  REQUIRE( w.units().unit_ordering( unit2 ) == 5 );
  REQUIRE( w.units().unit_ordering( unit7 ) == 6 );
  REQUIRE( w.units().unit_ordering( unit5 ) == 7 );

  expected = { unit5, unit7, unit2, unit3, unit1, unit6, unit4 };
  REQUIRE( f() == expected );
}

TEST_CASE( "[harbor-units] unit ordering in port" ) {
  world w;
  vector<UnitId> expected;

  auto const f = [&] {
    return harbor_units_in_port( w.units(),
                                 w.default_player_type() );
  };

  UnitId const unit1 =
      w.add_free_unit( e_unit_type::caravel ).id();
  UnitId const unit2 =
      w.add_free_unit( e_unit_type::caravel ).id();
  UnitId const unit3 =
      w.add_free_unit( e_unit_type::caravel ).id();
  UnitId const unit4 =
      w.add_free_unit( e_unit_type::caravel ).id();
  UnitId const unit5 =
      w.add_free_unit( e_unit_type::caravel ).id();
  UnitId const unit6 =
      w.add_free_unit( e_unit_type::caravel ).id();
  UnitId const unit7 =
      w.add_free_unit( e_unit_type::caravel ).id();

  expected = {};
  REQUIRE( f() == expected );

  unit_move_to_port( w.ss(), unit4 );
  unit_move_to_port( w.ss(), unit6 );
  unit_move_to_port( w.ss(), unit1 );
  unit_move_to_port( w.ss(), unit3 );
  unit_move_to_port( w.ss(), unit2 );
  unit_move_to_port( w.ss(), unit7 );
  unit_move_to_port( w.ss(), unit5 );

  REQUIRE( w.units().unit_ordering( unit4 ) == 1 );
  REQUIRE( w.units().unit_ordering( unit6 ) == 2 );
  REQUIRE( w.units().unit_ordering( unit1 ) == 3 );
  REQUIRE( w.units().unit_ordering( unit3 ) == 4 );
  REQUIRE( w.units().unit_ordering( unit2 ) == 5 );
  REQUIRE( w.units().unit_ordering( unit7 ) == 6 );
  REQUIRE( w.units().unit_ordering( unit5 ) == 7 );

  expected = { unit5, unit7, unit2, unit3, unit1, unit6, unit4 };
  REQUIRE( f() == expected );
}

TEST_CASE( "[harbor-units] try_select_in_port_ship" ) {
  world w;

  Player& player = w.default_player();

  maybe<UnitId>& selected_unit =
      w.old_world( player ).harbor_state.selected_unit;

  auto f = [&] { try_select_in_port_ship( w.ss(), player ); };

  UnitId const galleon_id =
      w.add_free_unit( e_unit_type::galleon ).id();

  REQUIRE( selected_unit == nothing );

  UnitId const privateer_id =
      w.add_unit_in_port( e_unit_type::privateer ).id();
  REQUIRE( selected_unit == privateer_id );

  f();
  REQUIRE( selected_unit == privateer_id );

  UnitId const caravel_id =
      w.add_unit_in_port( e_unit_type::caravel ).id();
  REQUIRE( selected_unit == privateer_id );

  f();
  REQUIRE( selected_unit == privateer_id );

  unit_sail_to_new_world( w.ss(), privateer_id );
  f();
  REQUIRE( selected_unit == caravel_id );

  unit_sail_to_new_world( w.ss(), caravel_id );
  f();
  REQUIRE( selected_unit == caravel_id );

  unit_move_to_port( w.ss(), galleon_id );
  REQUIRE( selected_unit == caravel_id );
  f();
  REQUIRE( selected_unit == galleon_id );

  f();
  REQUIRE( selected_unit == galleon_id );
}

} // namespace
} // namespace rn
