/****************************************************************
**colony-mgr.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-01-01.
*
* Description: Unit tests for colony-mgr module.
*
*****************************************************************/
#include "testing.hpp"

// Testing
#include "test/fake/world.hpp"
#include "test/mocks/iagent.hpp"
#include "test/mocks/land-view-plane.hpp"
#include "test/util/coro.hpp"

// Revolution Now
#include "src/colony-mgr.hpp"
#include "src/connectivity.rds.hpp"
#include "src/harbor-units.hpp"
#include "src/imap-updater.hpp"
#include "src/map-square.hpp"
#include "src/plane-stack.hpp"
#include "src/unit-mgr.hpp"
#include "src/unit-ownership.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/player.rds.hpp"
#include "ss/players.rds.hpp"
#include "ss/ref.hpp"
#include "ss/terrain.hpp"
#include "ss/units.hpp"

// refl
#include "refl/to-str.hpp"

// Must be last.
#include "catch-common.hpp"

namespace rn {
namespace {

using namespace std;
using namespace rn;

using ::Catch::UnorderedEquals;

using ::gfx::point;
using ::mock::matchers::_;
using ::refl::enum_map;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct world : testing::World {
  using Base = testing::World;
  world() : Base() {
    add_player( e_player::dutch );
    set_default_player_type( e_player::dutch );
    add_player( e_player::english );
    create_default_map();
  }

  void create_default_map() {
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
    // clang-format off
    vector<MapSquare> tiles{
    // 0  1  2  3  4  5  6  7  8
       _, L, _, L, L, L, L, L, _, // 0
       L, L, L, L, L, L, _, L, _, // 1
       _, L, L, L, L, L, L, L, _, // 2
       _, L, _, L, L, L, L, _, _, // 3
       _, L, L, L, L, L, L, L, _, // 4
       L, L, L, L, L, L, L, L, _, // 5
    };
    // clang-format on
    build_map( std::move( tiles ), 9 );
  }

  void create_disconnected_map() {
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
    // clang-format off
    vector<MapSquare> tiles{
    // 0  1  2  3  4  5  6  7  8
       _, L, _, L, _, L, L, L, _, // 0
       L, L, L, L, _, L, _, L, _, // 1
       _, L, L, L, _, L, L, L, _, // 2
       _, _, _, _, _, _, _, _, _, // 3
       _, L, L, L, _, L, L, L, _, // 4
       L, L, L, L, _, L, L, L, _, // 5
    };
    // clang-format on
    build_map( std::move( tiles ), 9 );
  }

  void create_island_map() {
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
    // clang-format off
    vector<MapSquare> tiles{
    // 0  1  2  3  4
       _, _, _, L, _, // 0
       _, L, _, _, _, // 1
       _, _, _, _, _, // 2
       _, _, _, L, _, // 3
       L, _, _, L, _, // 4
    };
    // clang-format on
    build_map( std::move( tiles ), 5 );
  }
};

TEST_CASE( "[colony-mgr] found_colony on land successful" ) {
  world W;

  Coord const coord = { .x = 1, .y = 1 };
  Unit& unit =
      W.add_unit_on_map( e_unit_type::free_colonist, coord );
  REQUIRE( unit_can_found_colony( W.ss(), unit.id() ) ==
           base::valid );
  ColonyId col_id = found_colony(
      W.ss(), W.ts(), W.default_player(), unit.id(), "colony" );
  Colony& col = W.colonies().colony_for( col_id );
  for( auto [type, q] : col.commodities ) {
    INFO( fmt::format( "type: {}, q: {}", type, q ) );
    REQUIRE( q == 0 );
  }
}

TEST_CASE( "[colony-mgr] found_colony during war fails" ) {
  world W;
  W.default_player().revolution.status =
      e_revolution_status::declared;
  Coord const coord = { .x = 1, .y = 1 };
  Unit& unit =
      W.add_unit_on_map( e_unit_type::free_colonist, coord );
  REQUIRE( unit_can_found_colony( W.ss(), unit.id() ) ==
           e_found_colony_err::war_of_independence );
}

TEST_CASE( "[colony-mgr] found_colony on island" ) {
  world w;
  w.create_island_map();
  {
    point const tile = { .x = 1, .y = 1 };
    UnitId const unit_id =
        w.add_unit_on_map( e_unit_type::free_colonist, tile )
            .id();
    REQUIRE( unit_can_found_colony( w.ss(), unit_id ) ==
             e_found_colony_err::no_island_colony );
  }
  {
    point const tile = { .x = 0, .y = 4 };
    UnitId const unit_id =
        w.add_unit_on_map( e_unit_type::free_colonist, tile )
            .id();
    REQUIRE( unit_can_found_colony( w.ss(), unit_id ) ==
             e_found_colony_err::no_island_colony );
  }
  {
    point const tile = { .x = 3, .y = 0 };
    UnitId const unit_id =
        w.add_unit_on_map( e_unit_type::free_colonist, tile )
            .id();
    REQUIRE( unit_can_found_colony( w.ss(), unit_id ) ==
             e_found_colony_err::no_island_colony );
  }
  {
    point const tile = { .x = 3, .y = 4 };
    UnitId const unit_id =
        w.add_unit_on_map( e_unit_type::free_colonist, tile )
            .id();
    REQUIRE( unit_can_found_colony( w.ss(), unit_id ) == valid );
  }
}

TEST_CASE( "[colony-mgr] native convert cannot found" ) {
  world W;

  Coord const coord = { .x = 1, .y = 1 };
  Unit& unit =
      W.add_unit_on_map( e_unit_type::native_convert, coord );
  REQUIRE( unit_can_found_colony( W.ss(), unit.id() ) ==
           e_found_colony_err::native_convert_cannot_found );
}

TEST_CASE( "[colony-mgr] found_colony strips unit" ) {
  world W;

  SECTION( "dragoon" ) {
    Coord const coord = { .x = 1, .y = 1 };
    Unit& founder     = W.add_unit_on_map(
        UnitType::create( e_unit_type::dragoon,
                              e_unit_type::petty_criminal )
            .value(),
        coord );
    REQUIRE( founder.type() == e_unit_type::dragoon );
    REQUIRE(
        unit_can_found_colony( W.ss(), founder.id() ).valid() );
    ColonyId col_id =
        found_colony( W.ss(), W.ts(), W.default_player(),
                      founder.id(), "colony" );
    REQUIRE( founder.type() == e_unit_type::petty_criminal );
    Colony& col = W.colonies().colony_for( col_id );
    // Make sure that the founding unit has shed all of its com-
    // modities into the colony commodity store.
    for( auto [type, q] : col.commodities ) {
      INFO( fmt::format( "type: {}, q: {}", type, q ) );
      switch( type ) {
        case e_commodity::horses:
          REQUIRE( q == 50 );
          break;
        case e_commodity::muskets:
          REQUIRE( q == 50 );
          break;
        default:
          REQUIRE( q == 0 );
          break;
      }
    }
  }

  SECTION( "hardy_pioneer" ) {
    Coord const coord = { .x = 1, .y = 1 };
    Unit& founder =
        W.add_unit_on_map( e_unit_type::hardy_pioneer, coord );
    REQUIRE( founder.type() == e_unit_type::hardy_pioneer );
    REQUIRE(
        unit_can_found_colony( W.ss(), founder.id() ).valid() );
    ColonyId col_id =
        found_colony( W.ss(), W.ts(), W.default_player(),
                      founder.id(), "colony" );
    REQUIRE( founder.type() == e_unit_type::hardy_colonist );
    Colony& col = W.colonies().colony_for( col_id );
    // Make sure that the founding unit has shed all of its com-
    // modities into the colony commodity store.
    for( auto [type, q] : col.commodities ) {
      INFO( fmt::format( "type: {}, q: {}", type, q ) );
      switch( type ) {
        case e_commodity::tools:
          REQUIRE( q == 100 );
          break;
        default:
          REQUIRE( q == 0 );
          break;
      }
    }
  }
}

TEST_CASE(
    "[colony-mgr] found_colony on existing colony fails" ) {
  world W;

  Coord const coord = { .x = 1, .y = 1 };
  UnitId id =
      W.add_unit_on_map( e_unit_type::free_colonist, coord )
          .id();
  REQUIRE( unit_can_found_colony( W.ss(), id ).valid() );
  ColonyId col_id = found_colony(
      W.ss(), W.ts(), W.default_player(), id, "colony 1" );
  Colony& col = W.colonies().colony_for( col_id );
  for( auto [type, q] : col.commodities ) {
    INFO( fmt::format( "type: {}, q: {}", type, q ) );
    REQUIRE( q == 0 );
  }

  id = W.add_unit_on_map( e_unit_type::free_colonist, coord )
           .id();
  REQUIRE( unit_can_found_colony( W.ss(), id ) ==
           invalid( e_found_colony_err::colony_exists_here ) );
}

TEST_CASE(
    "[colony-mgr] found_colony too close to another colony "
    "fails" ) {
  world W;

  Coord coord = { .x = 1, .y = 1 };
  UnitId id =
      W.add_unit_on_map( e_unit_type::free_colonist, coord )
          .id();
  REQUIRE( unit_can_found_colony( W.ss(), id ).valid() );
  found_colony( W.ss(), W.ts(), W.default_player(), id,
                "colony" );
  coord.x += 1;
  id = W.add_unit_on_map( e_unit_type::free_colonist, coord )
           .id();
  REQUIRE( unit_can_found_colony( W.ss(), id ) ==
           invalid( e_found_colony_err::too_close_to_colony ) );
}

TEST_CASE( "[colony-mgr] can't build colony in water" ) {
  world W;

  Coord const coord = { .x = 2, .y = 3 };
  CHECK( W.square( coord ).surface == e_surface::water );
  UnitId ship_id =
      W.add_unit_on_map( e_unit_type::merchantman, coord ).id();
  UnitId unit_id =
      create_free_unit( W.units(), W.default_player(),
                        e_unit_type::free_colonist );
  UnitOwnershipChanger( W.ss(), unit_id )
      .change_to_cargo( ship_id, /*starting_slot=*/0 );
  REQUIRE( unit_can_found_colony( W.ss(), unit_id ) ==
           invalid( e_found_colony_err::no_water_colony ) );
}

TEST_CASE( "[colony-mgr] can't build colony on moutains" ) {
  world W;

  Coord const coord = { .x = 1, .y = 1 };
  CHECK( W.square( coord ).surface == e_surface::land );
  W.square( coord ).overlay = e_land_overlay::mountains;
  UnitId unit_id =
      W.add_unit_on_map( e_unit_type::free_colonist, coord )
          .id();
  REQUIRE( unit_can_found_colony( W.ss(), unit_id ) ==
           invalid( e_found_colony_err::no_mountain_colony ) );
}

TEST_CASE(
    "[colony-mgr] found_colony by unit not on map fails" ) {
  world W;

  UnitId id = create_free_unit( W.units(), W.default_player(),
                                e_unit_type::free_colonist );
  unit_move_to_port( W.ss(), id );
  REQUIRE( unit_can_found_colony( W.ss(), id ) ==
           invalid( e_found_colony_err::colonist_not_on_map ) );
}

TEST_CASE( "[colony-mgr] found_colony by ship fails" ) {
  world W;

  Coord const coord = { .x = 2, .y = 3 };
  CHECK( W.square( coord ).surface == e_surface::water );
  UnitId id =
      W.add_unit_on_map( e_unit_type::merchantman, coord ).id();
  REQUIRE(
      unit_can_found_colony( W.ss(), id ) ==
      invalid( e_found_colony_err::ship_cannot_found_colony ) );
}

TEST_CASE( "[colony-mgr] found_colony by non-colonist fails" ) {
  world W;

  Coord const coord = { .x = 1, .y = 1 };
  UnitId id =
      W.add_unit_on_map( e_unit_type::wagon_train, coord ).id();
  REQUIRE( unit_can_found_colony( W.ss(), id ) ==
           invalid( e_found_colony_err::
                        non_colonist_cannot_found_colony ) );
}

TEST_CASE( "[colony-mgr] create, query, destroy" ) {
  world W;
  Colony& colony =
      W.add_colony( Coord{ .x = 1, .y = 2 }, e_player::english );
  REQUIRE( colony.id == ColonyId{ 1 } );

  REQUIRE( colony.id == ColonyId{ 1 } );
  REQUIRE( colony.player == e_player::english );
  REQUIRE( colony.name == "1" );
  REQUIRE( colony.location == Coord{ .x = 1, .y = 2 } );

  // These will check-fail if they don't exist.
  REQUIRE( W.colonies().exists( ColonyId{ 1 } ) );

  Colony& colony2 =
      W.add_colony( Coord{ .x = 1, .y = 3 }, e_player::dutch );
  REQUIRE( colony2.id == 2 );
  REQUIRE( W.colonies().exists( ColonyId{ 1 } ) );
  REQUIRE( W.colonies().exists( ColonyId{ 2 } ) );
  REQUIRE( W.colonies().all().size() == 2 );
  REQUIRE( W.colonies().all().contains( 1 ) );
  REQUIRE( W.colonies().all().contains( 2 ) );
  REQUIRE_THAT(
      W.colonies().for_player( e_player::dutch ),
      UnorderedEquals( vector<ColonyId>{ ColonyId{ 2 } } ) );
  REQUIRE_THAT(
      W.colonies().for_player( e_player::english ),
      UnorderedEquals( vector<ColonyId>{ ColonyId{ 1 } } ) );
  REQUIRE_THAT( W.colonies().for_player( e_player::french ),
                UnorderedEquals( vector<ColonyId>{} ) );

  REQUIRE( W.colonies().maybe_from_name( "1" ) ==
           ColonyId{ 1 } );
  W.colonies().destroy_colony( ColonyId{ 1 } );
  REQUIRE( W.colonies().all().size() == 1 );
  REQUIRE( W.colonies().all().contains( 2 ) );

  W.colonies().destroy_colony( ColonyId{ 2 } );
  REQUIRE( W.colonies().all().size() == 0 );
}

TEST_CASE( "[colony-mgr] initial colony buildings." ) {
  world W;
  auto [colony, founder] =
      W.found_colony_with_new_unit( Coord{ .x = 1, .y = 1 } );
  unordered_set<e_colony_building> buildings;
  for( auto const& [building, has] : colony.buildings )
    if( has ) buildings.insert( building );
  REQUIRE( buildings ==
           unordered_set<e_colony_building>{
             e_colony_building::blacksmiths_house,
             e_colony_building::carpenters_shop,
             e_colony_building::fur_traders_house,
             e_colony_building::rum_distillers_house,
             e_colony_building::tobacconists_house,
             e_colony_building::weavers_house,
             e_colony_building::town_hall,
           } );
}

TEST_CASE( "[colony-mgr] found_colony places initial unit." ) {
  world W;

  Unit& founder = W.add_unit_on_map( e_unit_type::free_colonist,
                                     Coord{ .x = 1, .y = 1 } );
  // Don't use W.add_colony here because we are testing
  // found_colony specifically.
  ColonyId id = found_colony( W.ss(), W.ts(), W.default_player(),
                              founder.id(), "my colony" );
  Colony& colony = W.colonies().colony_for( id );

  REQUIRE( colony.outdoor_jobs[e_direction::nw] == nothing );
  REQUIRE( colony.outdoor_jobs[e_direction::ne] == nothing );
  REQUIRE( colony.outdoor_jobs[e_direction::w] == nothing );
  REQUIRE( colony.outdoor_jobs[e_direction::e] == nothing );
  REQUIRE( colony.outdoor_jobs[e_direction::sw] == nothing );
  REQUIRE( colony.outdoor_jobs[e_direction::s] == nothing );
  REQUIRE( colony.outdoor_jobs[e_direction::se] == nothing );

  // Colonist should have been placed here.
  REQUIRE( colony.outdoor_jobs[e_direction::n] ==
           ( OutdoorUnit{ .unit_id = founder.id(),
                          .job     = e_outdoor_job::food } ) );
}

TEST_CASE( "[colony-mgr] change_unit_outdoor_job." ) {
  world W;
  Colony& colony = W.add_colony( Coord{ .x = 1, .y = 1 } );
  // Note that the founding colonist will have been placed on the
  // north tile.
  Unit& farmer = W.add_unit_outdoors( colony.id, e_direction::w,
                                      e_outdoor_job::food );
  Unit& ore_miner = W.add_unit_outdoors(
      colony.id, e_direction::e, e_outdoor_job::ore );

  // Sanity check.
  REQUIRE( colony.outdoor_jobs[e_direction::w] ==
           ( OutdoorUnit{ .unit_id = farmer.id(),
                          .job     = e_outdoor_job::food } ) );
  REQUIRE( colony.outdoor_jobs[e_direction::e] ==
           ( OutdoorUnit{ .unit_id = ore_miner.id(),
                          .job     = e_outdoor_job::ore } ) );

  // Change job.
  change_unit_outdoor_job( colony, farmer.id(),
                           e_outdoor_job::lumber );

  REQUIRE( colony.outdoor_jobs[e_direction::w] ==
           ( OutdoorUnit{ .unit_id = farmer.id(),
                          .job     = e_outdoor_job::lumber } ) );
  REQUIRE( colony.outdoor_jobs[e_direction::e] ==
           ( OutdoorUnit{ .unit_id = ore_miner.id(),
                          .job     = e_outdoor_job::ore } ) );
}

TEST_CASE( "[colony-mgr] colony destruction" ) {
  world W;
  Coord const loc        = { .x = 1, .y = 1 };
  auto [colony, founder] = W.found_colony_with_new_unit( loc );
  vector<UnitId> const units = colony_units_all( colony );
  REQUIRE( units.size() == 1 );
  UnitId const founder_id                   = founder.id();
  e_ship_damaged_reason ship_damaged_reason = {};

  REQUIRE( W.units().exists( founder_id ) );
  REQUIRE( W.colonies().all().size() == 1 );
  REQUIRE( W.terrain().square_at( loc ).road );

  SECTION( "non interactive" ) {
    destroy_colony( W.ss(), W.ts(), colony );
  }

  SECTION( "non interactive with ships" ) {
    Unit const& ship1 =
        W.add_unit_on_map( e_unit_type::caravel, loc );
    Unit const& ship2 =
        W.add_unit_on_map( e_unit_type::merchantman, loc );
    Unit const& ship3 =
        W.add_unit_on_map( e_unit_type::caravel, loc );
    destroy_colony( W.ss(), W.ts(), colony );
    REQUIRE( W.units().exists( ship1.id() ) );
    REQUIRE( W.units().exists( ship2.id() ) );
    REQUIRE( W.units().exists( ship3.id() ) );
    REQUIRE( as_const( W.units() ).ownership_of( ship1.id() ) ==
             UnitOwnership::harbor{
               .port_status = PortStatus::in_port{} } );
    REQUIRE( as_const( W.units() ).ownership_of( ship2.id() ) ==
             UnitOwnership::harbor{
               .port_status = PortStatus::in_port{} } );
    REQUIRE( as_const( W.units() ).ownership_of( ship3.id() ) ==
             UnitOwnership::harbor{
               .port_status = PortStatus::in_port{} } );
  }

  // This will make sure that sure that units on ships will be
  // offboarded before the ships are marked as damaged.
  SECTION( "non interactive with ships containing units" ) {
    Unit const& ship1 =
        W.add_unit_on_map( e_unit_type::caravel, loc );
    Unit const& ship2 =
        W.add_unit_on_map( e_unit_type::merchantman, loc );
    Unit const& ship3 =
        W.add_unit_on_map( e_unit_type::caravel, loc );
    Unit const& free_colonist = W.add_unit_in_cargo(
        e_unit_type::free_colonist, ship1.id() );
    Unit const& soldier =
        W.add_unit_in_cargo( e_unit_type::soldier, ship2.id() );
    UnitId const ship1_id         = ship1.id();
    UnitId const ship2_id         = ship2.id();
    UnitId const ship3_id         = ship3.id();
    UnitId const free_colonist_id = free_colonist.id();
    UnitId const soldier_id       = soldier.id();
    destroy_colony( W.ss(), W.ts(), colony );
    REQUIRE( W.units().exists( ship1_id ) );
    REQUIRE( W.units().exists( ship2_id ) );
    REQUIRE( W.units().exists( ship3_id ) );
    REQUIRE( W.units().exists( free_colonist_id ) );
    REQUIRE( W.units().exists( soldier_id ) );
    REQUIRE( as_const( W.units() ).ownership_of( ship1.id() ) ==
             UnitOwnership::harbor{
               .port_status = PortStatus::in_port{} } );
    REQUIRE( as_const( W.units() ).ownership_of( ship2.id() ) ==
             UnitOwnership::harbor{
               .port_status = PortStatus::in_port{} } );
    REQUIRE( as_const( W.units() ).ownership_of( ship3.id() ) ==
             UnitOwnership::harbor{
               .port_status = PortStatus::in_port{} } );
    REQUIRE(
        as_const( W.units() ).ownership_of( free_colonist_id ) ==
        UnitOwnership::world{ .coord = loc } );
    REQUIRE( as_const( W.units() ).ownership_of( soldier_id ) ==
             UnitOwnership::world{ .coord = loc } );
  }

  SECTION( "non interactive with ships, post-revolution" ) {
    W.default_player().revolution.status =
        e_revolution_status::declared;
    Unit const& ship1 =
        W.add_unit_on_map( e_unit_type::caravel, loc );
    Unit const& ship2 =
        W.add_unit_on_map( e_unit_type::merchantman, loc );
    Unit const& ship3 =
        W.add_unit_on_map( e_unit_type::caravel, loc );
    UnitId const ship1_id = ship1.id();
    UnitId const ship2_id = ship2.id();
    UnitId const ship3_id = ship3.id();
    destroy_colony( W.ss(), W.ts(), colony );
    REQUIRE_FALSE( W.units().exists( ship1_id ) );
    REQUIRE_FALSE( W.units().exists( ship2_id ) );
    REQUIRE_FALSE( W.units().exists( ship3_id ) );
  }

  SECTION( "interactive" ) {
    MockLandViewPlane mock_land_view;
    W.planes().get().set_bottom<ILandViewPlane>(
        mock_land_view );

    mock_land_view.EXPECT__animate_if_visible( _ );
    W.agent()
        .EXPECT__message_box( "some msg" )
        .returns( make_wait<>() );

    co_await_test( run_animated_colony_destruction(
        W.ss(), W.ts(), colony, ship_damaged_reason,
        /*msg=*/"some msg" ) );
  }

  SECTION( "interactive no animation" ) {
    // We don't technically need this here, but it makes the
    // error messages better if the code incorrectly tries to an-
    // imate something.
    MockLandViewPlane mock_land_view;
    W.planes().get().set_bottom<ILandViewPlane>(
        mock_land_view );

    W.agent()
        .EXPECT__message_box( "some msg" )
        .returns( make_wait<>() );

    co_await_test( run_colony_destruction(
        W.ss(), W.ts(), colony, ship_damaged_reason,
        /*msg=*/"some msg" ) );
  }

  SECTION( "interactive with ship" ) {
    Unit const& ship1 =
        W.add_unit_on_map( e_unit_type::caravel, loc );
    Unit const& ship2 =
        W.add_unit_on_map( e_unit_type::merchantman, loc );
    Unit const& ship3 =
        W.add_unit_on_map( e_unit_type::caravel, loc );

    MockLandViewPlane mock_land_view;
    W.planes().get().set_bottom<ILandViewPlane>(
        mock_land_view );

    mock_land_view.EXPECT__animate_if_visible( _ );

    SECTION( "reason=battle" ) {
      ship_damaged_reason = e_ship_damaged_reason::battle;
      W.agent()
          .EXPECT__message_box( "some msg" )
          .returns( make_wait<>() );
      W.agent()
          .EXPECT__message_box(
              "Port in [1] contained two [Caravels] that were "
              "damaged in battle and were sent to [Amsterdam] "
              "for repairs." )
          .returns( make_wait<>() );
      W.agent()
          .EXPECT__message_box(
              "Port in [1] contained one [Merchantman] that was "
              "damaged in battle and was sent to [Amsterdam] "
              "for repairs." )
          .returns( make_wait<>() );
    }

    SECTION( "reason=colony_abandoned" ) {
      ship_damaged_reason =
          e_ship_damaged_reason::colony_abandoned;
      W.agent()
          .EXPECT__message_box( "some msg" )
          .returns( make_wait<>() );
      W.agent()
          .EXPECT__message_box(
              "Port in [1] contained two [Caravels] that were "
              "damaged during colony collapse and were sent "
              "to [Amsterdam] for repairs." )
          .returns( make_wait<>() );
      W.agent()
          .EXPECT__message_box(
              "Port in [1] contained one [Merchantman] that was "
              "damaged during colony collapse and was sent to "
              "[Amsterdam] for repairs." )
          .returns( make_wait<>() );
    }

    SECTION( "reason=colony_starved" ) {
      ship_damaged_reason =
          e_ship_damaged_reason::colony_starved;
      W.agent()
          .EXPECT__message_box( "some msg" )
          .returns( make_wait<>() );
      W.agent()
          .EXPECT__message_box(
              "Port in [1] contained two [Caravels] that were "
              "damaged during colony collapse and were sent "
              "to [Amsterdam] for repairs." )
          .returns( make_wait<>() );
      W.agent()
          .EXPECT__message_box(
              "Port in [1] contained one [Merchantman] that was "
              "damaged during colony collapse and was sent to "
              "[Amsterdam] for repairs." )
          .returns( make_wait<>() );
    }

    co_await_test( run_animated_colony_destruction(
        W.ss(), W.ts(), colony, ship_damaged_reason,
        /*msg=*/"some msg" ) );

    REQUIRE( W.units().exists( ship1.id() ) );
    REQUIRE( W.units().exists( ship2.id() ) );
    REQUIRE( W.units().exists( ship3.id() ) );
    REQUIRE( as_const( W.units() ).ownership_of( ship1.id() ) ==
             UnitOwnership::harbor{
               .port_status = PortStatus::in_port{} } );
    REQUIRE( as_const( W.units() ).ownership_of( ship2.id() ) ==
             UnitOwnership::harbor{
               .port_status = PortStatus::in_port{} } );
    REQUIRE( as_const( W.units() ).ownership_of( ship3.id() ) ==
             UnitOwnership::harbor{
               .port_status = PortStatus::in_port{} } );
  }

  SECTION( "interactive with ship, post-revolution" ) {
    W.default_player().revolution.status =
        e_revolution_status::declared;
    Unit const& ship1 =
        W.add_unit_on_map( e_unit_type::caravel, loc );
    Unit const& ship2 =
        W.add_unit_on_map( e_unit_type::merchantman, loc );
    Unit const& ship3 =
        W.add_unit_on_map( e_unit_type::caravel, loc );
    UnitId const ship1_id = ship1.id();
    UnitId const ship2_id = ship2.id();
    UnitId const ship3_id = ship3.id();

    MockLandViewPlane mock_land_view;
    W.planes().get().set_bottom<ILandViewPlane>(
        mock_land_view );

    mock_land_view.EXPECT__animate_if_visible( _ );

    ship_damaged_reason = e_ship_damaged_reason::battle;
    W.agent()
        .EXPECT__message_box( "some msg" )
        .returns( make_wait<>() );
    W.agent()
        .EXPECT__message_box(
            "Port in [1] contained two [Caravels] that were "
            "damaged in battle and destroyed as there are no "
            "available ports for repair." )
        .returns( make_wait<>() );
    W.agent()
        .EXPECT__message_box(
            "Port in [1] contained one [Merchantman] that was "
            "damaged in battle and destroyed as there are no "
            "available ports for repair." )
        .returns( make_wait<>() );

    co_await_test( run_animated_colony_destruction(
        W.ss(), W.ts(), colony, ship_damaged_reason,
        /*msg=*/"some msg" ) );

    REQUIRE_FALSE( W.units().exists( ship1_id ) );
    REQUIRE_FALSE( W.units().exists( ship2_id ) );
    REQUIRE_FALSE( W.units().exists( ship3_id ) );
  }

  // !! Do not access colony after this point.

  REQUIRE( !W.units().exists( founder_id ) );
  REQUIRE( W.colonies().all().size() == 0 );
  REQUIRE( !W.terrain().square_at( loc ).road );
}

TEST_CASE(
    "[colony-mgr] find_occupied_surrounding_colony_squares" ) {
  world W;
  Colony& colony_nw = W.add_colony( Coord{ .x = 1, .y = 1 } );
  Colony& colony_ne = W.add_colony( Coord{ .x = 5, .y = 1 } );
  Colony& colony    = W.add_colony( Coord{ .x = 3, .y = 3 } );
  Colony& colony_sw = W.add_colony( Coord{ .x = 1, .y = 5 } );
  Colony& colony_se = W.add_colony( Coord{ .x = 5, .y = 5 } );

  Colony* colonies[] = { &colony_nw, &colony_ne, &colony_sw,
                         &colony_se };

  // Populate workers in the colonies.
  for( Colony* p_colony : colonies )
    for( e_direction d : refl::enum_values<e_direction> )
      p_colony->outdoor_jobs[d].emplace();
  // Remove one.
  colony_se.outdoor_jobs[e_direction::nw] = nothing;

  refl::enum_map<e_direction, bool> const& occupied_red_box =
      find_occupied_surrounding_colony_squares( W.ss(), colony );

  refl::enum_map<e_direction, bool> const expected{
    { e_direction::nw, true }, { e_direction::n, false },
    { e_direction::ne, true }, { e_direction::w, false },
    { e_direction::e, false }, { e_direction::sw, true },
    { e_direction::s, false }, { e_direction::se, false },
  };

  REQUIRE( occupied_red_box == expected );
}

TEST_CASE( "[colony-mgr] give_stockade_if_needed" ) {
  world W;
  Player& dutch   = W.dutch();
  Player& english = W.english();
  // _, L, _, L, L, L,
  // L, L, L, L, L, L,
  // _, L, L, L, L, L,
  // _, L, _, L, L, L,
  // _, L, L, L, L, L,
  // L, L, L, L, L, L,
  auto [dutch1, dutch1_founder] = W.found_colony_with_new_unit(
      { .x = 1, .y = 1 }, e_player::dutch );
  auto [dutch2, dutch2_founder] = W.found_colony_with_new_unit(
      { .x = 1, .y = 3 }, e_player::dutch );
  auto [english1, english1_founder] =
      W.found_colony_with_new_unit( { .x = 3, .y = 1 },
                                    e_player::english );
  W.add_unit_indoors( english1.id, e_indoor_job::bells );
  W.add_unit_indoors( english1.id, e_indoor_job::bells );
  W.add_unit_indoors( english1.id, e_indoor_job::bells );
  english.fathers.has[e_founding_father::sieur_de_la_salle] =
      true;

  // Sanity check.
  REQUIRE_FALSE( dutch1.buildings[e_colony_building::stockade] );
  REQUIRE_FALSE( dutch2.buildings[e_colony_building::stockade] );
  REQUIRE_FALSE(
      english1.buildings[e_colony_building::stockade] );

  give_stockade_if_needed( dutch, dutch1 );
  give_stockade_if_needed( dutch, dutch2 );
  REQUIRE_FALSE( dutch1.buildings[e_colony_building::stockade] );
  REQUIRE_FALSE( dutch2.buildings[e_colony_building::stockade] );
  REQUIRE_FALSE(
      english1.buildings[e_colony_building::stockade] );

  dutch.fathers.has[e_founding_father::sieur_de_la_salle] = true;
  give_stockade_if_needed( dutch, dutch1 );
  give_stockade_if_needed( dutch, dutch2 );
  REQUIRE_FALSE( dutch1.buildings[e_colony_building::stockade] );
  REQUIRE_FALSE( dutch2.buildings[e_colony_building::stockade] );
  REQUIRE_FALSE(
      english1.buildings[e_colony_building::stockade] );

  dutch.fathers.has[e_founding_father::sieur_de_la_salle] =
      false;
  W.add_unit_indoors( dutch1.id, e_indoor_job::bells );
  give_stockade_if_needed( dutch, dutch1 );
  give_stockade_if_needed( dutch, dutch2 );
  REQUIRE_FALSE( dutch1.buildings[e_colony_building::stockade] );
  REQUIRE_FALSE( dutch2.buildings[e_colony_building::stockade] );
  REQUIRE_FALSE(
      english1.buildings[e_colony_building::stockade] );

  dutch.fathers.has[e_founding_father::sieur_de_la_salle] =
      false;
  W.add_unit_indoors( dutch1.id, e_indoor_job::bells );
  give_stockade_if_needed( dutch, dutch1 );
  give_stockade_if_needed( dutch, dutch2 );
  REQUIRE_FALSE( dutch1.buildings[e_colony_building::stockade] );
  REQUIRE_FALSE( dutch2.buildings[e_colony_building::stockade] );
  REQUIRE_FALSE(
      english1.buildings[e_colony_building::stockade] );

  dutch.fathers.has[e_founding_father::sieur_de_la_salle] = true;
  give_stockade_if_needed( dutch, dutch1 );
  give_stockade_if_needed( dutch, dutch2 );
  REQUIRE( dutch1.buildings[e_colony_building::stockade] );
  REQUIRE_FALSE( dutch2.buildings[e_colony_building::stockade] );
  REQUIRE_FALSE(
      english1.buildings[e_colony_building::stockade] );

  give_stockade_if_needed( dutch, dutch1 );
  give_stockade_if_needed( dutch, dutch2 );
  REQUIRE( dutch1.buildings[e_colony_building::stockade] );
  REQUIRE_FALSE( dutch2.buildings[e_colony_building::stockade] );
  REQUIRE_FALSE(
      english1.buildings[e_colony_building::stockade] );

  give_stockade_if_needed( dutch, english1 );
  REQUIRE( dutch1.buildings[e_colony_building::stockade] );
  REQUIRE_FALSE( dutch2.buildings[e_colony_building::stockade] );
  REQUIRE( english1.buildings[e_colony_building::stockade] );
}

TEST_CASE( "[colony-mgr] colony_workers" ) {
  world W;
  Colony& colony =
      W.add_colony( { .x = 1, .y = 1 }, e_player::dutch );
  vector<UnitId> expected;

  auto f = [&] { return colony_workers( colony ); };

  // Empty.
  REQUIRE( f() == expected );

  UnitId const id1 =
      W.add_unit_outdoors( colony.id, e_direction::ne,
                           e_outdoor_job::cotton,
                           e_unit_type::expert_cotton_planter )
          .id();
  expected = { id1 };
  REQUIRE( f() == expected );

  UnitId const id2 =
      W.add_unit_indoors( colony.id, e_indoor_job::hammers,
                          e_unit_type::free_colonist )
          .id();
  expected = { id1, id2 };
  REQUIRE( f() == expected );

  UnitId const id3 =
      W.add_unit_outdoors( colony.id, e_direction::n,
                           e_outdoor_job::cotton,
                           e_unit_type::expert_cotton_planter )
          .id();
  expected = { id1, id2, id3 };
  REQUIRE( f() == expected );

  UnitId const id4 =
      W.add_unit_indoors( colony.id, e_indoor_job::bells,
                          e_unit_type::free_colonist )
          .id();
  expected = { id1, id2, id3, id4 };
  REQUIRE( f() == expected );
}

TEST_CASE( "[colony-mgr] is_valid_new_colony_name" ) {
  world w;
  string name;

  auto const f = [&] [[clang::noinline]] {
    return is_valid_new_colony_name( w.colonies(), name );
  };

  Colony& colony = w.add_colony( { .x = 1, .y = 1 } );
  colony.name    = "used";

  name = " xxx ";
  REQUIRE( f() == e_new_colony_name_err::spaces );

  name = "used";
  REQUIRE( f() == e_new_colony_name_err::already_exists );

  name = "new";
  REQUIRE( f() == valid );
}

TEST_CASE( "[colony-mgr] find_coastal_colonies" ) {
  world w;

  TerrainConnectivity const& connectivity =
      w.map_updater().connectivity();

  auto const f = [&] {
    return find_coastal_colonies( w.ss(), connectivity,
                                  w.default_player_type() );
  };

  point const kColony1 = { .x = 5, .y = 1 };
  point const kColony2 = { .x = 6, .y = 3 };
  point const kColony3 = { .x = 6, .y = 2 };

  // Sanity check that the water tiles (which are to the right of
  // each colony site) have the expected access to the right edge
  // of the map.
  auto const idx1 =
      connectivity.indices[kColony1.y *
                               w.terrain().world_size_tiles().w +
                           kColony1.x + 1];
  auto const idx2 =
      connectivity.indices[kColony2.y *
                               w.terrain().world_size_tiles().w +
                           kColony2.x + 1];

  INFO( format( "idx1={}, idx2={}", idx1, idx2 ) );
  BASE_CHECK( idx1 == 5 );
  BASE_CHECK( idx2 == 4 );

  BASE_CHECK(
      !connectivity.indices_with_right_edge_access.contains(
          idx1 ) );
  BASE_CHECK(
      connectivity.indices_with_right_edge_access.contains(
          idx2 ) );

  w.add_colony( kColony1 );
  ColonyId const colony_id_2 = w.add_colony( kColony2 ).id;
  ColonyId const colony_id_3 = w.add_colony( kColony3 ).id;

  REQUIRE( f() == vector<ColonyId>{ colony_id_2, colony_id_3 } );
}

TEST_WORLD( "[colony-mgr] find_connected_colonies" ) {
  create_disconnected_map();
  vector<ColonyId> expected;

  point tile;

  auto const f = [&] [[clang::noinline]] {
    return find_connected_colonies(
        ss(), map_updater().connectivity(),
        default_player_type(), tile );
  };

  // clang-format off
  //
  //       0  1  2  3  4  5  6  7  8
  //     +----------------------------+
  //   0 | _, L, _, L, _, L, L, L, _, | 0
  //   1 | L, L, L, L, _, L, _, L, _, | 1
  //   2 | _, L, L, L, _, L, L, L, _, | 2
  //   3 | _, _, _, _, _, _, _, _, _, | 3
  //   4 | _, L, L, L, _, L, L, L, _, | 4
  //   5 | L, L, L, L, _, L, L, L, _, | 5
  //     +----------------------------+
  //       0  1  2  3  4  5  6  7  8
  //
  // clang-format on

  point const colony_tiles[] = {
    // ---------------------
    { .x = 1, .y = 1 }, // 1
    { .x = 3, .y = 1 }, // 2
    // ---------------------
    { .x = 5, .y = 0 }, // 3
    // ---------------------
    { .x = 3, .y = 4 }, // 4
    { .x = 0, .y = 5 }, // 5
    // ---------------------
  };

  for( point const p : colony_tiles ) add_colony( p );

  tile     = { .x = 0, .y = 0 };
  expected = {};
  REQUIRE( f() == expected );

  tile     = { .x = 2, .y = 3 };
  expected = {};
  REQUIRE( f() == expected );

  tile     = { .x = 1, .y = 1 };
  expected = { 1, 2 };
  REQUIRE( f() == expected );

  tile     = { .x = 1, .y = 2 };
  expected = { 1, 2 };
  REQUIRE( f() == expected );

  tile     = { .x = 1, .y = 4 };
  expected = { 4, 5 };
  REQUIRE( f() == expected );

  tile     = { .x = 5, .y = 0 };
  expected = { 3 };
  REQUIRE( f() == expected );

  tile     = { .x = 7, .y = 2 };
  expected = { 3 };
  REQUIRE( f() == expected );

  tile     = { .x = 5, .y = 4 };
  expected = {};
  REQUIRE( f() == expected );
}

TEST_CASE( "[colony-mgr] total_colonies_population" ) {
  world w;

  auto const f =
      [&] [[clang::noinline]] ( e_player const player ) {
        return total_colonies_population( w.ss(), player );
      };

  using enum e_player;
  using enum e_unit_type;

  // Default.
  REQUIRE( f( dutch ) == 0 );
  REQUIRE( f( english ) == 0 );

  ColonyId const english1 =
      w.add_colony( { .x = 1, .y = 1 }, english ).id;
  ColonyId const english2 =
      w.add_colony( { .x = 3, .y = 1 }, english ).id;
  ColonyId const dutch1 =
      w.add_colony( { .x = 7, .y = 5 }, dutch ).id;
  ColonyId const dutch2 =
      w.add_colony( { .x = 4, .y = 4 }, dutch ).id;
  REQUIRE( f( dutch ) == 0 );
  REQUIRE( f( english ) == 0 );

  w.add_unit_indoors( english1, e_indoor_job::bells );
  REQUIRE( f( dutch ) == 0 );
  REQUIRE( f( english ) == 1 );
  w.add_unit_indoors( dutch2, e_indoor_job::bells );
  REQUIRE( f( dutch ) == 1 );
  REQUIRE( f( english ) == 1 );
  w.add_unit_outdoors( dutch2, e_direction::nw,
                       e_outdoor_job::food );
  REQUIRE( f( dutch ) == 2 );
  REQUIRE( f( english ) == 1 );

  w.add_unit_on_map( free_colonist, { .x = 1, .y = 1 },
                     english );
  REQUIRE( f( dutch ) == 2 );
  REQUIRE( f( english ) == 1 );

  w.add_unit_indoors( english2, e_indoor_job::bells );
  REQUIRE( f( dutch ) == 2 );
  REQUIRE( f( english ) == 2 );
  w.add_unit_indoors( dutch1, e_indoor_job::bells );
  REQUIRE( f( dutch ) == 3 );
  REQUIRE( f( english ) == 2 );
}

TEST_WORLD( "[colony-mgr] colony_commodities_by_value" ) {
  using enum e_commodity;
  vector<e_commodity> desired;
  vector<Commodity> expected;

  auto& prices =
      players().old_world[default_nation()].market.commodities;
  Colony& colony = add_colony( { .x = 1, .y = 1 } );
  auto& comms    = colony.commodities;

  auto const f = [&] [[clang::noinline]] {
    return colony_commodities_by_value_restricted(
        ss(), default_player(), as_const( colony ), desired );
  };

  // Default. NOTE: setting prices to default means setting the
  // bid_price to zero. The ask price is implicitly larger than
  // zero because the spreads are fixed, but only the bid price
  // is used by this method.
  prices   = {};
  comms    = {};
  desired  = {};
  expected = {};
  REQUIRE( f() == expected );

  // One item, no quantity.
  prices   = {};
  comms    = {};
  desired  = { sugar };
  expected = {};
  REQUIRE( f() == expected );

  // One item, some quantity.
  prices   = {};
  comms    = { { sugar, 10 } };
  desired  = { sugar };
  expected = { { .type = sugar, .quantity = 10 } };
  REQUIRE( f() == expected );

  // Two items, no prices.
  prices   = {};
  comms    = { { sugar, 10 }, { silver, 50 } };
  desired  = { sugar, silver };
  expected = { { .type = silver, .quantity = 50 },
               { .type = sugar, .quantity = 10 } };
  REQUIRE( f() == expected );

  // Three items, no prices.
  prices   = {};
  comms    = { { sugar, 10 }, { silver, 50 }, { muskets, 130 } };
  desired  = { sugar, silver, muskets };
  expected = { { .type = muskets, .quantity = 130 },
               { .type = silver, .quantity = 50 },
               { .type = sugar, .quantity = 10 } };
  REQUIRE( f() == expected );

  // Has bid price.
  prices[sugar].bid_price = 1;
  comms    = { { sugar, 10 }, { silver, 50 }, { muskets, 130 } };
  desired  = { sugar, silver, muskets };
  expected = {
    { .type = sugar, .quantity = 10 },
    { .type = muskets, .quantity = 130 },
    { .type = silver, .quantity = 50 },
  };
  REQUIRE( f() == expected );

  prices[sugar].bid_price   = 1;
  prices[muskets].bid_price = 1;
  comms    = { { sugar, 10 }, { silver, 50 }, { muskets, 130 } };
  desired  = { sugar, silver, muskets };
  expected = {
    { .type = muskets, .quantity = 130 },
    { .type = sugar, .quantity = 10 },
    { .type = silver, .quantity = 50 },
  };
  REQUIRE( f() == expected );

  prices[sugar].bid_price   = 12;
  prices[muskets].bid_price = 1;
  comms    = { { sugar, 10 }, { silver, 50 }, { muskets, 130 } };
  desired  = { sugar, silver, muskets };
  expected = {
    { .type = muskets, .quantity = 130 },
    { .type = sugar, .quantity = 10 },
    { .type = silver, .quantity = 50 },
  };
  REQUIRE( f() == expected );

  prices[sugar].bid_price   = 13;
  prices[muskets].bid_price = 1;
  comms    = { { sugar, 10 }, { silver, 50 }, { muskets, 130 } };
  desired  = { sugar, silver, muskets };
  expected = {
    { .type = muskets, .quantity = 130 },
    { .type = sugar, .quantity = 10 },
    { .type = silver, .quantity = 50 },
  };
  REQUIRE( f() == expected );

  prices[sugar].bid_price   = 14;
  prices[muskets].bid_price = 1;
  comms    = { { sugar, 10 }, { silver, 50 }, { muskets, 130 } };
  desired  = { sugar, silver, muskets };
  expected = {
    { .type = sugar, .quantity = 10 },
    { .type = muskets, .quantity = 130 },
    { .type = silver, .quantity = 50 },
  };
  REQUIRE( f() == expected );

  prices[sugar].bid_price   = 14;
  prices[muskets].bid_price = 1;
  prices[silver].bid_price  = 2;
  comms    = { { sugar, 10 }, { silver, 50 }, { muskets, 130 } };
  desired  = { sugar, silver, muskets };
  expected = {
    { .type = sugar, .quantity = 10 },
    { .type = muskets, .quantity = 130 },
    { .type = silver, .quantity = 50 },
  };
  REQUIRE( f() == expected );

  prices[sugar].bid_price   = 14;
  prices[muskets].bid_price = 1;
  prices[silver].bid_price  = 3;
  comms    = { { sugar, 10 }, { silver, 50 }, { muskets, 130 } };
  desired  = { sugar, silver, muskets };
  expected = {
    { .type = silver, .quantity = 50 },
    { .type = sugar, .quantity = 10 },
    { .type = muskets, .quantity = 130 },
  };
  REQUIRE( f() == expected );

  prices[sugar].bid_price   = 14;
  prices[muskets].bid_price = 2;
  prices[silver].bid_price  = 3;
  comms    = { { sugar, 10 }, { silver, 50 }, { muskets, 130 } };
  desired  = { sugar, silver, muskets };
  expected = {
    { .type = muskets, .quantity = 130 },
    { .type = silver, .quantity = 50 },
    { .type = sugar, .quantity = 10 },
  };
  REQUIRE( f() == expected );

  prices[sugar].bid_price   = 14;
  prices[muskets].bid_price = 2;
  prices[silver].bid_price  = 3;
  comms    = { { sugar, 10 }, { silver, 50 }, { muskets, 130 } };
  desired  = { sugar, muskets };
  expected = {
    { .type = muskets, .quantity = 130 },
    { .type = sugar, .quantity = 10 },
  };
  REQUIRE( f() == expected );
}

TEST_WORLD(
    "[colony-mgr] colony_commodities_by_value_restricted" ) {
  using enum e_commodity;
  vector<Commodity> expected;

  auto& prices =
      players().old_world[default_nation()].market.commodities;
  Colony& colony = add_colony( { .x = 1, .y = 1 } );
  auto& comms    = colony.commodities;

  auto const f = [&] [[clang::noinline]] {
    return colony_commodities_by_value( ss(), default_player(),
                                        as_const( colony ) );
  };

  // Default. NOTE: setting prices to default means setting the
  // bid_price to zero. The ask price is implicitly larger than
  // zero because the spreads are fixed, but only the bid price
  // is used by this method.
  prices   = {};
  comms    = {};
  expected = {};
  REQUIRE( f() == expected );

  // One item, some quantity.
  prices   = {};
  comms    = { { sugar, 10 } };
  expected = { { .type = sugar, .quantity = 10 } };
  REQUIRE( f() == expected );

  // Two items, no prices.
  prices   = {};
  comms    = { { sugar, 10 }, { silver, 50 } };
  expected = { { .type = silver, .quantity = 50 },
               { .type = sugar, .quantity = 10 } };
  REQUIRE( f() == expected );

  // Three items, no prices.
  prices   = {};
  comms    = { { sugar, 10 }, { silver, 50 }, { muskets, 130 } };
  expected = { { .type = muskets, .quantity = 130 },
               { .type = silver, .quantity = 50 },
               { .type = sugar, .quantity = 10 } };
  REQUIRE( f() == expected );

  // Has bid price.
  prices[sugar].bid_price = 1;
  comms    = { { sugar, 10 }, { silver, 50 }, { muskets, 130 } };
  expected = {
    { .type = sugar, .quantity = 10 },
    { .type = muskets, .quantity = 130 },
    { .type = silver, .quantity = 50 },
  };
  REQUIRE( f() == expected );

  prices[sugar].bid_price   = 1;
  prices[muskets].bid_price = 1;
  comms    = { { sugar, 10 }, { silver, 50 }, { muskets, 130 } };
  expected = {
    { .type = muskets, .quantity = 130 },
    { .type = sugar, .quantity = 10 },
    { .type = silver, .quantity = 50 },
  };
  REQUIRE( f() == expected );

  prices[sugar].bid_price   = 12;
  prices[muskets].bid_price = 1;
  comms    = { { sugar, 10 }, { silver, 50 }, { muskets, 130 } };
  expected = {
    { .type = muskets, .quantity = 130 },
    { .type = sugar, .quantity = 10 },
    { .type = silver, .quantity = 50 },
  };
  REQUIRE( f() == expected );

  prices[sugar].bid_price   = 13;
  prices[muskets].bid_price = 1;
  comms    = { { sugar, 10 }, { silver, 50 }, { muskets, 130 } };
  expected = {
    { .type = muskets, .quantity = 130 },
    { .type = sugar, .quantity = 10 },
    { .type = silver, .quantity = 50 },
  };
  REQUIRE( f() == expected );

  prices[sugar].bid_price   = 14;
  prices[muskets].bid_price = 1;
  comms    = { { sugar, 10 }, { silver, 50 }, { muskets, 130 } };
  expected = {
    { .type = sugar, .quantity = 10 },
    { .type = muskets, .quantity = 130 },
    { .type = silver, .quantity = 50 },
  };
  REQUIRE( f() == expected );

  prices[sugar].bid_price   = 14;
  prices[muskets].bid_price = 1;
  prices[silver].bid_price  = 2;
  comms    = { { sugar, 10 }, { silver, 50 }, { muskets, 130 } };
  expected = {
    { .type = sugar, .quantity = 10 },
    { .type = muskets, .quantity = 130 },
    { .type = silver, .quantity = 50 },
  };
  REQUIRE( f() == expected );

  prices[sugar].bid_price   = 14;
  prices[muskets].bid_price = 1;
  prices[silver].bid_price  = 3;
  comms    = { { sugar, 10 }, { silver, 50 }, { muskets, 130 } };
  expected = {
    { .type = silver, .quantity = 50 },
    { .type = sugar, .quantity = 10 },
    { .type = muskets, .quantity = 130 },
  };
  REQUIRE( f() == expected );

  prices[sugar].bid_price   = 14;
  prices[muskets].bid_price = 2;
  prices[silver].bid_price  = 3;
  comms    = { { sugar, 10 }, { silver, 50 }, { muskets, 130 } };
  expected = {
    { .type = muskets, .quantity = 130 },
    { .type = silver, .quantity = 50 },
    { .type = sugar, .quantity = 10 },
  };
  REQUIRE( f() == expected );
}

TEST_CASE( "[colony-mgr] sort_commodities_by_value" ) {
  world w;
}

TEST_CASE( "[colony-mgr] sort_slotted_commodities_by_value" ) {
  world w;
}

TEST_WORLD( "[colony-mgr] colony_auto_load_commodity" ) {
}

TEST_WORLD( "[colony-mgr] colony_auto_unload_commodity" ) {
}

} // namespace
} // namespace rn
