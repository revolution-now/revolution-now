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
#include "lua.hpp"
#include "terrain.hpp"
#include "ustate.hpp"
#include "utype.hpp"

// Must be last.
#include "catch-common.hpp"

FMT_TO_CATCH( ::rn::UnitId );
FMT_TO_CATCH( ::rn::ColonyId );

namespace {

using namespace std;
using namespace rn;

UnitId create_colonist( Coord where ) {
  return create_unit_on_map( e_nation::english,
                             e_unit_type::free_colonist, where );
}

TEST_CASE( "[colony-mgr] create colony on land successful" ) {
  testing::default_construct_all_game_state();
  generate_unittest_terrain();

  Coord coord = { 2_x, 2_y };
  auto  id    = create_colonist( coord );
  REQUIRE( found_colony( id, coord, "colony 1" ) );
}

TEST_CASE(
    "[colony-mgr] create colony on existing colony fails" ) {
  testing::default_construct_all_game_state();
  generate_unittest_terrain();

  Coord coord = { 2_x, 2_y };
  auto  id    = create_colonist( coord );
  REQUIRE( found_colony( id, coord, "colony 1" ) );

  id = create_colonist( coord );
  REQUIRE( !found_colony( id, coord, "colony 2" ) );
}

TEST_CASE(
    "[colony-mgr] create colony with existing name fails" ) {
  testing::default_construct_all_game_state();
  generate_unittest_terrain();

  Coord coord = { 2_x, 2_y };
  auto  id    = create_colonist( coord );
  REQUIRE( found_colony( id, coord, "colony 1" ) );

  coord += 1_w;
  id = create_colonist( coord );
  REQUIRE( !found_colony( id, coord, "colony 1" ) );
}

TEST_CASE( "[colony-mgr] lua" ) {
  testing::default_construct_all_game_state();
  generate_unittest_terrain();
  // auto xp = cstate_create_colony(
  //    e_nation::english, Coord{ 1_x, 2_y }, "my colony" );
  // REQUIRE( xp == ColonyId{ 1 } );
  // auto script = R"(
  //  local colony = cstate.colony_from_id( 1 )
  //  assert_eq( colony:id(), 1 )
  //  assert_eq( colony:name(), "my colony" )
  //  assert_eq( colony:nation(), e.nation.english )
  //  assert_eq( colony:location(), Coord{x=1,y=2} )
  //)";
  // REQUIRE( lua::run<void>( script ) == xp_success_t{} );

  // auto xp2 = lua::run<void>( "cstate.colony_from_id( 2 )" );
  // REQUIRE( !xp2.has_value() );
  // REQUIRE_THAT( xp2.error().what,
  //              Contains( "colony 2_id does not exist" ) );
}

} // namespace
