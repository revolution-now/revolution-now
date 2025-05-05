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
#include "test/mocks/ieuro-mind.hpp"
#include "test/mocks/igui.hpp"
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

using namespace std;

using ::gfx::point;
using ::mock::matchers::_;
using ::mock::matchers::StrContains;

using unexplored = PlayerSquare::unexplored;
using explored   = PlayerSquare::explored;
using clear      = FogStatus::clear;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() {
    add_player( e_nation::dutch );
    add_player( e_nation::spanish );
    set_default_player( e_nation::dutch );
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
        W.ss(), W.ts(), unit_id, { .x = 0, .y = 1 } );
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

#ifndef COMPILER_GCC
TEST_CASE( "[on-map] interactive: discovers new world" ) {
  World W;
  W.create_default_map();
  Player& player = W.default_player();
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
        W.ss(), W.ts(), unit_id, { .x = 0, .y = 1 } );
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    REQUIRE( *w == nothing );
    REQUIRE( W.units().coord_for( unit_id ) ==
             Coord{ .x = 0, .y = 1 } );
    REQUIRE( player.new_world_name == "my world" );
  }

  SECTION( "not yet discovered" ) {
    W.euro_mind().EXPECT__show_woodcut(
        e_woodcut::discovered_new_world );
    W.gui().EXPECT__string_input( _ ).returns<maybe<string>>(
        "my world 2" );
    w = TestingOnlyUnitOnMapMover::to_map_interactive(
        W.ss(), W.ts(), unit_id, { .x = 0, .y = 1 } );
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    REQUIRE( *w == nothing );
    REQUIRE( W.units().coord_for( unit_id ) ==
             Coord{ .x = 0, .y = 1 } );
    REQUIRE( player.new_world_name == "my world 2" );
  }
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

  W.euro_mind().EXPECT__show_woodcut(
      e_woodcut::discovered_pacific_ocean );
  w = TestingOnlyUnitOnMapMover::to_map_interactive(
      W.ss(), W.ts(), unit_id, { .x = 1, .y = 3 } );
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
      W.ss(), W.ts(), unit_id, { .x = 0, .y = 3 } );
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
  Player& player = W.default_player();
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
        W.ss(), W.ts(), unit_id, { .x = 0, .y = 1 } );
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    REQUIRE( *w == nothing );
    REQUIRE( W.units().coord_for( unit_id ) ==
             Coord{ .x = 0, .y = 1 } );
  }

  SECTION( "entering colony answer no" ) {
    W.gui().EXPECT__choice( _ ).returns<maybe<string>>( "no" );
    w = TestingOnlyUnitOnMapMover::to_map_interactive(
        W.ss(), W.ts(), unit_id, { .x = 1, .y = 1 } );
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    REQUIRE( *w == nothing );
    REQUIRE( W.units().coord_for( unit_id ) ==
             Coord{ .x = 1, .y = 1 } );
  }

  SECTION( "entering colony answer yes" ) {
    W.gui().EXPECT__choice( _ ).returns<maybe<string>>( "yes" );
    string const msg =
        "Treasure worth 1000\x7f arrives in Amsterdam!  The "
        "crown has provided a reimbursement of [500\x7f] after "
        "a [50%] witholding.";
    W.gui().EXPECT__message_box( msg ).returns( monostate{} );
    w = TestingOnlyUnitOnMapMover::to_map_interactive(
        W.ss(), W.ts(), unit_id, { .x = 1, .y = 1 } );
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    REQUIRE( w->has_value() );
    REQUIRE( !W.units().exists( unit_id ) );
  }
}
#endif

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
  player.new_world_name  = "my world";
  player.woodcuts[e_woodcut::discovered_new_world] = true;

  auto f = [&] {
    wait<maybe<UnitDeleted>> const w =
        TestingOnlyUnitOnMapMover::to_map_interactive(
            W.ss(), W.ts(), unit.id(), to );
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    REQUIRE( *w == nothing );
  };

  // Selects rumor result = fountain of youth.
  W.rand().EXPECT__between_ints( 0, 100 - 1 ).returns( 58 );
  W.euro_mind().EXPECT__show_woodcut(
      e_woodcut::discovered_fountain_of_youth );
  W.gui().EXPECT__message_box( StrContains( "Youth" ) );

  for( int i = 0; i < 8; ++i ) {
    // Pick immigrant.
    W.gui().EXPECT__choice( _ ).returns<maybe<string>>( "0" );
    // Replace with next immigrant.
    W.rand().EXPECT__between_doubles( _, _ ).returns( 0 );
    // Wait a bit.
    W.gui().EXPECT__wait_for( _ ).returns(
        chrono::microseconds{} );
  }

  f();
}

TEST_CASE( "[on-map] non-interactive: updates visibility" ) {
  World W;
  W.create_default_map();
  UNWRAP_CHECK( player_terrain, W.terrain().player_terrain(
                                    W.default_nation() ) );
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
      W.ss(), W.ts(), unit.id(), { .x = 0, .y = 1 } );
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

TEST_CASE(
    "[on-map] non-interactive: to_map_non_interactive "
    "unsentries surrounding units" ) {
  World W;
  W.create_default_map();
  Unit& unit1 =
      W.add_unit_on_map( e_unit_type::free_colonist,
                         { .x = 1, .y = 1 }, e_nation::dutch );
  unit1.sentry();
  REQUIRE( unit1.orders().holds<unit_orders::sentry>() );
  Unit& unit2 =
      W.add_unit_on_map( e_unit_type::free_colonist,
                         { .x = 2, .y = 1 }, e_nation::spanish );
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
                         { .x = 1, .y = 1 }, e_nation::dutch );
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
  MockLandViewPlane mock_land_view;
  W.planes().get().set_bottom<ILandViewPlane>( mock_land_view );
  W.create_default_map();
  Dwelling const& dwelling =
      W.add_dwelling( { .x = 3, .y = 1 }, e_tribe::cherokee );
  NativeUnit const& native_unit = W.add_native_unit_on_map(
      e_native_unit_type::brave, { .x = 3, .y = 1 },
      dwelling.id );

  SECTION( "spanish" ) {
    // In this case the visibility will not need to change be-
    // cause we haven't set any active nations, so therefore the
    // first human nation in the order list of nations (spanish)
    // will be considered as the viewer, and since the spanish
    // are being visited, there is no nation change.
    W.spanish().human = true;
    W.dutch().human   = true;
    W.add_unit_on_map( e_unit_type::free_colonist,
                       { .x = 1, .y = 1 }, e_nation::spanish );
    W.euro_mind( e_nation::spanish )
        .EXPECT__meet_tribe_ui_sequence(
            MeetTribe{ .nation        = e_nation::spanish,
                       .tribe         = e_tribe::cherokee,
                       .num_dwellings = 1 } )
        .returns<wait<e_declare_war_on_natives>>(
            e_declare_war_on_natives::no );
  }

  SECTION( "dutch" ) {
    // This one, unlike the one above, will need a visibility
    // change because the default viewer is again the spanish but
    // the dutch are being visited and they are humans.
    W.spanish().human = true;
    W.dutch().human   = true;
    W.add_unit_on_map( e_unit_type::free_colonist,
                       { .x = 1, .y = 1 }, e_nation::dutch );
    W.euro_mind( e_nation::dutch )
        .EXPECT__meet_tribe_ui_sequence(
            MeetTribe{ .nation        = e_nation::dutch,
                       .tribe         = e_tribe::cherokee,
                       .num_dwellings = 1 } )
        .returns<wait<e_declare_war_on_natives>>(
            e_declare_war_on_natives::no );
    mock_land_view.EXPECT__set_visibility( e_nation::dutch );
    mock_land_view.EXPECT__set_visibility( e_nation::spanish );
  }

  mock_land_view.EXPECT__ensure_visible(
      point{ .x = 2, .y = 1 } );
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
  Player& player = W.default_player();
  UnitId const unit_id =
      W.add_unit_on_map( e_unit_type::treasure,
                         { .x = 1, .y = 1 } )
          .id();

  auto f = [&] {
    return co_await_test(
        TestingOnlyUnitOnMapMover::to_map_interactive(
            W.ss(), W.ts(), unit_id, { .x = 1, .y = 1 } ) );
  };

  REQUIRE( player.new_world_name == nothing );
  REQUIRE_FALSE(
      player.woodcuts[e_woodcut::discovered_new_world] );

  W.euro_mind().EXPECT__show_woodcut(
      e_woodcut::discovered_new_world );
  W.gui().EXPECT__string_input( _ ).returns<maybe<string>>(
      "abc" );
  maybe<UnitDeleted> const unit_deleted = f();
  REQUIRE( unit_deleted == nothing );
  REQUIRE( W.units().coord_for( unit_id ) ==
           Coord{ .x = 1, .y = 1 } );
  REQUIRE( player.new_world_name == "abc" );
  REQUIRE( player.woodcuts[e_woodcut::discovered_new_world] );
}

} // namespace
} // namespace rn
