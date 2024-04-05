/****************************************************************
**native-turn.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-25.
*
* Description: Unit tests for the src/native-turn.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/native-turn.hpp"

// Testing
#include "test/fake/world.hpp"
#include "test/mocking.hpp"
#include "test/mocks/ieuro-mind.hpp"
#include "test/mocks/inative-mind.hpp"
#include "test/mocks/irand.hpp"
#include "test/mocks/land-view-plane.hpp"
#include "test/util/coro.hpp"

// Revolution Now
#include "src/iraid.rds.hpp"
#include "src/itribe-evolve.rds.hpp"
#include "src/plane-stack.hpp"
#include "src/ts.hpp"

// ss
#include "src/ss/dwelling.rds.hpp"
#include "src/ss/fog-square.rds.hpp"
#include "src/ss/ref.hpp"
#include "src/ss/settings.rds.hpp"
#include "src/ss/tribe.rds.hpp"
#include "src/ss/units.hpp"
#include "src/ss/woodcut.rds.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp"

RDS_DEFINE_MOCK( IRaid );
RDS_DEFINE_MOCK( ITribeEvolve );

namespace rn {
namespace {

using namespace std;

using ::mock::matchers::_;
using ::mock::matchers::Approx;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() {
    add_player( e_nation::spanish );
    add_player( e_nation::english );
    set_default_player( e_nation::spanish );
    // Need at least one human player otherwise the map will be
    // fully visible during the natives' turns and we won't be
    // able to test whether animations get suppressed or not due
    // to map visibility.
    set_default_player_as_human();
    create_default_map();
  }

  void create_default_map() {
    MapSquare const   L = make_grassland();
    vector<MapSquare> tiles{
        L, L, L, L, L, L, //
        L, L, L, L, L, L, //
        L, L, L, L, L, L, //
        L, L, L, L, L, L, //
        L, L, L, L, L, L, //
        L, L, L, L, L, L, //
    };
    build_map( std::move( tiles ), 6 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[native-turn] unit iteration, travel" ) {
  World           W;
  RealRaid        real_raid( W.ss(), W.ts() );
  RealTribeEvolve real_tribe_evolver( W.ss(), W.ts() );

  auto f = [&] {
    // In this one we don't inject dependencies because it seems
    // simple enough to test thoroughly.
    co_await_test( natives_turn( W.ss(), W.ts(), real_raid,
                                 real_tribe_evolver ) );
  };

  MockINativeMind& native_mind =
      W.native_mind( e_tribe::arawak );

  SECTION( "no units" ) { f(); }

  SECTION( "one unit, forfeight" ) {
    auto [dwelling_id, unit_id] = W.add_dwelling_and_brave_ids(
        { .x = 0, .y = 0 }, e_tribe::arawak );
    REQUIRE( W.units().unit_for( unit_id ).movement_points ==
             1 );
    native_mind.EXPECT__select_unit( set{ unit_id } )
        .returns( unit_id );
    native_mind.EXPECT__command_for( unit_id ).returns(
        NativeUnitCommand::forfeight{} );
    f();
    REQUIRE( W.units().unit_for( unit_id ).movement_points ==
             0 );
    REQUIRE( W.units().coord_for( unit_id ) ==
             Coord{ .x = 0, .y = 0 } );
  }

  SECTION( "one unit, travel" ) {
    auto [dwelling_id, unit_id] = W.add_dwelling_and_brave_ids(
        { .x = 0, .y = 0 }, e_tribe::arawak );
    REQUIRE( W.units().unit_for( unit_id ).movement_points ==
             1 );
    native_mind.EXPECT__select_unit( set{ unit_id } )
        .returns( unit_id );
    native_mind.EXPECT__command_for( unit_id ).returns(
        NativeUnitCommand::move{ .direction = e_direction::e } );
    f();
    REQUIRE( W.units().unit_for( unit_id ).movement_points ==
             0 );
    REQUIRE( W.units().coord_for( unit_id ) ==
             Coord{ .x = 1, .y = 0 } );
  }

  SECTION( "one unit, travel along road" ) {
    W.square( { .x = 0, .y = 0 } ).road = true;
    W.square( { .x = 1, .y = 0 } ).road = true;
    W.square( { .x = 2, .y = 0 } ).road = true;
    W.square( { .x = 3, .y = 0 } ).road = true;
    auto [dwelling_id, unit_id] = W.add_dwelling_and_brave_ids(
        { .x = 0, .y = 0 }, e_tribe::arawak );
    REQUIRE( W.units().unit_for( unit_id ).movement_points ==
             1 );
    // First move.
    native_mind.EXPECT__select_unit( set{ unit_id } )
        .returns( unit_id );
    native_mind.EXPECT__command_for( unit_id ).returns(
        NativeUnitCommand::move{ .direction = e_direction::e } );
    // Second move.
    native_mind.EXPECT__select_unit( set{ unit_id } )
        .returns( unit_id );
    native_mind.EXPECT__command_for( unit_id ).returns(
        NativeUnitCommand::move{ .direction = e_direction::e } );
    // Third move.
    native_mind.EXPECT__select_unit( set{ unit_id } )
        .returns( unit_id );
    native_mind.EXPECT__command_for( unit_id ).returns(
        NativeUnitCommand::move{ .direction = e_direction::e } );
    f();
    REQUIRE( W.units().unit_for( unit_id ).movement_points ==
             0 );
    REQUIRE( W.units().coord_for( unit_id ) ==
             Coord{ .x = 3, .y = 0 } );
  }

  SECTION( "one unit, travel and then forfeight" ) {
    W.square( { .x = 0, .y = 0 } ).road = true;
    W.square( { .x = 1, .y = 0 } ).road = true;
    W.square( { .x = 2, .y = 0 } ).road = true;
    W.square( { .x = 3, .y = 0 } ).road = true;
    auto [dwelling_id, unit_id] = W.add_dwelling_and_brave_ids(
        { .x = 0, .y = 0 }, e_tribe::arawak );
    REQUIRE( W.units().unit_for( unit_id ).movement_points ==
             1 );
    // First move.
    native_mind.EXPECT__select_unit( set{ unit_id } )
        .returns( unit_id );
    native_mind.EXPECT__command_for( unit_id ).returns(
        NativeUnitCommand::move{ .direction = e_direction::e } );
    // Second move.
    native_mind.EXPECT__select_unit( set{ unit_id } )
        .returns( unit_id );
    native_mind.EXPECT__command_for( unit_id ).returns(
        NativeUnitCommand::move{ .direction = e_direction::e } );
    // Third move.
    native_mind.EXPECT__select_unit( set{ unit_id } )
        .returns( unit_id );
    native_mind.EXPECT__command_for( unit_id ).returns(
        NativeUnitCommand::forfeight{} );
    f();
    REQUIRE( W.units().unit_for( unit_id ).movement_points ==
             0 );
    REQUIRE( W.units().coord_for( unit_id ) ==
             Coord{ .x = 2, .y = 0 } );
  }

  SECTION( "one unit, travel along partial road" ) {
    W.square( { .x = 0, .y = 0 } ).road = true;
    W.square( { .x = 1, .y = 0 } ).road = true;
    W.square( { .x = 2, .y = 0 } ).road = true;
    auto [dwelling_id, unit_id] = W.add_dwelling_and_brave_ids(
        { .x = 0, .y = 0 }, e_tribe::arawak );
    REQUIRE( W.units().unit_for( unit_id ).movement_points ==
             1 );
    // First move.
    native_mind.EXPECT__select_unit( set{ unit_id } )
        .returns( unit_id );
    native_mind.EXPECT__command_for( unit_id ).returns(
        NativeUnitCommand::move{ .direction = e_direction::e } );
    // Second move.
    native_mind.EXPECT__select_unit( set{ unit_id } )
        .returns( unit_id );
    native_mind.EXPECT__command_for( unit_id ).returns(
        NativeUnitCommand::move{ .direction = e_direction::e } );
    // Third move.
    native_mind.EXPECT__select_unit( set{ unit_id } )
        .returns( unit_id );
    native_mind.EXPECT__command_for( unit_id ).returns(
        NativeUnitCommand::move{ .direction = e_direction::e } );

    SECTION( "final move allowed" ) {
      W.rand()
          .EXPECT__bernoulli( Approx( .333333, .000001 ) )
          .returns( true );
      f();
      REQUIRE( W.units().coord_for( unit_id ) ==
               Coord{ .x = 3, .y = 0 } );
    }

    SECTION( "final move not allowed" ) {
      W.rand()
          .EXPECT__bernoulli( Approx( .333333, .000001 ) )
          .returns( false );
      f();
      REQUIRE( W.units().coord_for( unit_id ) ==
               Coord{ .x = 2, .y = 0 } );
    }

    REQUIRE( W.units().unit_for( unit_id ).movement_points ==
             0 );
  }

  SECTION( "one unit, animation enabled/disabled" ) {
    MockLandViewPlane mock_land_view;
    W.planes().back().land_view = &mock_land_view;
    auto [dwelling_id, unit_id] = W.add_dwelling_and_brave_ids(
        { .x = 0, .y = 0 }, e_tribe::arawak );
    REQUIRE( W.units().unit_for( unit_id ).movement_points ==
             1 );
    native_mind.EXPECT__select_unit( set{ unit_id } )
        .returns( unit_id );
    native_mind.EXPECT__command_for( unit_id ).returns(
        NativeUnitCommand::move{ .direction = e_direction::e } );

    bool& show_indian_moves =
        W.settings()
            .game_options
            .flags[e_game_flag_option::show_indian_moves];

    SECTION( "show_indian_moves=false" ) {
      show_indian_moves = false;

      SECTION( "src fog, dst fog" ) { f(); }

      SECTION( "src no fog, dst fog" ) {
        W.player_square( { .x = 0, .y = 0 } ) =
            PlayerSquare::explored{ .fog_status =
                                        FogStatus::clear{} };
        f();
      }

      SECTION( "src fog, dst no fog" ) {
        W.player_square( { .x = 1, .y = 0 } ) =
            PlayerSquare::explored{ .fog_status =
                                        FogStatus::clear{} };
        f();
      }
    }

    SECTION( "show_indian_moves=true" ) {
      show_indian_moves = true;

      SECTION( "src fog, dst fog" ) { f(); }

      SECTION( "src fog, dst no fog" ) {
        W.player_square( { .x = 1, .y = 0 } ) =
            PlayerSquare::explored{ .fog_status =
                                        FogStatus::clear{} };
        mock_land_view.EXPECT__animate( _ );
        f();
      }

      SECTION( "src no fog, dst fog" ) {
        W.player_square( { .x = 0, .y = 0 } ) =
            PlayerSquare::explored{ .fog_status =
                                        FogStatus::clear{} };
        mock_land_view.EXPECT__animate( _ );
        f();
      }

      SECTION( "src no fog, dst no fog" ) {
        W.player_square( { .x = 0, .y = 0 } ) =
            PlayerSquare::explored{ .fog_status =
                                        FogStatus::clear{} };
        W.player_square( { .x = 1, .y = 0 } ) =
            PlayerSquare::explored{ .fog_status =
                                        FogStatus::clear{} };
        mock_land_view.EXPECT__animate( _ );
        f();
      }
    }

    REQUIRE( W.units().unit_for( unit_id ).movement_points ==
             0 );
    REQUIRE( W.units().coord_for( unit_id ) ==
             Coord{ .x = 1, .y = 0 } );
  }

  SECTION( "two units same tribe, forfeight" ) {
    auto [dwelling_id1, unit_id1] = W.add_dwelling_and_brave_ids(
        { .x = 0, .y = 0 }, e_tribe::arawak );
    auto [dwelling_id2, unit_id2] = W.add_dwelling_and_brave_ids(
        { .x = 0, .y = 1 }, e_tribe::arawak );
    REQUIRE( W.units().unit_for( unit_id1 ).movement_points ==
             1 );
    REQUIRE( W.units().unit_for( unit_id2 ).movement_points ==
             1 );
    native_mind.EXPECT__select_unit( set{ unit_id1, unit_id2 } )
        .returns( unit_id2 );
    native_mind.EXPECT__command_for( unit_id2 )
        .returns( NativeUnitCommand::forfeight{} );
    native_mind.EXPECT__select_unit( set{ unit_id1 } )
        .returns( unit_id1 );
    native_mind.EXPECT__command_for( unit_id1 )
        .returns( NativeUnitCommand::forfeight{} );
    f();
    REQUIRE( W.units().unit_for( unit_id1 ).movement_points ==
             0 );
    REQUIRE( W.units().coord_for( unit_id1 ) ==
             Coord{ .x = 0, .y = 0 } );
    REQUIRE( W.units().unit_for( unit_id2 ).movement_points ==
             0 );
    REQUIRE( W.units().coord_for( unit_id2 ) ==
             Coord{ .x = 0, .y = 1 } );
  }

  SECTION( "two units same tribe, travel" ) {
    auto [dwelling_id1, unit_id1] = W.add_dwelling_and_brave_ids(
        { .x = 0, .y = 0 }, e_tribe::arawak );
    auto [dwelling_id2, unit_id2] = W.add_dwelling_and_brave_ids(
        { .x = 0, .y = 1 }, e_tribe::arawak );
    REQUIRE( W.units().unit_for( unit_id1 ).movement_points ==
             1 );
    REQUIRE( W.units().unit_for( unit_id2 ).movement_points ==
             1 );
    native_mind.EXPECT__select_unit( set{ unit_id1, unit_id2 } )
        .returns( unit_id2 );
    native_mind.EXPECT__command_for( unit_id2 )
        .returns( NativeUnitCommand::move{
            .direction = e_direction::e } );
    native_mind.EXPECT__select_unit( set{ unit_id1 } )
        .returns( unit_id1 );
    native_mind.EXPECT__command_for( unit_id1 )
        .returns( NativeUnitCommand::move{
            .direction = e_direction::e } );
    f();
    REQUIRE( W.units().unit_for( unit_id1 ).movement_points ==
             0 );
    REQUIRE( W.units().coord_for( unit_id1 ) ==
             Coord{ .x = 1, .y = 0 } );
    REQUIRE( W.units().unit_for( unit_id2 ).movement_points ==
             0 );
    REQUIRE( W.units().coord_for( unit_id2 ) ==
             Coord{ .x = 1, .y = 1 } );
  }

  SECTION( "two units same tribe, travel along road" ) {
    W.square( { .x = 0, .y = 0 } ).road = true;
    W.square( { .x = 1, .y = 0 } ).road = true;
    W.square( { .x = 2, .y = 0 } ).road = true;
    W.square( { .x = 3, .y = 0 } ).road = true;
    W.square( { .x = 0, .y = 1 } ).road = true;
    W.square( { .x = 1, .y = 1 } ).road = true;
    W.square( { .x = 2, .y = 1 } ).road = true;
    W.square( { .x = 3, .y = 1 } ).road = true;
    auto [dwelling_id1, unit_id1] = W.add_dwelling_and_brave_ids(
        { .x = 0, .y = 0 }, e_tribe::arawak );
    auto [dwelling_id2, unit_id2] = W.add_dwelling_and_brave_ids(
        { .x = 0, .y = 1 }, e_tribe::arawak );
    REQUIRE( W.units().unit_for( unit_id1 ).movement_points ==
             1 );
    REQUIRE( W.units().unit_for( unit_id2 ).movement_points ==
             1 );

    // Since the two units are in the same tribe we can alternate
    // moving them partially within the same turn.

    // 1. Move unit 1 to the right.
    native_mind.EXPECT__select_unit( set{ unit_id1, unit_id2 } )
        .returns( unit_id1 );
    native_mind.EXPECT__command_for( unit_id1 )
        .returns( NativeUnitCommand::move{
            .direction = e_direction::e } );
    // 2. Move unit 2 to the right.
    native_mind.EXPECT__select_unit( set{ unit_id1, unit_id2 } )
        .returns( unit_id2 );
    native_mind.EXPECT__command_for( unit_id2 )
        .returns( NativeUnitCommand::move{
            .direction = e_direction::e } );
    // 3. Move unit 1 to the right.
    native_mind.EXPECT__select_unit( set{ unit_id1, unit_id2 } )
        .returns( unit_id1 );
    native_mind.EXPECT__command_for( unit_id1 )
        .returns( NativeUnitCommand::move{
            .direction = e_direction::e } );
    // 4. Move unit 2 to the right.
    native_mind.EXPECT__select_unit( set{ unit_id1, unit_id2 } )
        .returns( unit_id2 );
    native_mind.EXPECT__command_for( unit_id2 )
        .returns( NativeUnitCommand::move{
            .direction = e_direction::e } );
    // 5. unit 1 forfeights.
    native_mind.EXPECT__select_unit( set{ unit_id1, unit_id2 } )
        .returns( unit_id1 );
    native_mind.EXPECT__command_for( unit_id1 )
        .returns( NativeUnitCommand::forfeight{} );
    // 6. Move unit 2 to the right.
    native_mind.EXPECT__select_unit( set{ unit_id2 } )
        .returns( unit_id2 );
    native_mind.EXPECT__command_for( unit_id2 )
        .returns( NativeUnitCommand::move{
            .direction = e_direction::e } );
    f();
    REQUIRE( W.units().unit_for( unit_id1 ).movement_points ==
             0 );
    REQUIRE( W.units().coord_for( unit_id1 ) ==
             Coord{ .x = 2, .y = 0 } );
    REQUIRE( W.units().unit_for( unit_id2 ).movement_points ==
             0 );
    REQUIRE( W.units().coord_for( unit_id2 ) ==
             Coord{ .x = 3, .y = 1 } );
  }

  SECTION( "two units different tribe, travel along road" ) {
    W.square( { .x = 0, .y = 0 } ).road = true;
    W.square( { .x = 1, .y = 0 } ).road = true;
    W.square( { .x = 2, .y = 0 } ).road = true;
    W.square( { .x = 3, .y = 0 } ).road = true;
    W.square( { .x = 0, .y = 1 } ).road = true;
    W.square( { .x = 1, .y = 1 } ).road = true;
    W.square( { .x = 2, .y = 1 } ).road = true;
    W.square( { .x = 3, .y = 1 } ).road = true;
    auto [dwelling_id1, unit_id1] = W.add_dwelling_and_brave_ids(
        { .x = 0, .y = 0 }, e_tribe::arawak );
    auto [dwelling_id2, unit_id2] = W.add_dwelling_and_brave_ids(
        { .x = 0, .y = 1 }, e_tribe::inca );
    REQUIRE( W.units().unit_for( unit_id1 ).movement_points ==
             1 );
    REQUIRE( W.units().unit_for( unit_id2 ).movement_points ==
             1 );

    // Since the two units are in different tribes we have to
    // move one fully first, then the next.
    MockINativeMind& native_mind2 =
        W.native_mind( e_tribe::inca );

    // 1. Move unit 1 to the right.
    native_mind.EXPECT__select_unit( set{ unit_id1 } )
        .returns( unit_id1 );
    native_mind.EXPECT__command_for( unit_id1 )
        .returns( NativeUnitCommand::move{
            .direction = e_direction::e } );
    // 2. Move unit 1 to the right.
    native_mind.EXPECT__select_unit( set{ unit_id1 } )
        .returns( unit_id1 );
    native_mind.EXPECT__command_for( unit_id1 )
        .returns( NativeUnitCommand::move{
            .direction = e_direction::e } );
    // 3. unit 1 forfeights.
    native_mind.EXPECT__select_unit( set{ unit_id1 } )
        .returns( unit_id1 );
    native_mind.EXPECT__command_for( unit_id1 )
        .returns( NativeUnitCommand::forfeight{} );
    // 4. Move unit 2 to the right.
    native_mind2.EXPECT__select_unit( set{ unit_id2 } )
        .returns( unit_id2 );
    native_mind2.EXPECT__command_for( unit_id2 )
        .returns( NativeUnitCommand::move{
            .direction = e_direction::e } );
    // 5. Move unit 2 to the right.
    native_mind2.EXPECT__select_unit( set{ unit_id2 } )
        .returns( unit_id2 );
    native_mind2.EXPECT__command_for( unit_id2 )
        .returns( NativeUnitCommand::move{
            .direction = e_direction::e } );
    // 6. Move unit 2 to the right.
    native_mind2.EXPECT__select_unit( set{ unit_id2 } )
        .returns( unit_id2 );
    native_mind2.EXPECT__command_for( unit_id2 )
        .returns( NativeUnitCommand::move{
            .direction = e_direction::e } );
    f();
    REQUIRE( W.units().unit_for( unit_id1 ).movement_points ==
             0 );
    REQUIRE( W.units().coord_for( unit_id1 ) ==
             Coord{ .x = 2, .y = 0 } );
    REQUIRE( W.units().unit_for( unit_id2 ).movement_points ==
             0 );
    REQUIRE( W.units().coord_for( unit_id2 ) ==
             Coord{ .x = 3, .y = 1 } );
  }
}

TEST_CASE( "[native-turn] attack euro unit" ) {
  World             W;
  MockIRaid         mock_raid;
  MockITribeEvolve  mock_tribe_evolver;
  MockLandViewPlane mock_land_view;
  W.planes().back().land_view = &mock_land_view;

  auto f = [&] {
    co_await_test( natives_turn( W.ss(), W.ts(), mock_raid,
                                 mock_tribe_evolver ) );
  };

  MockINativeMind& native_mind =
      W.native_mind( e_tribe::arawak );
  MockIEuroMind& mock_euro_mind = W.euro_mind();

  auto [dwelling, brave] = W.add_dwelling_and_brave(
      { .x = 0, .y = 0 }, e_tribe::arawak );
  Coord const defender_loc = { .x = 1, .y = 0 };

  mock_tribe_evolver.EXPECT__evolve_tribe_common(
      e_tribe::arawak );
  mock_tribe_evolver.EXPECT__evolve_dwellings_for_tribe(
      e_tribe::arawak );
  native_mind.EXPECT__select_unit( set{ brave.id } )
      .returns( brave.id );
  native_mind.EXPECT__command_for( brave.id )
      .returns( NativeUnitCommand::move{ e_direction::e } );
  mock_euro_mind.EXPECT__show_woodcut( e_woodcut::indian_raid );

  SECTION( "brave, one euro, brave loses, soldier promoted" ) {
    Unit const& soldier =
        W.add_unit_on_map( e_unit_type::soldier, defender_loc );
    mock_raid.EXPECT__raid_unit( brave, defender_loc );
    f();
    REQUIRE( brave.movement_points == 0 );
    REQUIRE( soldier.movement_points() == 1 );
  }

  SECTION( "brave, one euro, brave wins" ) {
    Unit const& soldier =
        W.add_unit_on_map( e_unit_type::soldier, defender_loc );
    mock_raid.EXPECT__raid_unit( brave, defender_loc );
    f();
    REQUIRE( brave.movement_points == 0 );
    REQUIRE( soldier.movement_points() == 1 );
  }

  SECTION( "brave, three euro, brave loses" ) {
    // The soldier should be chosen as defender.
    Unit const& free_colonist1 = W.add_unit_on_map(
        e_unit_type::free_colonist, defender_loc );
    // Put the soldier in the middle so we can test that it gets
    // picked for the right reasons.
    Unit const& soldier =
        W.add_unit_on_map( e_unit_type::soldier, defender_loc );
    Unit const& free_colonist2 = W.add_unit_on_map(
        e_unit_type::free_colonist, defender_loc );
    mock_raid.EXPECT__raid_unit( brave, defender_loc );
    f();
    REQUIRE( brave.movement_points == 0 );
    REQUIRE( soldier.movement_points() == 1 );
    REQUIRE( free_colonist1.movement_points() == 1 );
    REQUIRE( free_colonist2.movement_points() == 1 );
  }
}

TEST_CASE( "[native-turn] brave spawns" ) {
  World            W;
  MockIRaid        mock_raid;
  MockITribeEvolve mock_tribe_evolver;

  auto f = [&] {
    co_await_test( natives_turn( W.ss(), W.ts(), mock_raid,
                                 mock_tribe_evolver ) );
  };

  MockINativeMind& native_mind =
      W.native_mind( e_tribe::arawak );
  DwellingId const dwelling_id =
      W.add_dwelling( { .x = 0, .y = 0 }, e_tribe::arawak ).id;
  maybe<NativeUnitId> native_unit_id;

  mock_tribe_evolver.EXPECT__evolve_tribe_common(
      e_tribe::arawak );
  mock_tribe_evolver
      .EXPECT__evolve_dwellings_for_tribe( e_tribe::arawak )
      .invokes( [&] {
        native_unit_id = W.add_native_unit_on_map(
                              e_native_unit_type::mounted_brave,
                              { .x = 0, .y = 0 }, dwelling_id )
                             .id;
      } );

  native_mind.EXPECT__select_unit( set{ NativeUnitId{ 1 } } )
      .returns( NativeUnitId{ 1 } );
  native_mind.EXPECT__command_for( NativeUnitId{ 1 } )
      .returns( NativeUnitCommand::forfeight{} );
  f();
  REQUIRE( native_unit_id == NativeUnitId{ 1 } );
  REQUIRE( W.units().exists( *native_unit_id ) );
  NativeUnit const& mounted_brave =
      W.units().unit_for( NativeUnitId{ 1 } );
  REQUIRE( mounted_brave.type ==
           e_native_unit_type::mounted_brave );
  REQUIRE( mounted_brave.movement_points == 0 );
}

TEST_CASE( "[native-turn] brave equips" ) {
  World            W;
  MockIRaid        mock_raid;
  MockITribeEvolve mock_tribe_evolver;

  auto f = [&] {
    co_await_test( natives_turn( W.ss(), W.ts(), mock_raid,
                                 mock_tribe_evolver ) );
  };

  MockINativeMind& native_mind =
      W.native_mind( e_tribe::arawak );
  Tribe&           tribe = W.add_tribe( e_tribe::arawak );
  DwellingId const dwelling_id =
      W.add_dwelling( { .x = 0, .y = 0 }, e_tribe::arawak ).id;
  NativeUnit const& brave = W.add_native_unit_on_map(
      e_native_unit_type::brave, { .x = 0, .y = 0 },
      dwelling_id );
  NativeUnitId const brave_id = brave.id;

  REQUIRE( brave_id == NativeUnitId{ 1 } );
  REQUIRE( brave.movement_points == 1 );

  tribe.muskets        = 5;
  tribe.horse_herds    = 10;
  tribe.horse_breeding = 30;

  mock_tribe_evolver.EXPECT__evolve_tribe_common(
      e_tribe::arawak );
  mock_tribe_evolver.EXPECT__evolve_dwellings_for_tribe(
      e_tribe::arawak );
  // If you have a situation where the below functions are called
  // more than once, then it is probably because the brave has
  // not had its movement points reset to zero properly and so it
  // keeps asking the brave for orders.
  native_mind.EXPECT__select_unit( set{ NativeUnitId{ 1 } } )
      .returns( NativeUnitId{ 1 } );
  native_mind.EXPECT__command_for( NativeUnitId{ 1 } )
      .returns( NativeUnitCommand::equip{
          .how = { .type = e_native_unit_type::mounted_warrior,
                   .muskets_delta        = -1,
                   .horse_breeding_delta = -3 } } );
  f();
  REQUIRE( W.units().all().size() == 1 );
  REQUIRE( W.units().exists( brave_id ) );
  REQUIRE( brave.type == e_native_unit_type::mounted_warrior );
  REQUIRE( tribe.muskets == 4 );
  REQUIRE( tribe.horse_herds == 10 );
  REQUIRE( tribe.horse_breeding == 27 );
  REQUIRE( brave.movement_points == 0 );
}

} // namespace
} // namespace rn
