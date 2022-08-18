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
#include "test/mocks/igui.hpp"
#include "test/mocks/land-view-plane.hpp"

// Revolution Now
#include "colony-mgr.hpp"
#include "map-square.hpp"
#include "map-updater.hpp"
#include "plane-stack.hpp"
#include "ustate.hpp"

// ss
#include "ss/colonies.hpp"
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
using ::mock::matchers::StrContains;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() {
    add_player( e_nation::dutch );
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
  UnitId      id =
      W.add_unit_on_map( e_unit_type::free_colonist, coord );
  REQUIRE( unit_can_found_colony( W.ss(), id ) == base::valid );
  ColonyId col_id = found_colony( W.ss(), W.ts(), id, "colony" );
  Colony&  col    = W.colonies().colony_for( col_id );
  for( auto [type, q] : col.commodities ) {
    INFO( fmt::format( "type: {}, q: {}", type, q ) );
    REQUIRE( q == 0 );
  }
}

TEST_CASE( "[colony-mgr] native convert cannot found" ) {
  World W;

  Coord const coord = { .x = 1, .y = 1 };
  UnitId      id =
      W.add_unit_on_map( e_unit_type::native_convert, coord );
  REQUIRE( unit_can_found_colony( W.ss(), id ) ==
           e_found_colony_err::native_convert_cannot_found );
}

TEST_CASE( "[colony-mgr] found_colony strips unit" ) {
  World W;

  SECTION( "dragoon" ) {
    Coord const coord = { .x = 1, .y = 1 };
    UnitId      id    = W.add_unit_on_map(
        UnitType::create( e_unit_type::dragoon,
                                  e_unit_type::petty_criminal )
            .value(),
        coord );
    Unit& founder = W.units().unit_for( id );
    REQUIRE( founder.type() == e_unit_type::dragoon );
    REQUIRE( unit_can_found_colony( W.ss(), id ).valid() );
    ColonyId col_id =
        found_colony( W.ss(), W.ts(), id, "colony" );
    REQUIRE( founder.type() == e_unit_type::petty_criminal );
    Colony& col = W.colonies().colony_for( col_id );
    // Make sure that the founding unit has shed all of its com-
    // modities into the colony commodity store.
    for( auto [type, q] : col.commodities ) {
      INFO( fmt::format( "type: {}, q: {}", type, q ) );
      switch( type ) {
        case e_commodity::horses: REQUIRE( q == 50 ); break;
        case e_commodity::muskets: REQUIRE( q == 50 ); break;
        default: REQUIRE( q == 0 ); break;
      }
    }
  }

  SECTION( "hardy_pioneer" ) {
    Coord const coord = { .x = 1, .y = 1 };
    UnitId      id =
        W.add_unit_on_map( e_unit_type::hardy_pioneer, coord );
    Unit& founder = W.units().unit_for( id );
    REQUIRE( founder.type() == e_unit_type::hardy_pioneer );
    REQUIRE( unit_can_found_colony( W.ss(), id ).valid() );
    ColonyId col_id =
        found_colony( W.ss(), W.ts(), id, "colony" );
    REQUIRE( founder.type() == e_unit_type::hardy_colonist );
    Colony& col = W.colonies().colony_for( col_id );
    // Make sure that the founding unit has shed all of its com-
    // modities into the colony commodity store.
    for( auto [type, q] : col.commodities ) {
      INFO( fmt::format( "type: {}, q: {}", type, q ) );
      switch( type ) {
        case e_commodity::tools: REQUIRE( q == 100 ); break;
        default: REQUIRE( q == 0 ); break;
      }
    }
  }
}

TEST_CASE(
    "[colony-mgr] found_colony on existing colony fails" ) {
  World W;

  Coord const coord = { .x = 1, .y = 1 };
  UnitId      id =
      W.add_unit_on_map( e_unit_type::free_colonist, coord );
  REQUIRE( unit_can_found_colony( W.ss(), id ).valid() );
  ColonyId col_id =
      found_colony( W.ss(), W.ts(), id, "colony 1" );
  Colony& col = W.colonies().colony_for( col_id );
  for( auto [type, q] : col.commodities ) {
    INFO( fmt::format( "type: {}, q: {}", type, q ) );
    REQUIRE( q == 0 );
  }

  id = W.add_unit_on_map( e_unit_type::free_colonist, coord );
  REQUIRE( unit_can_found_colony( W.ss(), id ) ==
           invalid( e_found_colony_err::colony_exists_here ) );
}

TEST_CASE(
    "[colony-mgr] found_colony too close to another colony "
    "fails" ) {
  World W;

  Coord  coord = { .x = 1, .y = 1 };
  UnitId id =
      W.add_unit_on_map( e_unit_type::free_colonist, coord );
  REQUIRE( unit_can_found_colony( W.ss(), id ).valid() );
  found_colony( W.ss(), W.ts(), id, "colony" );
  coord.x += 1;
  id = W.add_unit_on_map( e_unit_type::free_colonist, coord );
  REQUIRE( unit_can_found_colony( W.ss(), id ) ==
           invalid( e_found_colony_err::too_close_to_colony ) );
}

TEST_CASE( "[colony-mgr] can't build colony in water" ) {
  World W;

  Coord const coord = { .x = 2, .y = 3 };
  CHECK( W.square( coord ).surface == e_surface::water );
  UnitId ship_id =
      W.add_unit_on_map( e_unit_type::merchantman, coord );
  UnitId unit_id =
      create_free_unit( W.units(), W.default_nation(),
                        e_unit_type::free_colonist );
  W.units().change_to_cargo_somewhere( ship_id, unit_id );
  REQUIRE( unit_can_found_colony( W.ss(), unit_id ) ==
           invalid( e_found_colony_err::no_water_colony ) );
}

TEST_CASE( "[colony-mgr] can't build colony on moutains" ) {
  World W;

  Coord const coord = { .x = 1, .y = 1 };
  CHECK( W.square( coord ).surface == e_surface::land );
  W.square( coord ).overlay = e_land_overlay::mountains;
  UnitId unit_id =
      W.add_unit_on_map( e_unit_type::free_colonist, coord );
  REQUIRE( unit_can_found_colony( W.ss(), unit_id ) ==
           invalid( e_found_colony_err::no_mountain_colony ) );
}

TEST_CASE(
    "[colony-mgr] found_colony by unit not on map fails" ) {
  World W;

  UnitId id = create_free_unit( W.units(), W.default_nation(),
                                e_unit_type::free_colonist );
  W.units().change_to_harbor_view(
      id,
      UnitHarborViewState{ .port_status = PortStatus::in_port{},
                           .sailed_from = {} } );
  REQUIRE( unit_can_found_colony( W.ss(), id ) ==
           invalid( e_found_colony_err::colonist_not_on_map ) );
}

TEST_CASE( "[colony-mgr] found_colony by ship fails" ) {
  World W;

  Coord const coord = { .x = 2, .y = 3 };
  CHECK( W.square( coord ).surface == e_surface::water );
  auto id = W.add_unit_on_map( e_unit_type::merchantman, coord );
  REQUIRE(
      unit_can_found_colony( W.ss(), id ) ==
      invalid( e_found_colony_err::ship_cannot_found_colony ) );
}

TEST_CASE( "[colony-mgr] found_colony by non-human fails" ) {
  World W;

  Coord const coord = { .x = 1, .y = 1 };
  auto id = W.add_unit_on_map( e_unit_type::wagon_train, coord );
  REQUIRE(
      unit_can_found_colony( W.ss(), id ) ==
      invalid(
          e_found_colony_err::non_human_cannot_found_colony ) );
}

TEST_CASE( "[colony-mgr] create, query, destroy" ) {
  World   W;
  Colony& colony =
      W.add_colony( Coord{ .x = 1, .y = 2 }, e_nation::english );
  REQUIRE( colony.id == ColonyId{ 1 } );

  REQUIRE( colony.id == ColonyId{ 1 } );
  REQUIRE( colony.nation == e_nation::english );
  REQUIRE( colony.name == "1" );
  REQUIRE( colony.location == Coord{ .x = 1, .y = 2 } );

  // These will check-fail if they don't exist.
  W.colonies().colony_for( ColonyId{ 1 } );

  Colony& colony2 =
      W.add_colony( Coord{ .x = 1, .y = 3 }, e_nation::dutch );
  REQUIRE( colony2.id == 2 );
  W.colonies().colony_for( ColonyId{ 1 } );
  W.colonies().colony_for( ColonyId{ 2 } );
  REQUIRE( W.colonies().all().size() == 2 );
  REQUIRE( W.colonies().all().contains( 1 ) );
  REQUIRE( W.colonies().all().contains( 2 ) );
  REQUIRE_THAT(
      W.colonies().for_nation( e_nation::dutch ),
      UnorderedEquals( vector<ColonyId>{ ColonyId{ 2 } } ) );
  REQUIRE_THAT(
      W.colonies().for_nation( e_nation::english ),
      UnorderedEquals( vector<ColonyId>{ ColonyId{ 1 } } ) );
  REQUIRE_THAT( W.colonies().for_nation( e_nation::french ),
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
  World   W;
  Colony& colony =
      W.add_colony_with_new_unit( Coord{ .x = 1, .y = 1 } );
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

  UnitId founder = W.add_unit_on_map( e_unit_type::free_colonist,
                                      Coord{ .x = 1, .y = 1 } );
  // Don't use W.add_colony here because we are testing
  // found_colony specifically.
  ColonyId id =
      found_colony( W.ss(), W.ts(), founder, "my colony" );
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
           ( OutdoorUnit{ .unit_id = founder,
                          .job     = e_outdoor_job::food } ) );
}

TEST_CASE( "[colony-mgr] change_unit_outdoor_job." ) {
  World   W;
  Colony& colony = W.add_colony( Coord{ .x = 1, .y = 1 } );
  // Note that the founding colonist will have been placed on the
  // north tile.
  UnitId farmer = W.add_unit_outdoors( colony.id, e_direction::w,
                                       e_outdoor_job::food );
  UnitId ore_miner = W.add_unit_outdoors(
      colony.id, e_direction::e, e_outdoor_job::ore );

  // Sanity check.
  REQUIRE( colony.outdoor_jobs[e_direction::w] ==
           ( OutdoorUnit{ .unit_id = farmer,
                          .job     = e_outdoor_job::food } ) );
  REQUIRE( colony.outdoor_jobs[e_direction::e] ==
           ( OutdoorUnit{ .unit_id = ore_miner,
                          .job     = e_outdoor_job::ore } ) );

  // Change job.
  change_unit_outdoor_job( colony, farmer,
                           e_outdoor_job::lumber );

  REQUIRE( colony.outdoor_jobs[e_direction::w] ==
           ( OutdoorUnit{ .unit_id = farmer,
                          .job     = e_outdoor_job::lumber } ) );
  REQUIRE( colony.outdoor_jobs[e_direction::e] ==
           ( OutdoorUnit{ .unit_id = ore_miner,
                          .job     = e_outdoor_job::ore } ) );
}

TEST_CASE( "[colony-mgr] destroy_colony" ) {
  World       W;
  Coord const loc            = { .x = 1, .y = 1 };
  Colony&     colony         = W.add_colony_with_new_unit( loc );
  vector<UnitId> const units = colony_units_all( colony );
  REQUIRE( units.size() == 1 );
  UnitId const founder = units[0];

  REQUIRE( W.units().exists( founder ) );
  REQUIRE( W.colonies().all().size() == 1 );
  REQUIRE( W.terrain().square_at( loc ).road );

  SECTION( "non interactive" ) {
    destroy_colony( W.ss(), W.map_updater(), colony );
  }

  SECTION( "non interactive with ship" ) {
    UnitId ship = W.add_unit_on_map( e_unit_type::caravel, loc );
    destroy_colony( W.ss(), W.map_updater(), colony );
    REQUIRE( W.units().exists( ship ) );
  }

  SECTION( "interactive" ) {
    MockLandViewPlane mock_land_view;
    W.planes().back().land_view = &mock_land_view;

    EXPECT_CALL( mock_land_view,
                 landview_animate_colony_depixelation( _ ) )
        .returns( make_wait<>() );
    EXPECT_CALL( W.gui(), message_box( "some msg" ) )
        .returns( make_wait<>() );

    wait<> w = run_colony_destruction(
        W.planes(), W.ss(), W.ts(), colony, /*msg=*/"some msg" );
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
  }

  SECTION( "interactive with ship" ) {
    UnitId ship = W.add_unit_on_map( e_unit_type::caravel, loc );

    MockLandViewPlane mock_land_view;
    W.planes().back().land_view = &mock_land_view;

    EXPECT_CALL( mock_land_view,
                 landview_animate_colony_depixelation( _ ) )
        .returns( make_wait<>() );
    EXPECT_CALL( W.gui(), message_box( "some msg" ) )
        .returns( make_wait<>() );
    EXPECT_CALL(
        W.gui(),
        message_box( StrContains( "had ships in its port" ) ) )
        .returns( make_wait<>() );

    wait<> w = run_colony_destruction(
        W.planes(), W.ss(), W.ts(), colony, /*msg=*/"some msg" );
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );

    REQUIRE( W.units().exists( ship ) );
  }

  // !! Do not access colony after this point.

  REQUIRE( !W.units().exists( founder ) );
  REQUIRE( W.colonies().all().size() == 0 );
  REQUIRE( !W.terrain().square_at( loc ).road );
}

TEST_CASE(
    "[colony-mgr] find_occupied_surrounding_colony_squares" ) {
  World   W;
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

TEST_CASE( "[colony-mgr] found_colony finds job for unit." ) {
  // TODO
}

TEST_CASE( "[colony-mgr] evolve colonies." ) {
  // TODO
}

TEST_CASE( "[colony-mgr] SoL membership changed." ) {
  // TODO
}

} // namespace
} // namespace rn
