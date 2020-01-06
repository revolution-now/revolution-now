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

// Revolution Now
#include "colony-mgr.hpp"
#include "cstate.hpp"
#include "lua.hpp"
#include "terrain.hpp"
#include "ustate.hpp"
#include "utype.hpp"

// Must be last.
#include "catch-common.hpp"

FMT_TO_CATCH( ::rn::UnitId );
FMT_TO_CATCH( ::rn::ColonyId );

namespace rn::testing {
namespace {

using namespace std;
using namespace rn;

void init_game_world_for_test() {
  testing::default_construct_all_game_state();
  generate_unittest_terrain();
}

UnitId create_colonist( Coord where ) {
  return create_unit_on_map( e_nation::english,
                             e_unit_type::free_colonist, where );
}

UnitId create_colonist() {
  return create_unit( e_nation::english,
                      e_unit_type::free_colonist );
}

UnitId create_ship( Coord where ) {
  return create_unit_on_map( e_nation::english,
                             e_unit_type::merchantman, where );
}

TEST_CASE( "[colony-mgr] create colony on land successful" ) {
  init_game_world_for_test();

  Coord coord = { 2_x, 2_y };
  auto  id    = create_colonist( coord );
  REQUIRE( can_found_colony( id ) ==
           e_found_colony_result::good );
  REQUIRE( found_colony( id, "colony" ) );
}

TEST_CASE(
    "[colony-mgr] create colony on existing colony fails" ) {
  init_game_world_for_test();

  Coord coord = { 2_x, 2_y };
  auto  id    = create_colonist( coord );
  REQUIRE( can_found_colony( id ) ==
           e_found_colony_result::good );
  REQUIRE( found_colony( id, "colony 1" ) );

  id = create_colonist( coord );
  REQUIRE( can_found_colony( id ) ==
           e_found_colony_result::colony_exists_here );
  REQUIRE( !found_colony( id, "colony 2" ) );
}

TEST_CASE(
    "[colony-mgr] create colony with existing name fails" ) {
  init_game_world_for_test();

  Coord coord = { 2_x, 2_y };
  auto  id    = create_colonist( coord );
  REQUIRE( can_found_colony( id ) ==
           e_found_colony_result::good );
  REQUIRE( found_colony( id, "colony" ) );

  coord += 1_w;
  id = create_colonist( coord );
  REQUIRE( can_found_colony( id ) ==
           e_found_colony_result::good );
  REQUIRE( !found_colony( id, "colony" ) );
}

TEST_CASE( "[colony-mgr] create colony in water fails" ) {
  init_game_world_for_test();

  Coord coord   = { 1_x, 1_y };
  auto  ship_id = create_ship( coord );
  auto  unit_id = create_colonist();
  ustate_change_to_cargo( ship_id, unit_id );
  REQUIRE( can_found_colony( unit_id ) ==
           e_found_colony_result::no_water_colony );
  REQUIRE( !found_colony( unit_id, "colony" ) );
}

TEST_CASE(
    "[colony-mgr] found colony by unit not on map fails" ) {
  init_game_world_for_test();

  auto id = create_colonist();
  ustate_change_to_euro_port_view(
      id, UnitEuroPortViewState::in_port{} );
  REQUIRE( can_found_colony( id ) ==
           e_found_colony_result::colonist_not_on_map );
  REQUIRE( !found_colony( id, "colony" ) );
}

TEST_CASE( "[colony-mgr] found colony by ship fails" ) {
  init_game_world_for_test();

  Coord coord = { 1_x, 1_y };
  auto  id    = create_ship( coord );
  REQUIRE( can_found_colony( id ) ==
           e_found_colony_result::ship_cannot_found_colony );
  REQUIRE( !found_colony( id, "colony" ) );
}

TEST_CASE( "[colony-mgr] lua" ) {
  init_game_world_for_test();
  auto script = R"(
    coord = Coord{y=2, x=2}
    unit_ = ustate.create_unit_on_map(
             e.nation.english,
             e.unit_type.free_colonist,
             coord )
    col_id = colony_mgr.found_colony(
               unit_:id(), "New York" )
    local colony = cstate.colony_from_id( col_id )
    assert_eq( colony:id(), 1 )
    assert_eq( colony:name(), "New York" )
    assert_eq( colony:nation(), e.nation.english )
    assert_eq( colony:location(), Coord{x=2,y=2} )
    return col_id
  )";
  REQUIRE( lua::run<ColonyId>( script ) == ColonyId{ 1 } );
  REQUIRE( colony_from_id( ColonyId{ 1 } ).name() ==
           "New York" );
}

} // namespace
} // namespace rn::testing
