/****************************************************************
**command-move.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-07-25.
*
* Description: Unit tests for the src/command-move.* module.
*
*****************************************************************/
#include "test/mocking.hpp"
#include "test/testing.hpp"

// Under test.
#include "src/command-move.hpp"

// Testing
#include "test/fake/world.hpp"
#include "test/mocks/iagent.hpp"
#include "test/mocks/icolony-viewer.hpp"
#include "test/mocks/icombat.hpp"
#include "test/mocks/iengine.hpp"
#include "test/mocks/igui.hpp"
#include "test/mocks/irand.hpp"
#include "test/mocks/land-view-plane.hpp"
#include "test/util/coro.hpp"

// Revolution Now
#include "src/harbor-units.hpp"
#include "src/map-square.hpp"
#include "src/plane-stack.hpp"

// config
#include "src/config/unit-type.rds.hpp"

// ss
#include "ss/dwelling.rds.hpp"
#include "ss/events.rds.hpp"
#include "ss/goto.rds.hpp"
#include "ss/old-world-state.rds.hpp"
#include "ss/player.hpp"
#include "ss/revolution.rds.hpp"
#include "ss/settings.hpp"
#include "ss/terrain.hpp"
#include "ss/trade-route.rds.hpp"
#include "ss/unit.hpp"
#include "ss/units.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;
using namespace signal;

using ::gfx::point;
using ::mock::matchers::_;
using ::mock::matchers::Field;
using ::mock::matchers::StrContains;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct world : testing::World {
  using Base = testing::World;
  world() : Base() {
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
    MapSquare const S = make_sea_lane();
    // clang-format off
    vector<MapSquare> tiles{
      _, L, _, L, L, L, S, S, S,
      L, L, L, L, L, L, S, S, S,
      _, L, L, L, L, L, S, S, S,
      _, L, L, L, _, L, S, S, S,
      _, L, L, L, L, L, S, S, S,
      _, L, L, L, L, L, S, S, S,
    };
    // clang-format on
    build_map( std::move( tiles ), 9 );
    add_player( e_player::dutch );
    add_player( e_player::french );
    set_default_player_type( e_player::dutch );

    // This is so that we don't try to pop up a box telling the
    // player that they've discovered the new world.
    player( e_player::dutch ).new_world_name  = "";
    player( e_player::french ).new_world_name = "";
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
// This tests the case where a ship is on a land square adjacent
// to ocean and that it can move from the land square onto the
// ocean. This can happen if the ship in the port of a colony.
TEST_CASE( "[command-move] ship can move from land to ocean" ) {
  world W;
  MockLandViewPlane mock_land_view;
  W.planes().get().set_bottom<ILandViewPlane>( mock_land_view );
  Player& player = W.default_player();
  UnitId id      = W.add_unit_on_map( e_unit_type::galleon,
                                      Coord{ .x = 1, .y = 1 } )
                  .id();
  // Sanity check to make sure we are testing what we think we're
  // testing.
  REQUIRE( is_land( W.square( W.units().coord_for( id ) ) ) );
  REQUIRE( W.units().coord_for( id ) ==
           Coord{ .x = 1, .y = 1 } );
  REQUIRE( W.units().unit_for( id ).desc().ship );

  {
    // First make sure that it can't move from land to land.
    unique_ptr<CommandHandler> handler = handle_command(
        W.engine(), W.ss(), W.ts(), W.agent(), player, id,
        command::move{ .d = e_direction::n } );
    wait<bool> w_confirm = handler->confirm();
    REQUIRE( !w_confirm.exception() );
    REQUIRE( w_confirm.ready() );
    REQUIRE( *w_confirm == false );
    REQUIRE( W.units().coord_for( id ) ==
             Coord{ .x = 1, .y = 1 } );
  }

  {
    // Now make sure that it can move from land to water.
    unique_ptr<CommandHandler> handler = handle_command(
        W.engine(), W.ss(), W.ts(), W.agent(), player, id,
        command::move{ .d = e_direction::nw } );
    wait<bool> w_confirm = handler->confirm();
    REQUIRE( !w_confirm.exception() );
    REQUIRE( w_confirm.ready() );
    REQUIRE( *w_confirm == true );
    REQUIRE( W.units().coord_for( id ) ==
             Coord{ .x = 1, .y = 1 } );

    mock_land_view.EXPECT__animate_if_visible( _ );
    wait<> w_perform = handler->perform();
    REQUIRE( !w_perform.exception() );
    REQUIRE( w_perform.ready() );
    REQUIRE( W.units().coord_for( id ) ==
             Coord{ .x = 0, .y = 0 } );
  }
}

TEST_CASE( "[command-move] ship can't move into inland lake" ) {
  world W;
  Player& player = W.default_player();
  UnitId id      = W.add_unit_on_map( e_unit_type::galleon,
                                      Coord{ .x = 4, .y = 2 } )
                  .id();
  // Sanity check to make sure we are testing what we think we're
  // testing.
  REQUIRE( is_land( W.square( W.units().coord_for( id ) ) ) );
  REQUIRE( W.units().coord_for( id ) ==
           Coord{ .x = 4, .y = 2 } );
  REQUIRE( W.units().unit_for( id ).desc().ship );

  unique_ptr<CommandHandler> handler = handle_command(
      W.engine(), W.ss(), W.ts(), W.agent(), player, id,
      command::move{ .d = e_direction::s } );
  W.agent().EXPECT__message_box( StrContains( "inland lake" ) );
  wait<bool> w_confirm = handler->confirm();
  REQUIRE( !w_confirm.exception() );
  REQUIRE( w_confirm.ready() );
  REQUIRE( *w_confirm == false );
  REQUIRE( W.units().coord_for( id ) ==
           Coord{ .x = 4, .y = 2 } );
}

TEST_CASE(
    "[command-move] consumption of movement points when moving "
    "into a colony" ) {
  world W;
  MockLandViewPlane land_view_plane;
  W.planes().get().set_bottom<ILandViewPlane>( land_view_plane );
  Player& player = W.default_player();
  W.add_colony( { .x = 1, .y = 1 } );
  Unit const& missionary = W.add_unit_on_map(
      e_unit_type::missionary, { .x = 0, .y = 1 } );
  Unit const& free_colonist = W.add_unit_on_map(
      e_unit_type::free_colonist, { .x = 2, .y = 1 } );
  Unit const& wagon_train = W.add_unit_on_map(
      e_unit_type::wagon_train, { .x = 1, .y = 0 } );
  Unit const& privateer = W.add_unit_on_map(
      e_unit_type::privateer, { .x = 0, .y = 0 } );

  auto move_unit = [&]( UnitId unit_id, e_direction d ) {
    INFO( format( "unit_id={}, d={}", unit_id, d ) );
    land_view_plane.EXPECT__animate_if_visible( _ );
    unique_ptr<CommandHandler> handler = handle_command(
        W.engine(), W.ss(), W.ts(), W.agent(), player, unit_id,
        command::move{ .d = d } );
    wait<CommandHandlerRunResult> const w = handler->run();
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    CommandHandlerRunResult const expected_result{
      .order_was_run = true, .units_to_prioritize = {} };
    REQUIRE( *w == expected_result );
  };

  // Allow the free colonist to move along a road. This tests
  // that a unit is allowed to expend less than one movement
  // point to enter a colony in the relevant situation.
  W.square( { .x = 1, .y = 1 } ).road = true;
  W.square( { .x = 2, .y = 1 } ).road = true;
  // This will make sure that the movement points required to
  // enter a colony square are capped at 1.
  W.square( { .x = 1, .y = 1 } ).overlay = e_land_overlay::hills;

  // Sanity check.
  REQUIRE( missionary.movement_points() == 2 );
  REQUIRE( free_colonist.movement_points() == 1 );
  REQUIRE( wagon_train.movement_points() == 2 );
  REQUIRE( privateer.movement_points() == 8 );
  REQUIRE( W.units().coord_for( missionary.id() ) ==
           Coord{ .x = 0, .y = 1 } );
  REQUIRE( W.units().coord_for( free_colonist.id() ) ==
           Coord{ .x = 2, .y = 1 } );
  REQUIRE( W.units().coord_for( wagon_train.id() ) ==
           Coord{ .x = 1, .y = 0 } );
  REQUIRE( W.units().coord_for( privateer.id() ) ==
           Coord{ .x = 0, .y = 0 } );

  move_unit( missionary.id(), e_direction::e );
  move_unit( free_colonist.id(), e_direction::w );
  move_unit( wagon_train.id(), e_direction::s );
  move_unit( privateer.id(), e_direction::se );

  // No road; consumes one movement point.
  REQUIRE( missionary.movement_points() == 1 );

  // Road; consumes 1/3 point.
  REQUIRE( free_colonist.movement_points() ==
           MovementPoints::_2_3() );

  // This is the OG behavior; can be changed via the unit-type
  // config (ends_turn_in_colony).
  REQUIRE( wagon_train.movement_points() == 0 );
  REQUIRE( privateer.movement_points() == 0 );
  REQUIRE( W.units().coord_for( missionary.id() ) ==
           Coord{ .x = 1, .y = 1 } );
  REQUIRE( W.units().coord_for( free_colonist.id() ) ==
           Coord{ .x = 1, .y = 1 } );
  REQUIRE( W.units().coord_for( wagon_train.id() ) ==
           Coord{ .x = 1, .y = 1 } );
  REQUIRE( W.units().coord_for( privateer.id() ) ==
           Coord{ .x = 1, .y = 1 } );
}

TEST_CASE(
    "[command-move] opening of colony view when moving into a "
    "colony" ) {
  world w;
  MockLandViewPlane land_view_plane;
  w.planes().get().set_bottom<ILandViewPlane>( land_view_plane );
  Player& player            = w.default_player();
  Colony const& colony      = w.add_colony( { .x = 1, .y = 1 } );
  Unit const& free_colonist = w.add_unit_on_map(
      e_unit_type::free_colonist, { .x = 2, .y = 1 } );
  Unit const& wagon_train = w.add_unit_on_map(
      e_unit_type::wagon_train, { .x = 1, .y = 0 } );
  Unit const& privateer = w.add_unit_on_map(
      e_unit_type::privateer, { .x = 0, .y = 0 } );

  auto move_unit = [&]( UnitId const unit_id,
                        command::move const& mv ) {
    land_view_plane.EXPECT__animate_if_visible( _ );
    unique_ptr<CommandHandler> handler =
        handle_command( w.engine(), w.ss(), w.ts(), w.agent(),
                        player, unit_id, mv );
    wait<CommandHandlerRunResult> const w = handler->run();
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    CommandHandlerRunResult const expected_result{
      .order_was_run = true, .units_to_prioritize = {} };
    REQUIRE( *w == expected_result );
  };

  // Allow the free colonist to move along a road. This tests
  // that a unit is allowed to expend less than one movement
  // point to enter a colony in the relevant situation.
  w.square( { .x = 1, .y = 1 } ).road = true;
  w.square( { .x = 2, .y = 1 } ).road = true;
  // This will make sure that the movement points required to
  // enter a colony square are capped at 1.
  w.square( { .x = 1, .y = 1 } ).overlay = e_land_overlay::hills;

  // Sanity check.
  REQUIRE( w.units().coord_for( free_colonist.id() ) ==
           Coord{ .x = 2, .y = 1 } );
  REQUIRE( w.units().coord_for( wagon_train.id() ) ==
           Coord{ .x = 1, .y = 0 } );
  REQUIRE( w.units().coord_for( privateer.id() ) ==
           Coord{ .x = 0, .y = 0 } );

  SECTION( "no mod key" ) {
    move_unit( free_colonist.id(),
               { .d = e_direction::w, .mod_key_2 = false } );
    move_unit( wagon_train.id(),
               { .d = e_direction::s, .mod_key_2 = false } );
    move_unit( privateer.id(),
               { .d = e_direction::se, .mod_key_2 = false } );
  }

  SECTION( "with mod key" ) {
    move_unit( free_colonist.id(),
               { .d = e_direction::w, .mod_key_2 = true } );
    move_unit( wagon_train.id(),
               { .d = e_direction::s, .mod_key_2 = true } );
    w.colony_viewer()
        .EXPECT__show( _, colony.id )
        .returns( e_colony_abandoned::no );
    w.agent().EXPECT__human().returns( true );
    move_unit( privateer.id(),
               { .d = e_direction::se, .mod_key_2 = true } );
  }

  REQUIRE( w.units().coord_for( free_colonist.id() ) ==
           Coord{ .x = 1, .y = 1 } );
  REQUIRE( w.units().coord_for( wagon_train.id() ) ==
           Coord{ .x = 1, .y = 1 } );
  REQUIRE( w.units().coord_for( privateer.id() ) ==
           Coord{ .x = 1, .y = 1 } );
}

TEST_CASE(
    "[command-move] ship unloads units when moving into "
    "colony" ) {
  world W;
  MockLandViewPlane land_view_plane;
  W.planes().get().set_bottom<ILandViewPlane>( land_view_plane );
  Player& player = W.default_player();
  W.add_colony( { .x = 1, .y = 1 } );
  Unit const& galleon = W.add_unit_on_map( e_unit_type::galleon,
                                           { .x = 0, .y = 0 } );
  Unit const& missionary = W.add_unit_in_cargo(
      e_unit_type::missionary, galleon.id() );
  Unit const& free_colonist = W.add_unit_in_cargo(
      e_unit_type::free_colonist, galleon.id() );

  auto move_unit = [&]( UnitId unit_id, e_direction d ) {
    land_view_plane.EXPECT__animate_if_visible( _ );
    unique_ptr<CommandHandler> handler = handle_command(
        W.engine(), W.ss(), W.ts(), W.agent(), player, unit_id,
        command::move{ .d = d } );
    wait<CommandHandlerRunResult> const w = handler->run();
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    return *w;
  };

  // Sanity check.
  CommandHandlerRunResult const expected_res{
    .order_was_run       = true,
    .units_to_prioritize = { missionary.id(),
                             free_colonist.id() } };
  CommandHandlerRunResult const res =
      move_unit( galleon.id(), e_direction::se );
  REQUIRE( res == expected_res );

  REQUIRE( missionary.movement_points() == 2 );
  REQUIRE( free_colonist.movement_points() == 1 );
  REQUIRE( galleon.movement_points() == 0 );

  REQUIRE( W.units().coord_for( missionary.id() ) ==
           Coord{ .x = 1, .y = 1 } );
  REQUIRE( W.units().coord_for( free_colonist.id() ) ==
           Coord{ .x = 1, .y = 1 } );
  REQUIRE( W.units().coord_for( galleon.id() ) ==
           Coord{ .x = 1, .y = 1 } );
}

// Because the only ship that can carry a treasure is a galleon,
// that means when a ship unloads a treasure into a colony the
// player already has a galleon and thus the game will not offer
// to transport it, unless the player has Cortes. The exception
// is post-declaration; see a later test case for that.
TEST_CASE(
    "[command-move] galleon carrying treasure unloads into "
    "colony" ) {
  world w;
  w.settings().game_setup_options.difficulty =
      e_difficulty::conquistador;
  MockLandViewPlane land_view_plane;
  w.planes().get().set_bottom<ILandViewPlane>( land_view_plane );
  Player& player    = w.default_player();
  MockIAgent& agent = w.agent();
  w.add_colony( { .x = 1, .y = 1 } );
  Unit const& galleon = w.add_unit_on_map( e_unit_type::galleon,
                                           { .x = 0, .y = 0 } );
  UnitId const treasure_id =
      w.add_unit_in_cargo( e_unit_type::treasure, galleon.id() )
          .id();

  auto move_unit = [&]( UnitId unit_id, e_direction d ) {
    land_view_plane.EXPECT__animate_if_visible( _ );
    unique_ptr<CommandHandler> handler = handle_command(
        w.engine(), w.ss(), w.ts(), w.agent(), player, unit_id,
        command::move{ .d = d } );
    wait<CommandHandlerRunResult> const w = handler->run();
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    return *w;
  };

  SECTION( "no cortes" ) {
    CommandHandlerRunResult const expected_res{
      .order_was_run       = true,
      .units_to_prioritize = { treasure_id } };
    CommandHandlerRunResult const res =
        move_unit( galleon.id(), e_direction::se );
    REQUIRE( res == expected_res );

    REQUIRE( player.money == 0 );
    REQUIRE( w.units().exists( treasure_id ) );
    REQUIRE( galleon.movement_points() == 0 );
    REQUIRE( w.units().coord_for( galleon.id() ) ==
             Coord{ .x = 1, .y = 1 } );
  }

  SECTION( "with cortes" ) {
    player.fathers.has[e_founding_father::hernan_cortes] = true;
    w.old_world( player.type ).taxes.tax_rate            = 7;

    agent
        .EXPECT__should_king_transport_treasure(
            StrContains( "bounty" ) )
        .returns( ui::e_confirm::yes );
    agent.EXPECT__message_box(
        StrContains( "Treasure worth 1000" ) );
    agent.EXPECT__handle( TreasureArrived{} );
    CommandHandlerRunResult const expected_res{
      .order_was_run = true, .units_to_prioritize = {} };
    CommandHandlerRunResult const res =
        move_unit( galleon.id(), e_direction::se );
    REQUIRE( res == expected_res );

    // The king takes 7% of the 1000 treasure.
    REQUIRE( player.money == 1000 - 70 );
    REQUIRE( !w.units().exists( treasure_id ) );
  }
}

TEST_CASE(
    "[command-move] ship unloads treasure post declaration, "
    "treasure is transported, treasure is deleted" ) {
  world w;
  w.settings().game_setup_options.difficulty =
      e_difficulty::conquistador;
  MockLandViewPlane land_view_plane;
  w.planes().get().set_bottom<ILandViewPlane>( land_view_plane );
  Player& player           = w.default_player();
  MockIAgent& agent        = w.agent();
  player.revolution.status = e_revolution_status::declared;
  w.add_colony( { .x = 1, .y = 1 } );
  Unit const& galleon = w.add_unit_on_map( e_unit_type::galleon,
                                           { .x = 0, .y = 0 } );
  // This unit will be deleted.
  UnitId const treasure_id =
      w.add_unit_in_cargo( e_unit_type::treasure, galleon.id() )
          .id();

  auto move_unit = [&]( UnitId unit_id, e_direction d ) {
    land_view_plane.EXPECT__animate_if_visible( _ );
    unique_ptr<CommandHandler> handler = handle_command(
        w.engine(), w.ss(), w.ts(), w.agent(), player, unit_id,
        command::move{ .d = d } );
    wait<CommandHandlerRunResult> const w = handler->run();
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    return *w;
  };

  // Sanity check.
  agent.EXPECT__message_box(
      StrContains( "traveling merchants" ) );
  agent.EXPECT__handle( TreasureArrived{} );
  CommandHandlerRunResult const expected_res{
    .order_was_run = true, .units_to_prioritize = {} };
  CommandHandlerRunResult const res =
      move_unit( galleon.id(), e_direction::se );
  REQUIRE( res == expected_res );

  // We keep all 1000 of the treasure.
  REQUIRE( player.money == 1000 );
  REQUIRE( !w.units().exists( treasure_id ) );
  REQUIRE( galleon.movement_points() == 0 );
  REQUIRE( w.units().coord_for( galleon.id() ) ==
           Coord{ .x = 1, .y = 1 } );
}

TEST_CASE(
    "[command-move] treasure moves into colony, treasure is "
    "transported, treasure is deleted" ) {
  world W;
  MockLandViewPlane land_view_plane;
  W.planes().get().set_bottom<ILandViewPlane>( land_view_plane );
  Player& player    = W.default_player();
  MockIAgent& agent = W.agent();
  W.add_colony( { .x = 1, .y = 1 } );
  // This unit will be deleted.
  UnitId const treasure_id =
      W.add_unit_on_map( e_unit_type::treasure,
                         { .x = 0, .y = 1 } )
          .id();

  auto move_unit = [&]( UnitId unit_id, e_direction d ) {
    land_view_plane.EXPECT__animate_if_visible( _ );
    unique_ptr<CommandHandler> handler = handle_command(
        W.engine(), W.ss(), W.ts(), W.agent(), player, unit_id,
        command::move{ .d = d } );
    wait<CommandHandlerRunResult> const w = handler->run();
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    return *w;
  };

  agent
      .EXPECT__should_king_transport_treasure(
          StrContains( "bounty" ) )
      .returns( ui::e_confirm::yes );
  agent.EXPECT__message_box(
      StrContains( "Treasure worth 1000" ) );
  agent.EXPECT__handle( TreasureArrived{} );
  CommandHandlerRunResult const expected_res{
    .order_was_run = true, .units_to_prioritize = {} };
  CommandHandlerRunResult const res =
      move_unit( treasure_id, e_direction::e );
  REQUIRE( res == expected_res );

  // The king takes 50% of the 1000 treasure.
  REQUIRE( player.money == 500 );
  REQUIRE( !W.units().exists( treasure_id ) );
}

TEST_CASE(
    "[command-move] unit on ship attempting to attack brave" ) {
  world W;
  Unit& caravel  = W.add_unit_on_map( e_unit_type::caravel,
                                      { .x = 0, .y = 0 } );
  Unit& colonist = W.add_unit_in_cargo(
      e_unit_type::free_colonist, caravel.id() );
  Dwelling& dwelling =
      W.add_dwelling( { .x = 1, .y = 0 }, e_tribe::cherokee );
  W.add_native_unit_on_map( e_native_unit_type::brave,
                            { .x = 1, .y = 1 }, dwelling.id );

  unique_ptr<CommandHandler> handler = handle_command(
      W.engine(), W.ss(), W.ts(), W.agent( e_player::french ),
      W.french(), colonist.id(),
      command::move{ .d = e_direction::se } );
  W.agent( W.default_player_type() )
      .EXPECT__message_box(
          "We cannot attack a land unit from a ship." );
  wait<bool> w_confirm = handler->confirm();
  REQUIRE( !w_confirm.exception() );
  REQUIRE( w_confirm.ready() );
  REQUIRE_FALSE( *w_confirm );
}

TEST_CASE(
    "[command-move] unit attempting to board/attack foreign "
    "ship" ) {
  world W;
  Unit const& caravel =
      W.add_unit_on_map( e_unit_type::caravel,
                         { .x = 0, .y = 0 }, e_player::dutch );

  Unit const& colonist =
      W.add_unit_on_map( e_unit_type::free_colonist,
                         { .x = 1, .y = 1 }, e_player::french );

  BASE_CHECK( caravel.player_type() != colonist.player_type() );
  unique_ptr<CommandHandler> handler = handle_command(
      W.engine(), W.ss(), W.ts(), W.agent( e_player::french ),
      W.french(), colonist.id(),
      command::move{ .d = e_direction::nw } );
  W.agent( e_player::french )
      .EXPECT__message_box(
          "Our land units can neither attack nor board foreign "
          "ships." );
  bool const confirmed = co_await_test( handler->confirm() );
  REQUIRE_FALSE( confirmed );
}

// This can't really happen in the normal game since we never
// allow ships to linger on land, but we should still make sure
// that we don't crash because this situation could be created in
// cheat mode.
TEST_CASE(
    "[command-move] land unit attempting to attack ship on "
    "land" ) {
  world W;
  Unit const& caravel =
      W.add_unit_on_map( e_unit_type::caravel,
                         { .x = 1, .y = 0 }, e_player::dutch );

  Unit const& soldier =
      W.add_unit_on_map( e_unit_type::free_colonist,
                         { .x = 1, .y = 1 }, e_player::french );

  BASE_CHECK( caravel.player_type() != soldier.player_type() );
  unique_ptr<CommandHandler> handler = handle_command(
      W.engine(), W.ss(), W.ts(), W.agent( e_player::french ),
      W.french(), soldier.id(),
      command::move{ .d = e_direction::n } );
  W.agent( e_player::french )
      .EXPECT__message_box(
          "Our land units can neither attack nor board foreign "
          "ships." );
  bool const confirmed = co_await_test( handler->confirm() );
  REQUIRE_FALSE( confirmed );
}

TEST_CASE(
    "[command-move] units on ships in colony offboarded to help "
    "defend" ) {
  world W;
  MockLandViewPlane mock_land_view;
  W.planes().get().set_bottom<ILandViewPlane>( mock_land_view );
  Colony const& colony =
      W.add_colony( { .x = 1, .y = 0 }, e_player::dutch );
  Unit const& master_distiller =
      W.add_unit_indoors( colony.id, e_indoor_job::bells,
                          e_unit_type::master_distiller );
  Unit const& caravel =
      W.add_unit_on_map( e_unit_type::caravel,
                         { .x = 1, .y = 0 }, e_player::dutch );
  Unit const& soldier =
      W.add_unit_on_map( e_unit_type::soldier,
                         { .x = 1, .y = 1 }, e_player::french );
  BASE_CHECK( colony.player != soldier.player_type() );

  auto require_on_map = [&]( Unit const& unit ) {
    REQUIRE( as_const( W.units() )
                 .ownership_of( unit.id() )
                 .holds<UnitOwnership::world>() );
  };

  auto require_in_cargo = [&]( Unit const& unit ) {
    REQUIRE( as_const( W.units() )
                 .ownership_of( unit.id() )
                 .holds<UnitOwnership::cargo>() );
  };

  auto require_in_colony = [&]( Unit const& unit ) {
    REQUIRE( as_const( W.units() )
                 .ownership_of( unit.id() )
                 .holds<UnitOwnership::colony>() );
  };

  auto require_sentried = [&]( Unit const& unit ) {
    REQUIRE( unit.orders().holds<unit_orders::sentry>() );
  };

  auto require_not_sentried = [&]( Unit const& unit ) {
    REQUIRE( !unit.orders().holds<unit_orders::sentry>() );
  };

  SECTION( "no units on ship" ) {
    // Select which colony worker to be the defender. In this
    // case there is only one. This is done because there are not
    // military units.
    W.rand().EXPECT__between_ints( 0, 0 ).returns( 0 );
    unique_ptr<CommandHandler> handler = handle_command(
        W.engine(), W.ss(), W.ts(), W.agent( e_player::french ),
        W.french(), soldier.id(),
        command::move{ .d = e_direction::n } );
    require_in_colony( master_distiller );
    require_not_sentried( master_distiller );
    require_on_map( caravel );
    bool const confirmed = co_await_test( handler->confirm() );
    REQUIRE( confirmed );
    // We don't really care what this is, we just need to give it
    // some plausible values so that we can test that the combat
    // creation function is called with the correct defender.
    CombatEuroAttackUndefendedColony const combat{
      .winner    = e_combat_winner::defender,
      .colony_id = colony.id,
      .attacker  = { .id = soldier.id(),
                     .outcome =
                         EuroUnitCombatOutcome::no_change{} },
      .defender  = {
         .id = master_distiller.id(),
         .outcome =
            EuroColonyWorkerCombatOutcome::no_change{} } };
    W.combat()
        .EXPECT__euro_attack_undefended_colony(
            soldier, master_distiller, colony )
        .returns( combat );
    mock_land_view.EXPECT__animate_if_visible( _ );
    W.agent( e_player::dutch )
        .EXPECT__message_box(
            "[Dutch] Master Distiller defeats [French] in 1!" );
    co_await_test( handler->perform() );
  }

  SECTION( "unit on ship" ) {
    Unit const& soldier_onboard = W.add_unit_in_cargo(
        e_unit_type::soldier, caravel.id() );
    require_in_cargo( soldier_onboard );
    unique_ptr<CommandHandler> handler = handle_command(
        W.engine(), W.ss(), W.ts(), W.agent( e_player::french ),
        W.french(), soldier.id(),
        command::move{ .d = e_direction::n } );
    require_in_colony( master_distiller );
    require_not_sentried( master_distiller );
    require_on_map( soldier_onboard );
    require_sentried( soldier_onboard );
    require_on_map( caravel );
    bool const confirmed = co_await_test( handler->confirm() );
    REQUIRE( confirmed );
    // We don't really care what this is, we just need to give it
    // some plausible values so that we can test that the combat
    // creation function is called with the correct defender.
    CombatEuroAttackEuro const combat{
      .winner   = e_combat_winner::defender,
      .attacker = { .id = soldier.id(),
                    .outcome =
                        EuroUnitCombatOutcome::no_change{} },
      .defender = {
        .id      = soldier_onboard.id(),
        .outcome = EuroUnitCombatOutcome::no_change{} } };
    // In this scenario, since the soldier should have been auto-
    // matically offboarded from the ship, it should have been
    // chosen as the defender. So that means it is just a regular
    // land battle with the soldier as the defender. Although we
    // specified the soldier as the defender above, the real test
    // is that it gets passed in as a parameter to the function
    // below; that's what indicates that it was chosen.
    W.combat()
        .EXPECT__euro_attack_euro( soldier, soldier_onboard )
        .returns( combat );
    mock_land_view.EXPECT__animate_if_visible( _ );
    W.agent( e_player::dutch )
        .EXPECT__message_box(
            "[Dutch] Soldier defeats [French] in 1!" );
    co_await_test( handler->perform() );
  }
}

TEST_CASE(
    "[command-move] sailing the high seas after declaring" ) {
  world w;
  MockLandViewPlane mock_land_view;
  MockIAgent& agent = w.agent();
  w.planes().get().set_bottom<ILandViewPlane>( mock_land_view );
  Player& player      = w.default_player();
  Unit const& caravel = w.add_unit_on_map(
      e_unit_type::caravel, { .x = 6, .y = 1 }, player.type );

  // Make sure we're testing what we think we're testing.
  BASE_CHECK( w.units().coord_for( caravel.id() ).x ==
              w.terrain().world_size_tiles().w - 3 );

  auto const sail_high_seas = Field(
      &ChoiceConfig::msg, StrContains( "sail the high seas?" ) );
  auto const not_permitted =
      StrContains( "no longer permitted" );

  SECTION( "before declaration" ) {
    player.revolution.status = e_revolution_status::not_declared;

    {
      unique_ptr<CommandHandler> const handler = handle_command(
          w.engine(), w.ss(), w.ts(), w.agent(), player,
          caravel.id(), command::move{ .d = e_direction::e } );
      agent.EXPECT__should_sail_high_seas( caravel.id() )
          .returns( ui::e_confirm::no );
      bool const confirmed = co_await_test( handler->confirm() );
      REQUIRE( confirmed );
      mock_land_view.EXPECT__animate_if_visible( _ );
      co_await_test( handler->perform() );
      REQUIRE( w.units().coord_for( caravel.id() ).to_gfx() ==
               point{ .x = 7, .y = 1 } );
    }

    {
      unique_ptr<CommandHandler> const handler = handle_command(
          w.engine(), w.ss(), w.ts(), w.agent(), player,
          caravel.id(), command::move{ .d = e_direction::e } );
      agent.EXPECT__should_sail_high_seas( caravel.id() )
          .returns( ui::e_confirm::no );
      bool const confirmed = co_await_test( handler->confirm() );
      REQUIRE( confirmed );
      mock_land_view.EXPECT__animate_if_visible( _ );
      co_await_test( handler->perform() );
      REQUIRE( w.units().coord_for( caravel.id() ).to_gfx() ==
               point{ .x = 8, .y = 1 } );
      REQUIRE_FALSE( w.events()
                         .one_time_help
                         .showed_no_sail_high_seas_during_war );
    }

    // Map edge.
    {
      unique_ptr<CommandHandler> const handler = handle_command(
          w.engine(), w.ss(), w.ts(), w.agent(), player,
          caravel.id(), command::move{ .d = e_direction::e } );
      agent.EXPECT__should_sail_high_seas( caravel.id() )
          .returns( ui::e_confirm::no );
      bool const confirmed = co_await_test( handler->confirm() );
      REQUIRE_FALSE( confirmed );
      REQUIRE( w.units().coord_for( caravel.id() ).to_gfx() ==
               point{ .x = 8, .y = 1 } );
      REQUIRE_FALSE( w.events()
                         .one_time_help
                         .showed_no_sail_high_seas_during_war );
    }

    // Map edge.
    {
      unique_ptr<CommandHandler> const handler = handle_command(
          w.engine(), w.ss(), w.ts(), w.agent(), player,
          caravel.id(), command::move{ .d = e_direction::e } );
      agent.EXPECT__should_sail_high_seas( caravel.id() )
          .returns( ui::e_confirm::yes );
      bool const confirmed = co_await_test( handler->confirm() );
      REQUIRE( confirmed );
      mock_land_view.EXPECT__animate_if_visible( _ );
      co_await_test( handler->perform() );
      REQUIRE( is_unit_inbound( w.units(), caravel.id() ) );
      REQUIRE_FALSE( w.events()
                         .one_time_help
                         .showed_no_sail_high_seas_during_war );
    }
  }

  SECTION( "after declaration" ) {
    player.revolution.status = e_revolution_status::declared;
    REQUIRE_FALSE(
        w.events()
            .one_time_help.showed_no_sail_high_seas_during_war );

    {
      BASE_CHECK(
          w.player( caravel.player_type() ).revolution.status ==
          e_revolution_status::declared );
      unique_ptr<CommandHandler> const handler = handle_command(
          w.engine(), w.ss(), w.ts(), w.agent(), player,
          caravel.id(), command::move{ .d = e_direction::e } );
      w.agent().EXPECT__message_box( not_permitted );
      bool const confirmed = co_await_test( handler->confirm() );
      REQUIRE( confirmed );
      mock_land_view.EXPECT__animate_if_visible( _ );
      co_await_test( handler->perform() );
      REQUIRE( w.units().coord_for( caravel.id() ).to_gfx() ==
               point{ .x = 7, .y = 1 } );
      REQUIRE( w.events()
                   .one_time_help
                   .showed_no_sail_high_seas_during_war );
    }

    {
      unique_ptr<CommandHandler> const handler = handle_command(
          w.engine(), w.ss(), w.ts(), w.agent(), player,
          caravel.id(), command::move{ .d = e_direction::e } );
      // NOTE: no msg box here.
      bool const confirmed = co_await_test( handler->confirm() );
      REQUIRE( confirmed );
      mock_land_view.EXPECT__animate_if_visible( _ );
      co_await_test( handler->perform() );
      REQUIRE( w.units().coord_for( caravel.id() ).to_gfx() ==
               point{ .x = 8, .y = 1 } );
      REQUIRE( w.events()
                   .one_time_help
                   .showed_no_sail_high_seas_during_war );
    }

    {
      unique_ptr<CommandHandler> const handler = handle_command(
          w.engine(), w.ss(), w.ts(), w.agent(), player,
          caravel.id(), command::move{ .d = e_direction::e } );
      w.agent().EXPECT__message_box( not_permitted );
      bool const confirmed = co_await_test( handler->confirm() );
      REQUIRE_FALSE( confirmed );
      REQUIRE( w.units().coord_for( caravel.id() ).to_gfx() ==
               point{ .x = 8, .y = 1 } );
      REQUIRE( w.events()
                   .one_time_help
                   .showed_no_sail_high_seas_during_war );
    }
  }
}

TEST_CASE( "[command-move] goto: high seas via sea lane" ) {
  world w;
  MockLandViewPlane mock_land_view;
  MockIAgent& agent = w.agent();
  w.planes().get().set_bottom<ILandViewPlane>( mock_land_view );
  Player& player = w.default_player();

  Unit& caravel = w.add_unit_on_map(
      e_unit_type::caravel, { .x = 6, .y = 1 }, player.type );

  // Make sure we're testing what we think we're testing.
  BASE_CHECK( w.units().coord_for( caravel.id() ).x ==
              w.terrain().world_size_tiles().w - 3 );

  SECTION( "Sanity check: No goto, confirmation no" ) {
    auto const handler = handle_command(
        w.engine(), w.ss(), w.ts(), w.agent(), player,
        caravel.id(), command::move{ .d = e_direction::e } );
    agent.EXPECT__should_sail_high_seas( caravel.id() )
        .returns( ui::e_confirm::no );
    bool const confirmed = co_await_test( handler->confirm() );
    REQUIRE( confirmed );
    mock_land_view.EXPECT__animate_if_visible( _ );
    co_await_test( handler->perform() );
    REQUIRE( w.units().coord_for( caravel.id() ).to_gfx() ==
             point{ .x = 7, .y = 1 } );
    REQUIRE( caravel.orders().holds<unit_orders::none>() );
  }

  SECTION( "Sanity check: No goto, confirmation yes" ) {
    auto const handler = handle_command(
        w.engine(), w.ss(), w.ts(), w.agent(), player,
        caravel.id(), command::move{ .d = e_direction::e } );
    agent.EXPECT__should_sail_high_seas( caravel.id() )
        .returns( ui::e_confirm::yes );
    bool const confirmed = co_await_test( handler->confirm() );
    REQUIRE( confirmed );
    mock_land_view.EXPECT__animate_if_visible( _ );
    co_await_test( handler->perform() );
    REQUIRE( is_unit_inbound( w.units(), caravel.id() ) );
    REQUIRE( caravel.orders().holds<unit_orders::none>() );
  }

  SECTION( "goto map, dst tile not target tile" ) {
    caravel.orders() = unit_orders::go_to{
      .target = goto_target::map{
        .tile     = { .x = 8, .y = 1 },
        .snapshot = GotoTargetSnapshot::
            empty_or_friendly_with_sea_lane{} } };
    auto const handler = handle_command(
        w.engine(), w.ss(), w.ts(), w.agent(), player,
        caravel.id(), command::move{ .d = e_direction::e } );
    bool const confirmed = co_await_test( handler->confirm() );
    REQUIRE( confirmed );
    mock_land_view.EXPECT__animate_if_visible( _ );
    co_await_test( handler->perform() );
    REQUIRE( w.units().coord_for( caravel.id() ).to_gfx() ==
             point{ .x = 7, .y = 1 } );
    REQUIRE( caravel.orders() ==
             unit_orders::go_to{
               .target = goto_target::map{
                 .tile     = { .x = 8, .y = 1 },
                 .snapshot = GotoTargetSnapshot::
                     empty_or_friendly_with_sea_lane{} } } );
  }

  SECTION( "goto map, dst tile is target tile" ) {
    caravel.orders() = unit_orders::go_to{
      .target = goto_target::map{
        .tile     = { .x = 7, .y = 1 },
        .snapshot = GotoTargetSnapshot::
            empty_or_friendly_with_sea_lane{} } };
    auto const handler = handle_command(
        w.engine(), w.ss(), w.ts(), w.agent(), player,
        caravel.id(), command::move{ .d = e_direction::e } );
    bool const confirmed = co_await_test( handler->confirm() );
    REQUIRE( confirmed );
    mock_land_view.EXPECT__animate_if_visible( _ );
    co_await_test( handler->perform() );
    REQUIRE( is_unit_inbound( w.units(), caravel.id() ) );
    REQUIRE( caravel.orders().holds<unit_orders::none>() );
  }

  SECTION(
      "goto map, dst tile is target tile but snapshot was "
      "hidden" ) {
    caravel.orders() = unit_orders::go_to{
      .target = goto_target::map{ .tile     = { .x = 7, .y = 1 },
                                  .snapshot = nothing } };
    auto const handler = handle_command(
        w.engine(), w.ss(), w.ts(), w.agent(), player,
        caravel.id(), command::move{ .d = e_direction::e } );
    bool const confirmed = co_await_test( handler->confirm() );
    REQUIRE( confirmed );
    mock_land_view.EXPECT__animate_if_visible( _ );
    co_await_test( handler->perform() );
    REQUIRE_FALSE( is_unit_inbound( w.units(), caravel.id() ) );
    REQUIRE( caravel.orders() ==
             unit_orders::go_to{ .target = goto_target::map{
                                   .tile = { .x = 7, .y = 1 },
                                   .snapshot = nothing } } );
  }

  SECTION( "goto map, harbor" ) {
    caravel.orders() =
        unit_orders::go_to{ .target = goto_target::harbor{} };
    auto const handler = handle_command(
        w.engine(), w.ss(), w.ts(), w.agent(), player,
        caravel.id(), command::move{ .d = e_direction::e } );
    bool const confirmed = co_await_test( handler->confirm() );
    REQUIRE( confirmed );
    mock_land_view.EXPECT__animate_if_visible( _ );
    co_await_test( handler->perform() );
    REQUIRE( is_unit_inbound( w.units(), caravel.id() ) );
    REQUIRE( caravel.orders().holds<unit_orders::none>() );
  }
}

TEST_CASE(
    "[command-move] trade route: high seas via sea lane" ) {
  world w;
  MockLandViewPlane mock_land_view;
  w.planes().get().set_bottom<ILandViewPlane>( mock_land_view );
  Player& player = w.default_player();

  Unit& caravel = w.add_unit_on_map(
      e_unit_type::caravel, { .x = 6, .y = 1 }, player.type );

  // Make sure we're testing what we think we're testing.
  BASE_CHECK( w.units().coord_for( caravel.id() ).x ==
              w.terrain().world_size_tiles().w - 3 );

  w.trade_routes().routes[1] = TradeRoute{
    .id    = 1,
    .stops = {
      TradeRouteStop{ .target = TradeRouteTarget::harbor{} } } };
  caravel.orders() =
      unit_orders::trade_route{ .id = 1, .en_route_to_stop = 0 };
  auto const handler = handle_command(
      w.engine(), w.ss(), w.ts(), w.agent(), player,
      caravel.id(), command::move{ .d = e_direction::e } );
  bool const confirmed = co_await_test( handler->confirm() );
  REQUIRE( confirmed );
  mock_land_view.EXPECT__animate_if_visible( _ );
  co_await_test( handler->perform() );
  REQUIRE( is_unit_inbound( w.units(), caravel.id() ) );
  REQUIRE( caravel.orders().holds<unit_orders::trade_route>() );
}

TEST_CASE( "[command-move] goto: high seas via map edge" ) {
  world w;
  MockLandViewPlane mock_land_view;
  w.planes().get().set_bottom<ILandViewPlane>( mock_land_view );
  Player& player = w.default_player();

  Unit& caravel = w.add_unit_on_map(
      e_unit_type::caravel, { .x = 8, .y = 1 }, player.type );

  // Make sure we're testing what we think we're testing.
  BASE_CHECK( w.units().coord_for( caravel.id() ).x ==
              w.terrain().world_size_tiles().w - 1 );

  caravel.orders() =
      unit_orders::go_to{ .target = goto_target::harbor{} };
  auto const handler = handle_command(
      w.engine(), w.ss(), w.ts(), w.agent(), player,
      caravel.id(), command::move{ .d = e_direction::e } );
  bool const confirmed = co_await_test( handler->confirm() );
  REQUIRE( confirmed );
  mock_land_view.EXPECT__animate_if_visible( _ );
  co_await_test( handler->perform() );
  REQUIRE( is_unit_inbound( w.units(), caravel.id() ) );
  REQUIRE( caravel.orders().holds<unit_orders::none>() );
}

TEST_CASE(
    "[command-move] trade route: high seas via map edge" ) {
  world w;
  MockLandViewPlane mock_land_view;
  w.planes().get().set_bottom<ILandViewPlane>( mock_land_view );
  Player& player = w.default_player();

  Unit& caravel = w.add_unit_on_map(
      e_unit_type::caravel, { .x = 8, .y = 1 }, player.type );

  // Make sure we're testing what we think we're testing.
  BASE_CHECK( w.units().coord_for( caravel.id() ).x ==
              w.terrain().world_size_tiles().w - 1 );

  w.trade_routes().routes[1] = TradeRoute{
    .id    = 1,
    .stops = {
      TradeRouteStop{ .target = TradeRouteTarget::harbor{} } } };
  caravel.orders() =
      unit_orders::trade_route{ .id = 1, .en_route_to_stop = 0 };
  auto const handler = handle_command(
      w.engine(), w.ss(), w.ts(), w.agent(), player,
      caravel.id(), command::move{ .d = e_direction::e } );
  bool const confirmed = co_await_test( handler->confirm() );
  REQUIRE( confirmed );
  mock_land_view.EXPECT__animate_if_visible( _ );
  co_await_test( handler->perform() );
  REQUIRE( is_unit_inbound( w.units(), caravel.id() ) );
  REQUIRE( caravel.orders().holds<unit_orders::trade_route>() );
}

TEST_CASE(
    "[command-move] goto: ship in colony port clears orders" ) {
  world w;
  MockLandViewPlane mock_land_view;
  w.planes().get().set_bottom<ILandViewPlane>( mock_land_view );
  Player& player = w.default_player();

  w.add_colony( { .x = 1, .y = 0 } );
  Unit& caravel = w.add_unit_on_map(
      e_unit_type::caravel, { .x = 0, .y = 0 }, player.type );
  caravel.orders() = unit_orders::go_to{
    .target = goto_target::map{
      .tile     = { .x = 1, .y = 0 },
      .snapshot = GotoTargetSnapshot::empty_or_friendly{} } };

  auto const handler = handle_command(
      w.engine(), w.ss(), w.ts(), w.agent(), player,
      caravel.id(), command::move{ .d = e_direction::e } );
  bool const confirmed = co_await_test( handler->confirm() );
  REQUIRE( confirmed );
  mock_land_view.EXPECT__animate_if_visible( _ );
  co_await_test( handler->perform() );
  REQUIRE( w.units().coord_for( caravel.id() ).to_gfx() ==
           point{ .x = 1, .y = 0 } );
  REQUIRE( caravel.orders().holds<unit_orders::none>() );
}

TEST_CASE(
    "[command-move] trade route: ship in colony port does NOT "
    "clear orders" ) {
  world w;
  MockLandViewPlane mock_land_view;
  w.planes().get().set_bottom<ILandViewPlane>( mock_land_view );
  Player& player = w.default_player();

  w.add_colony( { .x = 1, .y = 0 } );
  Unit& caravel = w.add_unit_on_map(
      e_unit_type::caravel, { .x = 0, .y = 0 }, player.type );
  w.trade_routes().routes[1] = TradeRoute{
    .id    = 1,
    .stops = { TradeRouteStop{
      .target = TradeRouteTarget::colony{ .colony_id = 1 } } } };
  caravel.orders() =
      unit_orders::trade_route{ .id = 1, .en_route_to_stop = 0 };

  auto const handler = handle_command(
      w.engine(), w.ss(), w.ts(), w.agent(), player,
      caravel.id(), command::move{ .d = e_direction::e } );
  bool const confirmed = co_await_test( handler->confirm() );
  REQUIRE( confirmed );
  mock_land_view.EXPECT__animate_if_visible( _ );
  co_await_test( handler->perform() );
  REQUIRE( w.units().coord_for( caravel.id() ).to_gfx() ==
           point{ .x = 1, .y = 0 } );
  REQUIRE( caravel.orders().holds<unit_orders::trade_route>() );
}

TEST_CASE( "[command-move] move off top edge of map" ) {
  world w;
  Player& player = w.default_player();

  Unit& caravel = w.add_unit_on_map(
      e_unit_type::caravel, { .x = 0, .y = 0 }, player.type );

  auto const sail_high_seas = Field(
      &ChoiceConfig::msg, StrContains( "sail the high seas?" ) );

  auto const handler = handle_command(
      w.engine(), w.ss(), w.ts(), w.agent(), player,
      caravel.id(), command::move{ .d = e_direction::n } );
  bool const confirmed = co_await_test( handler->confirm() );
  REQUIRE_FALSE( confirmed );
  REQUIRE( w.units().coord_for( caravel.id() ).to_gfx() ==
           point{ .x = 0, .y = 0 } );
  REQUIRE( caravel.orders().holds<unit_orders::none>() );
}

} // namespace
} // namespace rn
