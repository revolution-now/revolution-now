/****************************************************************
**on-map.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-17.
*
* Description: Unit tests for the src/on-map.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/on-map.hpp"

// Testing
#include "test/fake/world.hpp"
#include "test/mocks/iagent.hpp"
#include "test/mocks/irand.hpp"
#include "test/mocks/land-view-plane.hpp"
#include "test/util/coro.hpp"

// Revolution Now
#include "src/plane-stack.hpp"

// mock
#include "src/mock/matchers.hpp"

// ss
#include "src/ss/dwelling.rds.hpp"
#include "src/ss/player.rds.hpp"
#include "src/ss/terrain.hpp"
#include "src/ss/tribe.rds.hpp"
#include "src/ss/units.hpp"

// refl
#include "src/refl/to-str.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {

struct TestingOnlyUnitOnMapMover : UnitOnMapMover {
  using Base = UnitOnMapMover;
  using Base::to_map_interactive;
  using Base::to_map_non_interactive;
};

namespace {

using namespace ::std;
using namespace ::rn::signal;

using ::gfx::point;
using ::mock::matchers::_;
using ::mock::matchers::StrContains;
using ::mock::matchers::Type;

using unexplored = PlayerSquare::unexplored;
using explored   = PlayerSquare::explored;
using clear      = FogStatus::clear;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() {
    add_player( e_player::dutch );
    add_player( e_player::spanish );
    set_default_player_type( e_player::dutch );

    planes().get().set_bottom<ILandViewPlane>( mock_land_view );
  }

  Coord const kColonySquare = Coord{ .x = 1, .y = 1 };

  void create_default_map() {
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
    // clang-format off
    vector<MapSquare> tiles{
      _, L, L, L, L, L,
      L, L, L, L, L, L,
      _, L, L, L, L, L,
      _, L, L, L, L, L,
      _, L, L, L, L, L,
      _, L, L, L, L, L,
    };
    // clang-format on
    build_map( std::move( tiles ), 6 );
  }

  void create_island_map() {
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
    // clang-format off
    vector<MapSquare> tiles{
      _, _, _, //
      _, L, _, //
      _, _, _, //
    };
    // clang-format on
    build_map( std::move( tiles ), 3 );
  }

  MockLandViewPlane mock_land_view;
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[on-map] non-interactive: moves the unit" ) {
  World W;
  W.create_default_map();

  SECTION( "euro unit" ) {
    UnitId const unit_id =
        W.add_unit_on_map( e_unit_type::treasure,
                           { .x = 1, .y = 0 } )
            .id();
    TestingOnlyUnitOnMapMover::to_map_non_interactive(
        W.ss(), W.map_updater(), unit_id, { .x = 0, .y = 1 } );
    REQUIRE( W.units().coord_for( unit_id ) ==
             Coord{ .x = 0, .y = 1 } );
  }

  SECTION( "native unit" ) {
    Dwelling const& dwelling =
        W.add_dwelling( { .x = 0, .y = 0 }, e_tribe::apache );
    NativeUnitId const unit_id =
        W.add_native_unit_on_map(
             e_native_unit_type::armed_brave, { .x = 1, .y = 0 },
             dwelling.id )
            .id;
    UnitOnMapMover::native_unit_to_map_non_interactive(
        W.ss(), unit_id, { .x = 1, .y = 1 } );
    REQUIRE( W.units().coord_for( unit_id ) ==
             Coord{ .x = 1, .y = 1 } );
  }
}

TEST_CASE( "[on-map] interactive: discovers new world" ) {
  World W;
  W.create_default_map();
  Player& player    = W.default_player();
  MockIAgent& agent = W.agent();
  UnitId const unit_id =
      W.add_unit_on_map( e_unit_type::treasure,
                         { .x = 1, .y = 0 } )
          .id();
  wait<maybe<UnitDeleted>> w = make_wait<maybe<UnitDeleted>>();

  REQUIRE( player.new_world_name == nothing );

  SECTION( "already discovered" ) {
    player.new_world_name = "my world";
    player.woodcuts[e_woodcut::discovered_new_world] = true;
    w = TestingOnlyUnitOnMapMover::to_map_interactive(
        W.ss(), W.ts(), W.rand(), unit_id, { .x = 0, .y = 1 } );
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    REQUIRE( *w == nothing );
    REQUIRE( W.units().coord_for( unit_id ) ==
             Coord{ .x = 0, .y = 1 } );
    REQUIRE( player.new_world_name == "my world" );
  }

  SECTION( "not yet discovered" ) {
    W.agent().EXPECT__show_woodcut(
        e_woodcut::discovered_new_world );
    agent.EXPECT__name_new_world().returns( "my world 2" );
    w = TestingOnlyUnitOnMapMover::to_map_interactive(
        W.ss(), W.ts(), W.rand(), unit_id, { .x = 0, .y = 1 } );
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    REQUIRE( *w == nothing );
    REQUIRE( W.units().coord_for( unit_id ) ==
             Coord{ .x = 0, .y = 1 } );
    REQUIRE( player.new_world_name == "my world 2" );
  }
}

TEST_CASE( "[on-map] interactive: meets natives" ) {
  World W;
  W.create_default_map();
  Player& player    = W.default_player();
  MockIAgent& agent = W.agent();

  W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::cherokee );

  UnitId const unit_id =
      W.add_unit_on_map( e_unit_type::free_colonist,
                         { .x = 1, .y = 3 } )
          .id();

  player.new_world_name                            = "my world";
  player.woodcuts[e_woodcut::discovered_new_world] = true;

  agent.EXPECT__meet_tribe_ui_sequence(
      MeetTribe{ .player        = e_player::dutch,
                 .tribe         = e_tribe::cherokee,
                 .num_dwellings = 1,
                 .land_awarded  = {} },
      point{ .x = 1, .y = 0 } );

  REQUIRE( W.cherokee().relationship[player.type].encountered ==
           false );

  auto const w = TestingOnlyUnitOnMapMover::to_map_interactive(
      W.ss(), W.ts(), W.rand(), unit_id, { .x = 1, .y = 0 } );
  REQUIRE( !w.exception() );
  REQUIRE( w.ready() );
  REQUIRE( *w == nothing );

  REQUIRE( W.cherokee().relationship[player.type].encountered ==
           true );
  REQUIRE( W.units().coord_for( unit_id ) ==
           Coord{ .x = 1, .y = 0 } );
}

TEST_CASE( "[on-map] interactive: discovers pacific ocean" ) {
  World W;
  W.create_default_map();
  Player& player                           = W.default_player();
  player.new_world_name                    = "";
  W.terrain().pacific_ocean_endpoints()[0] = 1;
  W.terrain().pacific_ocean_endpoints()[1] = 2;
  W.terrain().pacific_ocean_endpoints()[2] = 1;
  UnitId const unit_id =
      W.add_unit_on_map( e_unit_type::treasure,
                         { .x = 2, .y = 4 } )
          .id();
  wait<maybe<UnitDeleted>> w = make_wait<maybe<UnitDeleted>>();

  REQUIRE(
      player.woodcuts[e_woodcut::discovered_pacific_ocean] ==
      false );

  W.agent().EXPECT__show_woodcut(
      e_woodcut::discovered_pacific_ocean );
  w = TestingOnlyUnitOnMapMover::to_map_interactive(
      W.ss(), W.ts(), W.rand(), unit_id, { .x = 1, .y = 3 } );
  REQUIRE( !w.exception() );
  REQUIRE( w.ready() );
  REQUIRE( *w == nothing );
  REQUIRE( W.units().coord_for( unit_id ) ==
           Coord{ .x = 1, .y = 3 } );
  REQUIRE(
      player.woodcuts[e_woodcut::discovered_pacific_ocean] ==
      true );

  // Make sure it doesn't happen again.
  REQUIRE( W.terrain().is_pacific_ocean( { .x = 0, .y = 2 } ) );
  w = TestingOnlyUnitOnMapMover::to_map_interactive(
      W.ss(), W.ts(), W.rand(), unit_id, { .x = 0, .y = 3 } );
  REQUIRE( !w.exception() );
  REQUIRE( w.ready() );
  REQUIRE( *w == nothing );
  REQUIRE( W.units().coord_for( unit_id ) ==
           Coord{ .x = 0, .y = 3 } );
  REQUIRE(
      player.woodcuts[e_woodcut::discovered_pacific_ocean] ==
      true );
}

TEST_CASE( "[on-map] interactive: treasure in colony" ) {
  World W;
  W.create_default_map();
  Player& player    = W.default_player();
  MockIAgent& agent = W.agent();
  W.found_colony_with_new_unit( W.kColonySquare );
  UnitId const unit_id =
      W.add_unit_on_map( e_unit_type::treasure,
                         { .x = 1, .y = 0 } )
          .id();
  wait<maybe<UnitDeleted>> w = make_wait<maybe<UnitDeleted>>();

  // This is so that the user doesn't get prompted to name the
  // new world.
  player.new_world_name = "";

  SECTION( "not entering colony" ) {
    w = TestingOnlyUnitOnMapMover::to_map_interactive(
        W.ss(), W.ts(), W.rand(), unit_id, { .x = 0, .y = 1 } );
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    REQUIRE( *w == nothing );
    REQUIRE( W.units().coord_for( unit_id ) ==
             Coord{ .x = 0, .y = 1 } );
  }

  SECTION( "entering colony answer no" ) {
    agent.EXPECT__should_king_transport_treasure( _ ).returns(
        ui::e_confirm::no );
    w = TestingOnlyUnitOnMapMover::to_map_interactive(
        W.ss(), W.ts(), W.rand(), unit_id, { .x = 1, .y = 1 } );
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    REQUIRE( *w == nothing );
    REQUIRE( W.units().coord_for( unit_id ) ==
             Coord{ .x = 1, .y = 1 } );
  }

  SECTION( "entering colony answer yes" ) {
    agent.EXPECT__should_king_transport_treasure( _ ).returns(
        ui::e_confirm::yes );
    string const msg =
        "Treasure worth 1000\x7f arrives in Amsterdam!  The "
        "crown has provided a reimbursement of [500\x7f] after "
        "a [50%] witholding.";
    agent.EXPECT__message_box( msg );
    agent.EXPECT__handle( TreasureArrived{} );
    w = TestingOnlyUnitOnMapMover::to_map_interactive(
        W.ss(), W.ts(), W.rand(), unit_id, { .x = 1, .y = 1 } );
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    REQUIRE( w->has_value() );
    REQUIRE( !W.units().exists( unit_id ) );
  }
}

TEST_CASE(
    "[on-map] interactive: [LCR] shows fountain of youth "
    "woodcut" ) {
  World W;
  W.create_default_map();
  Coord to         = { .x = 1, .y = 1 };
  Unit const& unit = W.add_unit_on_map(
      e_unit_type::free_colonist, { .x = 1, .y = 0 } );
  MapSquare& square      = W.square( { .x = 1, .y = 1 } );
  square.lost_city_rumor = true;
  Player& player         = W.default_player();
  MockIAgent& agent      = W.agent();
  agent.EXPECT__human().by_default().returns( true );
  player.new_world_name                            = "my world";
  player.woodcuts[e_woodcut::discovered_new_world] = true;

  auto f = [&] {
    wait<maybe<UnitDeleted>> const w =
        TestingOnlyUnitOnMapMover::to_map_interactive(
            W.ss(), W.ts(), W.rand(), unit.id(), to );
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    REQUIRE( *w == nothing );
  };

  // Selects rumor result = fountain of youth.
  W.rand().EXPECT__uniform_int( 0, 100 - 1 ).returns( 57 );
  agent.EXPECT__show_woodcut(
      e_woodcut::discovered_fountain_of_youth );
  agent.EXPECT__message_box( StrContains( "Youth" ) );

  for( int i = 0; i < 8; ++i ) {
    // Pick immigrant.
    agent.EXPECT__handle( Type<ChooseImmigrant>() ).returns( 0 );
    // Replace with next immigrant.
    W.rand().EXPECT__uniform_double( _, _ ).returns( 0 );
    // Wait a bit.
    agent.EXPECT__wait_for( _ ).returns(
        chrono::microseconds{} );
  }

  f();
}

TEST_CASE( "[on-map] interactive: [LCR] unit lost" ) {
  World W;
  W.create_default_map();
  Coord to = { .x = 1, .y = 1 };
  UnitId const unit_id =
      W.add_unit_on_map( e_unit_type::free_colonist,
                         { .x = 1, .y = 0 } )
          .id();
  MapSquare& square      = W.square( { .x = 1, .y = 1 } );
  square.lost_city_rumor = true;
  Player& player         = W.default_player();
  MockIAgent& agent      = W.agent();
  agent.EXPECT__human().by_default().returns( true );
  player.new_world_name                            = "my world";
  player.woodcuts[e_woodcut::discovered_new_world] = true;

  // Selects rumor result = fountain of youth.
  W.rand().EXPECT__uniform_int( 0, 100 - 1 ).returns( 94 );
  agent.EXPECT__message_box( StrContains( "vanished" ) );

  REQUIRE( W.units().exists( unit_id ) );
  wait<maybe<UnitDeleted>> const w =
      TestingOnlyUnitOnMapMover::to_map_interactive(
          W.ss(), W.ts(), W.rand(), unit_id, to );
  REQUIRE( !w.exception() );
  REQUIRE( w.ready() );
  REQUIRE( w->has_value() );
  REQUIRE_FALSE( W.units().exists( unit_id ) );
}

TEST_CASE( "[on-map] non-interactive: updates visibility" ) {
  World W;
  W.create_default_map();
  UNWRAP_CHECK( player_terrain, W.terrain().player_terrain(
                                    W.default_player_type() ) );
  auto const& map = player_terrain.map;

  REQUIRE( map[{ .x = 0, .y = 1 }] == unexplored{} );
  REQUIRE( map[{ .x = 1, .y = 1 }] == unexplored{} );
  REQUIRE( map[{ .x = 2, .y = 1 }] == unexplored{} );
  REQUIRE( map[{ .x = 3, .y = 1 }] == unexplored{} );
  REQUIRE( map[{ .x = 4, .y = 1 }] == unexplored{} );

  Unit const& unit =
      W.add_free_unit( e_unit_type::free_colonist );
  REQUIRE( map[{ .x = 0, .y = 1 }] == unexplored{} );
  REQUIRE( map[{ .x = 1, .y = 1 }] == unexplored{} );
  REQUIRE( map[{ .x = 2, .y = 1 }] == unexplored{} );
  REQUIRE( map[{ .x = 3, .y = 1 }] == unexplored{} );
  REQUIRE( map[{ .x = 4, .y = 1 }] == unexplored{} );

  TestingOnlyUnitOnMapMover::to_map_non_interactive(
      W.ss(), W.map_updater(), unit.id(), { .x = 0, .y = 1 } );
  REQUIRE( map[{ .x = 0, .y = 1 }]
               .inner_if<explored>()
               .get_if<clear>() );
  REQUIRE( map[{ .x = 1, .y = 1 }]
               .inner_if<explored>()
               .get_if<clear>() );
  REQUIRE( map[{ .x = 2, .y = 1 }] == unexplored{} );
  REQUIRE( map[{ .x = 3, .y = 1 }] == unexplored{} );
  REQUIRE( map[{ .x = 4, .y = 1 }] == unexplored{} );
}

TEST_CASE( "[on-map] non-interactive: removes LCR" ) {
  World W;
  W.create_default_map();
  point const kSrc  = { .x = 1, .y = 0 };
  point const kDst  = { .x = 0, .y = 1 };
  MapSquare& square = W.square( kDst );

  UnitId const unit_id =
      W.add_unit_on_map( e_unit_type::free_colonist, kSrc ).id();
  square.lost_city_rumor = true;

  // Before.
  REQUIRE( W.units().coord_for( unit_id ).to_gfx() == kSrc );
  REQUIRE( square.lost_city_rumor );

  TestingOnlyUnitOnMapMover::to_map_non_interactive(
      W.ss(), W.map_updater(), unit_id, { .x = 0, .y = 1 } );

  // After.
  REQUIRE( W.units().coord_for( unit_id ).to_gfx() == kDst );
  REQUIRE_FALSE( square.lost_city_rumor );
}

TEST_CASE(
    "[on-map] non-interactive: to_map_non_interactive "
    "unsentries surrounding units" ) {
  World W;
  W.create_default_map();
  Unit& unit1 =
      W.add_unit_on_map( e_unit_type::free_colonist,
                         { .x = 1, .y = 1 }, e_player::dutch );
  unit1.sentry();
  REQUIRE( unit1.orders().holds<unit_orders::sentry>() );
  Unit& unit2 =
      W.add_unit_on_map( e_unit_type::free_colonist,
                         { .x = 2, .y = 1 }, e_player::spanish );
  REQUIRE( unit1.orders().holds<unit_orders::none>() );
  REQUIRE( unit2.orders().holds<unit_orders::none>() );
}

TEST_CASE(
    "[on-map] non-interactive: "
    "native_unit_to_map_non_interactive unsentries surrounding "
    "units" ) {
  World W;
  W.create_default_map();
  Unit& unit1 =
      W.add_unit_on_map( e_unit_type::free_colonist,
                         { .x = 1, .y = 1 }, e_player::dutch );
  unit1.sentry();
  REQUIRE( unit1.orders().holds<unit_orders::sentry>() );
  Dwelling const& dwelling =
      W.add_dwelling( { .x = 2, .y = 1 }, e_tribe::cherokee );
  W.add_native_unit_on_map( e_native_unit_type::brave,
                            { .x = 2, .y = 0 }, dwelling.id );
  REQUIRE( unit1.orders().holds<unit_orders::none>() );
}

TEST_CASE(
    "[on-map] interactive: native_unit_to_map_interactive meets "
    "europeans" ) {
  World W;
  W.create_default_map();
  Dwelling const& dwelling =
      W.add_dwelling( { .x = 3, .y = 1 }, e_tribe::cherokee );
  NativeUnit const& native_unit = W.add_native_unit_on_map(
      e_native_unit_type::brave, { .x = 3, .y = 1 },
      dwelling.id );

  SECTION( "spanish" ) {
    // In this case the visibility will not need to change be-
    // cause we haven't set any active players, so therefore the
    // first human player in the order list of players (spanish)
    // will be considered as the viewer, and since the spanish
    // are being visited, there is no player change.
    W.spanish().control = e_player_control::human;
    W.dutch().control   = e_player_control::human;
    W.add_unit_on_map( e_unit_type::free_colonist,
                       { .x = 1, .y = 1 }, e_player::spanish );
    W.agent( e_player::spanish )
        .EXPECT__meet_tribe_ui_sequence(
            MeetTribe{ .player        = e_player::spanish,
                       .tribe         = e_tribe::cherokee,
                       .num_dwellings = 1 },
            point{ .x = 2, .y = 1 } )
        .returns<wait<e_declare_war_on_natives>>(
            e_declare_war_on_natives::no );
  }

  SECTION( "dutch" ) {
    // This one, unlike the one above, will need a visibility
    // change because the default viewer is again the spanish but
    // the dutch are being visited and they are humans.
    W.spanish().control = e_player_control::human;
    W.dutch().control   = e_player_control::human;
    W.add_unit_on_map( e_unit_type::free_colonist,
                       { .x = 1, .y = 1 }, e_player::dutch );
    W.agent( e_player::dutch )
        .EXPECT__meet_tribe_ui_sequence(
            MeetTribe{ .player        = e_player::dutch,
                       .tribe         = e_tribe::cherokee,
                       .num_dwellings = 1 },
            point{ .x = 2, .y = 1 } )
        .returns<wait<e_declare_war_on_natives>>(
            e_declare_war_on_natives::no );
    W.mock_land_view.EXPECT__set_visibility( e_player::dutch );
    W.mock_land_view.EXPECT__set_visibility( e_player::spanish );
  }

  wait<> const w =
      UnitOnMapMover::native_unit_to_map_interactive(
          W.ss(), W.ts(), native_unit.id, { .x = 2, .y = 1 } );
  REQUIRE( !w.exception() );
  REQUIRE( w.ready() );
}

TEST_CASE(
    "[on-map] non-interactive: "
    "native_unit_to_map_non_interactive performs inter-tribe "
    "trade" ) {
  World W;
  W.create_default_map();

  auto mv_unit = [&]( NativeUnit const& native_unit,
                      e_direction d ) {
    Coord const curr = W.units().coord_for( native_unit.id );
    Coord const dst  = curr.moved( d );
    UnitOnMapMover::native_unit_to_map_non_interactive(
        W.ss(), native_unit.id, dst );
  };

  Dwelling const& cherokee_dwelling =
      W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::cherokee );
  Dwelling const& aztec_dwelling =
      W.add_dwelling( { .x = 3, .y = 1 }, e_tribe::aztec );
  Dwelling const& iroquois_dwelling =
      W.add_dwelling( { .x = 5, .y = 1 }, e_tribe::iroquois );

  Tribe& cherokee = W.cherokee();
  Tribe& aztec    = W.aztec();
  Tribe& iroquois = W.iroquois();

  cherokee.horse_herds = 1;
  aztec.horse_herds    = 2;
  iroquois.horse_herds = 3;

  NativeUnit const& cherokee_brave = W.add_native_unit_on_map(
      e_native_unit_type::brave, { .x = 1, .y = 2 },
      cherokee_dwelling.id );
  NativeUnit const& aztec_brave = W.add_native_unit_on_map(
      e_native_unit_type::brave, { .x = 3, .y = 2 },
      aztec_dwelling.id );
  NativeUnit const& iroquois_brave = W.add_native_unit_on_map(
      e_native_unit_type::brave, { .x = 5, .y = 2 },
      iroquois_dwelling.id );

  REQUIRE( cherokee.horse_herds == 1 );
  REQUIRE( aztec.horse_herds == 2 );
  REQUIRE( iroquois.horse_herds == 3 );

  SECTION( "cherokee -> aztec dwelling" ) {
    mv_unit( cherokee_brave, e_direction::n );
    REQUIRE( cherokee.horse_herds == 1 );
    REQUIRE( aztec.horse_herds == 2 );
    REQUIRE( iroquois.horse_herds == 3 );

    mv_unit( cherokee_brave, e_direction::ne );
    REQUIRE( cherokee.horse_herds == 2 );
    REQUIRE( aztec.horse_herds == 2 );
    REQUIRE( iroquois.horse_herds == 3 );
  }

  SECTION( "cherokee -> aztec brave" ) {
    mv_unit( cherokee_brave, e_direction::se );
    REQUIRE( cherokee.horse_herds == 2 );
    REQUIRE( aztec.horse_herds == 2 );
    REQUIRE( iroquois.horse_herds == 3 );
  }

  SECTION( "aztec -> iroquois brave" ) {
    mv_unit( iroquois_brave, e_direction::w );
    REQUIRE( cherokee.horse_herds == 1 );
    REQUIRE( aztec.horse_herds == 3 );
    REQUIRE( iroquois.horse_herds == 3 );
  }

  SECTION( "aztec -> iroquois+cherokee braves" ) {
    mv_unit( cherokee_brave, e_direction::s );
    mv_unit( cherokee_brave, e_direction::se );
    mv_unit( iroquois_brave, e_direction::s );
    mv_unit( iroquois_brave, e_direction::sw );
    REQUIRE( cherokee.horse_herds == 1 );
    REQUIRE( aztec.horse_herds == 2 );
    REQUIRE( iroquois.horse_herds == 3 );

    // This one tests the case when we need two passes in order
    // to spread a larger number of horses (3) from a later tribe
    // (iroquois) to all three tribes. The iroquois are "later"
    // because they are to the south-east of the aztec, thus the
    // cherokee (1), which are to their south-west, come first
    // because of the ordering of the e_direction enum.
    static_assert( e_direction::sw < e_direction::se );
    mv_unit( aztec_brave, e_direction::s );
    REQUIRE( cherokee.horse_herds == 3 );
    REQUIRE( aztec.horse_herds == 3 );
    REQUIRE( iroquois.horse_herds == 3 );
  }
}

TEST_CASE(
    "[on-map] interactive: discovers new world on island" ) {
  World W;
  W.create_island_map();
  Player& player    = W.default_player();
  MockIAgent& agent = W.agent();
  UnitId const unit_id =
      W.add_unit_on_map( e_unit_type::treasure,
                         { .x = 1, .y = 1 } )
          .id();

  auto f = [&] {
    return co_await_test(
        TestingOnlyUnitOnMapMover::to_map_interactive(
            W.ss(), W.ts(), W.rand(), unit_id,
            { .x = 1, .y = 1 } ) );
  };

  REQUIRE( player.new_world_name == nothing );
  REQUIRE_FALSE(
      player.woodcuts[e_woodcut::discovered_new_world] );

  agent.EXPECT__show_woodcut( e_woodcut::discovered_new_world );
  agent.EXPECT__name_new_world().returns( "abc" );
  maybe<UnitDeleted> const unit_deleted = f();
  REQUIRE( unit_deleted == nothing );
  REQUIRE( W.units().coord_for( unit_id ) ==
           Coord{ .x = 1, .y = 1 } );
  REQUIRE( player.new_world_name == "abc" );
  REQUIRE( player.woodcuts[e_woodcut::discovered_new_world] );
}

} // namespace
} // namespace rn
