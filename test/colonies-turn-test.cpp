/****************************************************************
**colonies-turn-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-04-02.
*
* Description: Unit tests for the colonies-turn module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/colonies-turn.hpp"

// Testing.
#include "test/fake/world.hpp"
#include "test/mocking.hpp"
#include "test/mocks/igui.hpp"
#include "test/mocks/land-view-plane.hpp"
#include "test/util/coro.hpp"

// Revolution Now
#include "src/colony-evolve.hpp"
#include "src/plane-stack.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

using ::mock::matchers::_;
using ::mock::matchers::Eq;
using ::mock::matchers::Field;
using ::mock::matchers::StrContains;

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
    MapSquare const   _ = make_ocean();
    MapSquare const   L = make_grassland();
    vector<MapSquare> tiles{
        _, L, _, L, L, L, //
        L, L, L, L, L, L, //
        _, L, L, L, L, L, //
        _, L, _, L, L, L, //
        _, L, L, L, L, L, //
        L, L, L, L, L, L, //
    };
    build_map( std::move( tiles ), 6 );
  }
};

/****************************************************************
** Mocks.
*****************************************************************/
DEFINE_MOCK_IColonyEvolver();

/****************************************************************
** Test Cases
*****************************************************************/
// FIXME: this test was imported from before we mocked the depen-
// dencies and so it probably should be replaced with more thor-
// ough unit tests.
TEST_CASE( "[colonies-turn] presents transient updates." ) {
  World              W;
  MockIColonyEvolver mock_colony_evolver;

  MockLandViewPlane land_view_plane;
  W.planes().back().land_view = &land_view_plane;

  auto evolve_colonies = [&] {
    co_await_test( evolve_colonies_for_player(
        W.ss(), W.ts(), W.default_player(),
        mock_colony_evolver ) );
  };

  SECTION( "without updates" ) {
    // This one should not try to center the viewport on the
    // colony.
    for( Coord const coord :
         vector{ Coord{ .x = 1, .y = 1 },
                 Coord{ .x = 3, .y = 3 } } ) {
      Colony& colony = W.add_colony( coord );
      // Doesn't matter what this holds, only the count.
      ColonyEvolution const evolution{ .notifications = { {} } };
      mock_colony_evolver
          .EXPECT__evolve_one_turn( Eq( ref( colony ) ) )
          .returns( evolution );
      mock_colony_evolver
          .EXPECT__generate_notification_message(
              colony, evolution.notifications[0] )
          .returns( ColonyNotificationMessage{
              .msg       = "xxx"s + to_string( colony.id ),
              .transient = true } );
    }

    W.gui().EXPECT__transient_message_box( "xxx1" );
    W.gui().EXPECT__transient_message_box( "xxx2" );
    evolve_colonies();
  }

  SECTION( "with blocking updates" ) {
    for( Coord const coord :
         vector{ Coord{ .x = 1, .y = 1 },
                 Coord{ .x = 3, .y = 3 } } ) {
      Colony& colony = W.add_colony( coord );
      // Doesn't matter what this holds, only the count.
      ColonyEvolution const evolution{
          .notifications = { {}, {} } };
      mock_colony_evolver
          .EXPECT__evolve_one_turn( Eq( ref( colony ) ) )
          .returns( evolution );
      mock_colony_evolver
          .EXPECT__generate_notification_message(
              colony, evolution.notifications[0] )
          .returns( ColonyNotificationMessage{
              .msg       = "xxx"s + to_string( colony.id ),
              .transient = false } );
      mock_colony_evolver
          .EXPECT__generate_notification_message(
              colony, evolution.notifications[1] )
          .returns( ColonyNotificationMessage{
              .msg       = "xxx"s + to_string( colony.id ),
              .transient = true } );
      land_view_plane.EXPECT__ensure_visible( coord );
      W.gui()
          .EXPECT__choice(
              Field( &ChoiceConfig::msg,
                     "xxx"s + to_string( colony.id ) ),
              _ )
          .returns<maybe<string>>( nothing );
    }

    // The transient messages should be grouped at the end for
    // both colonies. FIXME: this only verifies that they are
    // called (which is probably good enough) but doesn't actu-
    // ally verify that they are grouped together at the end. Not
    // sure how to easily do that with the current mocking frame-
    // work.
    W.gui().EXPECT__transient_message_box( "xxx1" );
    W.gui().EXPECT__transient_message_box( "xxx2" );
    evolve_colonies();
  }
}

} // namespace
} // namespace rn
