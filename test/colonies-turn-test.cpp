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
#include "test/mocks/iagent.hpp"
#include "test/mocks/igui.hpp"
#include "test/mocks/iharbor-viewer.hpp"
#include "test/mocks/irand.hpp"
#include "test/mocks/land-view-plane.hpp"
#include "test/util/coro.hpp"

// Revolution Now
#include "src/icolony-evolve.rds.hpp"
#include "src/plane-stack.hpp"

// ss
#include "src/ss/player.rds.hpp"

// refl
#include "src/refl/to-str.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

RDS_DEFINE_MOCK( IColonyEvolver );
RDS_DEFINE_MOCK( IColonyNotificationGenerator );

namespace rn {
namespace {

using namespace std;
using namespace rn::signal;

using ::gfx::point;
using ::mock::matchers::_;
using ::mock::matchers::Eq;
using ::mock::matchers::Field;
using ::mock::matchers::StrContains;
using ::mock::matchers::Type;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct world : testing::World {
  using Base = testing::World;
  world() : Base() {
    add_default_player();
    set_default_player_as_human();
    create_default_map();
  }

  void create_default_map() {
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
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
** Test Cases
*****************************************************************/
// FIXME: this test was imported from before we mocked the depen-
// dencies and so it probably should be replaced with more thor-
// ough unit tests.
TEST_CASE( "[colonies-turn] presents transient updates." ) {
  world w;
  MockIColonyEvolver mock_colony_evolver;
  MockIColonyNotificationGenerator
      mock_colony_notification_generator;

  MockLandViewPlane land_view_plane;
  w.planes().get().set_bottom<ILandViewPlane>( land_view_plane );

  MockIHarborViewer harbor_viewer;

  auto evolve_colonies = [&] {
    co_await_test( evolve_colonies_for_player(
        w.ss(), w.ts(), w.default_player(), mock_colony_evolver,
        harbor_viewer, mock_colony_notification_generator ) );
  };

  MockIAgent& agent = w.agent();
  agent.EXPECT__human().by_default().returns( true );

  SECTION( "without updates" ) {
    // This one should not try to center the viewport on the
    // colony.
    for( Coord const coord : vector{
           Coord{ .x = 1, .y = 1 }, Coord{ .x = 3, .y = 3 } } ) {
      Colony& colony = w.add_colony( coord );
      // Doesn't matter what this holds, only the count.
      ColonyEvolution const evolution{ .notifications = { {} } };
      mock_colony_evolver
          .EXPECT__evolve_colony_one_turn( Eq( ref( colony ) ) )
          .returns( evolution );
      mock_colony_notification_generator
          .EXPECT__generate_colony_notification_message(
              colony, evolution.notifications[0] )
          .returns( ColonyNotificationMessage{
            .msg       = "xxx"s + to_string( colony.id ),
            .transient = true } );
    }

    agent.EXPECT__handle( Type<ColonySignalTransient>() );
    agent.EXPECT__handle( Type<ColonySignalTransient>() );
    evolve_colonies();
  }

  SECTION( "with blocking updates" ) {
    for( point const p : vector<point>{ { .x = 1, .y = 1 },
                                        { .x = 3, .y = 3 } } ) {
      Colony& colony = w.add_colony( p );
      // Doesn't matter what this holds, only the count.
      ColonyEvolution const evolution{
        .notifications = { {}, {} } };
      mock_colony_evolver
          .EXPECT__evolve_colony_one_turn( Eq( ref( colony ) ) )
          .returns( evolution );
      mock_colony_notification_generator
          .EXPECT__generate_colony_notification_message(
              colony, evolution.notifications[0] )
          .returns( ColonyNotificationMessage{
            .msg       = "xxx"s + to_string( colony.id ),
            .transient = false } );
      mock_colony_notification_generator
          .EXPECT__generate_colony_notification_message(
              colony, evolution.notifications[1] )
          .returns( ColonyNotificationMessage{
            .msg       = "xxx"s + to_string( colony.id ),
            .transient = true } );
      agent.EXPECT__pan_tile( p );
      w.gui()
          .EXPECT__choice(
              Field( &ChoiceConfig::msg,
                     "xxx"s + to_string( colony.id ) ) )
          .returns<maybe<string>>( nothing );
    }

    // The transient messages should be grouped at the end for
    // both colonies. FIXME: this only verifies that they are
    // called (which is probably good enough) but doesn't actu-
    // ally verify that they are grouped together at the end. Not
    // sure how to easily do that with the current mocking frame-
    // work.
    agent.EXPECT__handle( Type<ColonySignalTransient>() );
    agent.EXPECT__handle( Type<ColonySignalTransient>() );
    evolve_colonies();
  }

  SECTION(
      "does not show harbor on new immigrant with no ships in "
      "port" ) {
    Player& player = w.default_player();
    player.fathers.has[e_founding_father::william_brewster] =
        false;
    // Trigger a new immigrant.  It will be chosen automatically
    // for the player due to lack of Brewster.
    player.crosses = 1000;
    // Select the immigratn.
    w.rand().EXPECT__between_ints( 0, 2 ).returns( 1 );
    // Notify player.
    agent.EXPECT__message_box( StrContains( "immigrant" ) );
    agent.EXPECT__handle( ImmigrantArrived{
      .type = e_unit_type::petty_criminal } );
    // Select pool replacement.
    w.rand().EXPECT__between_doubles( 0, _ ).returns( 0.0 );
    evolve_colonies();
  }

  SECTION( "shows harbor on new immigrant with ship in port" ) {
    Player& player = w.default_player();
    w.add_unit_in_port( e_unit_type::caravel );
    player.fathers.has[e_founding_father::william_brewster] =
        false;
    // Trigger a new immigrant.  It will be chosen automatically
    // for the player due to lack of Brewster.
    player.crosses = 1000;
    // Select the immigratn.
    w.rand().EXPECT__between_ints( 0, 2 ).returns( 1 );
    // Notify player.
    agent.EXPECT__message_box( StrContains( "immigrant" ) );
    agent.EXPECT__handle( ImmigrantArrived{
      .type = e_unit_type::petty_criminal } );
    // Select pool replacement.
    w.rand().EXPECT__between_doubles( 0, _ ).returns( 0.0 );
    // Show the harbor view since the player has a ship in port.
    harbor_viewer.EXPECT__show();
    evolve_colonies();
  }
}

TEST_CASE(
    "[colonies-turn] only evolves crosses pre-declaration." ) {
  world w;
  MockIColonyEvolver mock_colony_evolver;
  MockIColonyNotificationGenerator
      mock_colony_notification_generator;

  Player const& player = w.default_player();

  MockLandViewPlane land_view_plane;
  w.planes().get().set_bottom<ILandViewPlane>( land_view_plane );

  MockIHarborViewer harbor_viewer;

  auto const evolve_colonies = [&] {
    co_await_test( evolve_colonies_for_player(
        w.ss(), w.ts(), w.default_player(), mock_colony_evolver,
        harbor_viewer, mock_colony_notification_generator ) );
  };

  point const kPoint{ .x = 1, .y = 1 };
  Colony& colony = w.add_colony( kPoint );
  ColonyEvolution const evolution{
    .production = { .crosses = 5 } };
  mock_colony_evolver
      .EXPECT__evolve_colony_one_turn( Eq( ref( colony ) ) )
      .returns( evolution );

  SECTION( "before declaration" ) {
    evolve_colonies();
    REQUIRE( player.crosses == 5 + 2 );
  }

  SECTION( "before declaration" ) {
    w.declare_independence();
    evolve_colonies();
    REQUIRE( player.crosses == 0 );
  }
}

} // namespace
} // namespace rn
