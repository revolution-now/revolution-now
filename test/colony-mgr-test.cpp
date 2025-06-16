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
#include "test/mocks/ieuro-mind.hpp"
#include "test/mocks/land-view-plane.hpp"
#include "test/util/coro.hpp"

// Revolution Now
#include "src/colony-mgr.hpp"
#include "src/harbor-units.hpp"
#include "src/imap-updater.hpp"
#include "src/map-square.hpp"
#include "src/plane-stack.hpp"
#include "src/unit-mgr.hpp"
#include "src/unit-ownership.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/player.rds.hpp"
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

using ::mock::matchers::_;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() {
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
      _, L, _, L, L, L,
      L, L, L, L, L, L,
      _, L, L, L, L, L,
      _, L, _, L, L, L,
      _, L, L, L, L, L,
      L, L, L, L, L, L,
    };
    // clang-format on
    build_map( std::move( tiles ), 6 );
  }
};

TEST_CASE( "[colony-mgr] found_colony on land successful" ) {
  World W;

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
  World W;
  W.default_player().revolution.status =
      e_revolution_status::declared;
  Coord const coord = { .x = 1, .y = 1 };
  Unit& unit =
      W.add_unit_on_map( e_unit_type::free_colonist, coord );
  REQUIRE( unit_can_found_colony( W.ss(), unit.id() ) ==
           e_found_colony_err::war_of_independence );
}

TEST_CASE( "[colony-mgr] native convert cannot found" ) {
  World W;

  Coord const coord = { .x = 1, .y = 1 };
  Unit& unit =
      W.add_unit_on_map( e_unit_type::native_convert, coord );
  REQUIRE( unit_can_found_colony( W.ss(), unit.id() ) ==
           e_found_colony_err::native_convert_cannot_found );
}

TEST_CASE( "[colony-mgr] found_colony strips unit" ) {
  World W;

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
  World W;

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
  World W;

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
  World W;

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
  World W;

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
  World W;

  UnitId id = create_free_unit( W.units(), W.default_player(),
                                e_unit_type::free_colonist );
  unit_move_to_port( W.ss(), id );
  REQUIRE( unit_can_found_colony( W.ss(), id ) ==
           invalid( e_found_colony_err::colonist_not_on_map ) );
}

TEST_CASE( "[colony-mgr] found_colony by ship fails" ) {
  World W;

  Coord const coord = { .x = 2, .y = 3 };
  CHECK( W.square( coord ).surface == e_surface::water );
  UnitId id =
      W.add_unit_on_map( e_unit_type::merchantman, coord ).id();
  REQUIRE(
      unit_can_found_colony( W.ss(), id ) ==
      invalid( e_found_colony_err::ship_cannot_found_colony ) );
}

TEST_CASE( "[colony-mgr] found_colony by non-colonist fails" ) {
  World W;

  Coord const coord = { .x = 1, .y = 1 };
  UnitId id =
      W.add_unit_on_map( e_unit_type::wagon_train, coord ).id();
  REQUIRE( unit_can_found_colony( W.ss(), id ) ==
           invalid( e_found_colony_err::
                        non_colonist_cannot_found_colony ) );
}

TEST_CASE( "[colony-mgr] create, query, destroy" ) {
  World W;
  Colony& colony =
      W.add_colony( Coord{ .x = 1, .y = 2 }, e_player::english );
  REQUIRE( colony.id == ColonyId{ 1 } );

  REQUIRE( colony.id == ColonyId{ 1 } );
  REQUIRE( colony.player == e_player::english );
  REQUIRE( colony.name == "1" );
  REQUIRE( colony.location == Coord{ .x = 1, .y = 2 } );

  // These will check-fail if they don't exist.
  W.colonies().colony_for( ColonyId{ 1 } );

  Colony& colony2 =
      W.add_colony( Coord{ .x = 1, .y = 3 }, e_player::dutch );
  REQUIRE( colony2.id == 2 );
  W.colonies().colony_for( ColonyId{ 1 } );
  W.colonies().colony_for( ColonyId{ 2 } );
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
  World W;
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
  World W;

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
  World W;
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
  World W;
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

    mock_land_view.EXPECT__animate( _ ).returns( make_wait<>() );
    W.euro_mind()
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

    W.euro_mind()
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

    mock_land_view.EXPECT__animate( _ ).returns( make_wait<>() );

    SECTION( "reason=battle" ) {
      ship_damaged_reason = e_ship_damaged_reason::battle;
      W.euro_mind()
          .EXPECT__message_box( "some msg" )
          .returns( make_wait<>() );
      W.euro_mind()
          .EXPECT__message_box(
              "Port in [1] contained two [Caravels] that were "
              "damaged in battle and were sent to [Amsterdam] "
              "for repairs." )
          .returns( make_wait<>() );
      W.euro_mind()
          .EXPECT__message_box(
              "Port in [1] contained one [Merchantman] that was "
              "damaged in battle and was sent to [Amsterdam] "
              "for repairs." )
          .returns( make_wait<>() );
    }

    SECTION( "reason=colony_abandoned" ) {
      ship_damaged_reason =
          e_ship_damaged_reason::colony_abandoned;
      W.euro_mind()
          .EXPECT__message_box( "some msg" )
          .returns( make_wait<>() );
      W.euro_mind()
          .EXPECT__message_box(
              "Port in [1] contained two [Caravels] that were "
              "damaged during colony collapse and were sent "
              "to [Amsterdam] for repairs." )
          .returns( make_wait<>() );
      W.euro_mind()
          .EXPECT__message_box(
              "Port in [1] contained one [Merchantman] that was "
              "damaged during colony collapse and was sent to "
              "[Amsterdam] for repairs." )
          .returns( make_wait<>() );
    }

    SECTION( "reason=colony_starved" ) {
      ship_damaged_reason =
          e_ship_damaged_reason::colony_starved;
      W.euro_mind()
          .EXPECT__message_box( "some msg" )
          .returns( make_wait<>() );
      W.euro_mind()
          .EXPECT__message_box(
              "Port in [1] contained two [Caravels] that were "
              "damaged during colony collapse and were sent "
              "to [Amsterdam] for repairs." )
          .returns( make_wait<>() );
      W.euro_mind()
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

    mock_land_view.EXPECT__animate( _ );

    ship_damaged_reason = e_ship_damaged_reason::battle;
    W.euro_mind()
        .EXPECT__message_box( "some msg" )
        .returns( make_wait<>() );
    W.euro_mind()
        .EXPECT__message_box(
            "Port in [1] contained two [Caravels] that were "
            "damaged in battle and destroyed as there are no "
            "available ports for repair." )
        .returns( make_wait<>() );
    W.euro_mind()
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
  World W;
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
  World W;
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
  World W;
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

TEST_CASE( "[colony-mgr] found_colony finds job for unit." ) {
  // TODO
  //
  // 1. Avoids indian villages.
  // 2. Avoids indian-owned land.
}

} // namespace
} // namespace rn
