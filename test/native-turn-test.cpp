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
#include "test/mocks/inative-mind.hpp"
#include "test/mocks/irand.hpp"
#include "test/mocks/land-view-plane.hpp"
#include "test/util/coro.hpp"

// Revolution Now
#include "src/plane-stack.hpp"

// ss
#include "src/ss/settings.rds.hpp"
#include "src/ss/units.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp"

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
    add_default_player();
    // Need at least one human player otherwise the map will be
    // fully visible during the natives' turns and we won't be
    // able to test whether animations get suppressed or not due
    // to map visibility.
    set_human_player( default_nation() );
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
TEST_CASE(
    "[native-turn] natives_turn, unit iteration, travel" ) {
  World W;

  auto f = [&] {
    co_await_test( natives_turn( W.ss(), W.ts() ) );
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
        NativeUnitCommand::travel{ .direction =
                                       e_direction::e } );
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
        NativeUnitCommand::travel{ .direction =
                                       e_direction::e } );
    // Second move.
    native_mind.EXPECT__select_unit( set{ unit_id } )
        .returns( unit_id );
    native_mind.EXPECT__command_for( unit_id ).returns(
        NativeUnitCommand::travel{ .direction =
                                       e_direction::e } );
    // Third move.
    native_mind.EXPECT__select_unit( set{ unit_id } )
        .returns( unit_id );
    native_mind.EXPECT__command_for( unit_id ).returns(
        NativeUnitCommand::travel{ .direction =
                                       e_direction::e } );
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
        NativeUnitCommand::travel{ .direction =
                                       e_direction::e } );
    // Second move.
    native_mind.EXPECT__select_unit( set{ unit_id } )
        .returns( unit_id );
    native_mind.EXPECT__command_for( unit_id ).returns(
        NativeUnitCommand::travel{ .direction =
                                       e_direction::e } );
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
        NativeUnitCommand::travel{ .direction =
                                       e_direction::e } );
    // Second move.
    native_mind.EXPECT__select_unit( set{ unit_id } )
        .returns( unit_id );
    native_mind.EXPECT__command_for( unit_id ).returns(
        NativeUnitCommand::travel{ .direction =
                                       e_direction::e } );
    // Third move.
    native_mind.EXPECT__select_unit( set{ unit_id } )
        .returns( unit_id );
    native_mind.EXPECT__command_for( unit_id ).returns(
        NativeUnitCommand::travel{ .direction =
                                       e_direction::e } );

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
        NativeUnitCommand::travel{ .direction =
                                       e_direction::e } );

    bool& show_indian_moves =
        W.settings()
            .game_options
            .flags[e_game_flag_option::show_indian_moves];

    SECTION( "show_indian_moves=false" ) {
      show_indian_moves = false;

      SECTION( "src fog, dst fog" ) { f(); }

      SECTION( "src no fog, dst fog" ) {
        W.player_square( { .x = 0, .y = 0 } )
            .emplace()
            .fog_of_war_removed = true;
        f();
      }

      SECTION( "src fog, dst no fog" ) {
        W.player_square( { .x = 1, .y = 0 } )
            .emplace()
            .fog_of_war_removed = true;
        f();
      }
    }

    SECTION( "show_indian_moves=true" ) {
      show_indian_moves = true;

      SECTION( "src fog, dst fog" ) { f(); }

      SECTION( "src fog, dst no fog" ) {
        W.player_square( { .x = 1, .y = 0 } )
            .emplace()
            .fog_of_war_removed = true;
        mock_land_view.EXPECT__animate( _ ).returns();
        f();
      }

      SECTION( "src no fog, dst fog" ) {
        W.player_square( { .x = 0, .y = 0 } )
            .emplace()
            .fog_of_war_removed = true;
        mock_land_view.EXPECT__animate( _ ).returns();
        f();
      }

      SECTION( "src no fog, dst no fog" ) {
        W.player_square( { .x = 0, .y = 0 } )
            .emplace()
            .fog_of_war_removed = true;
        W.player_square( { .x = 1, .y = 0 } )
            .emplace()
            .fog_of_war_removed = true;
        mock_land_view.EXPECT__animate( _ ).returns();
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
        .returns( NativeUnitCommand::travel{
            .direction = e_direction::e } );
    native_mind.EXPECT__select_unit( set{ unit_id1 } )
        .returns( unit_id1 );
    native_mind.EXPECT__command_for( unit_id1 )
        .returns( NativeUnitCommand::travel{
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
        .returns( NativeUnitCommand::travel{
            .direction = e_direction::e } );
    // 2. Move unit 2 to the right.
    native_mind.EXPECT__select_unit( set{ unit_id1, unit_id2 } )
        .returns( unit_id2 );
    native_mind.EXPECT__command_for( unit_id2 )
        .returns( NativeUnitCommand::travel{
            .direction = e_direction::e } );
    // 3. Move unit 1 to the right.
    native_mind.EXPECT__select_unit( set{ unit_id1, unit_id2 } )
        .returns( unit_id1 );
    native_mind.EXPECT__command_for( unit_id1 )
        .returns( NativeUnitCommand::travel{
            .direction = e_direction::e } );
    // 4. Move unit 2 to the right.
    native_mind.EXPECT__select_unit( set{ unit_id1, unit_id2 } )
        .returns( unit_id2 );
    native_mind.EXPECT__command_for( unit_id2 )
        .returns( NativeUnitCommand::travel{
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
        .returns( NativeUnitCommand::travel{
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
        .returns( NativeUnitCommand::travel{
            .direction = e_direction::e } );
    // 2. Move unit 1 to the right.
    native_mind.EXPECT__select_unit( set{ unit_id1 } )
        .returns( unit_id1 );
    native_mind.EXPECT__command_for( unit_id1 )
        .returns( NativeUnitCommand::travel{
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
        .returns( NativeUnitCommand::travel{
            .direction = e_direction::e } );
    // 5. Move unit 2 to the right.
    native_mind2.EXPECT__select_unit( set{ unit_id2 } )
        .returns( unit_id2 );
    native_mind2.EXPECT__command_for( unit_id2 )
        .returns( NativeUnitCommand::travel{
            .direction = e_direction::e } );
    // 6. Move unit 2 to the right.
    native_mind2.EXPECT__select_unit( set{ unit_id2 } )
        .returns( unit_id2 );
    native_mind2.EXPECT__command_for( unit_id2 )
        .returns( NativeUnitCommand::travel{
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

} // namespace
} // namespace rn
