/****************************************************************
**cstate.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-12-21.
*
* Description: Unit tests for cstate module.
*
*****************************************************************/
#include "testing.hpp"

// Revolution Now
#include "cstate.hpp"
#include "lua.hpp"

// luapp
#include "luapp/state.hpp"

// Must be last.
#include "catch-common.hpp"

FMT_TO_CATCH( ::rn::UnitId );
FMT_TO_CATCH( ::rn::ColonyId );

namespace rn {
namespace {

using namespace std;
using namespace rn;

using Catch::Contains;
using Catch::UnorderedEquals;

TEST_CASE( "[cstate] create, query, destroy" ) {
  testing::default_construct_all_game_state();
  auto xp = cstate_create_colony(
      e_nation::english, Coord{ 1_x, 2_y }, "my colony" );
  REQUIRE( xp == ColonyId{ 1 } );

  Colony const& colony = colony_from_id( ColonyId{ 1 } );
  REQUIRE( colony.id() == ColonyId{ 1 } );
  REQUIRE( colony.nation() == e_nation::english );
  REQUIRE( colony.name() == "my colony" );
  REQUIRE( colony.location() == Coord{ 1_x, 2_y } );

  REQUIRE( colony_exists( ColonyId{ 1 } ) );
  REQUIRE( !colony_exists( ColonyId{ 2 } ) );

  auto xp2 = cstate_create_colony(
      e_nation::dutch, Coord{ 1_x, 3_y }, "my second colony" );
  REQUIRE( xp2 == ColonyId{ 2 } );
  REQUIRE_THAT( colonies_all(),
                UnorderedEquals( vector<ColonyId>{
                    ColonyId{ 1 }, ColonyId{ 2 } } ) );
  REQUIRE_THAT(
      colonies_all( e_nation::dutch ),
      UnorderedEquals( vector<ColonyId>{ ColonyId{ 2 } } ) );
  REQUIRE_THAT(
      colonies_all( e_nation::english ),
      UnorderedEquals( vector<ColonyId>{ ColonyId{ 1 } } ) );
  REQUIRE_THAT( colonies_all( e_nation::french ),
                UnorderedEquals( vector<ColonyId>{} ) );

  cstate_destroy_colony( ColonyId{ 1 } );
  REQUIRE_THAT(
      colonies_all(),
      UnorderedEquals( vector<ColonyId>{ ColonyId{ 2 } } ) );

  cstate_destroy_colony( ColonyId{ 2 } );
  REQUIRE_THAT( colonies_all(),
                UnorderedEquals( vector<ColonyId>{} ) );
}

TEST_CASE( "[cstate] colonies_in_rect" ) {
  testing::default_construct_all_game_state();
  vector<Coord> coords{
      { 1_x, 2_y }, // 1
      { 1_x, 5_y }, // 2
      { 1_x, 8_y }, // 3
      { 3_x, 5_y }, // 4
      { 4_x, 4_y }, // 5
      { 5_x, 3_y }, // 6
      { 4_x, 2_y }, // 7
      { 3_x, 2_y }, // 8
      { 4_x, 3_y }, // 9
      { 7_x, 9_y }, // 10
      { 3_x, 3_y }, // 11
  };
  int i = 0;
  for( auto coord : coords ) {
    auto xp =
        cstate_create_colony( e_nation::english, coord,
                              fmt::format( "colony{}", i++ ) );
    REQUIRE( xp == ColonyId{ i } );
  }
  REQUIRE_THAT( colonies_in_rect( Rect{ 3_x, 3_y, 0_w, 0_h } ),
                UnorderedEquals( vector<ColonyId>{} ) );
  REQUIRE_THAT(
      colonies_in_rect( Rect{ 3_x, 3_y, 1_w, 1_h } ),
      UnorderedEquals( vector<ColonyId>{ ColonyId{ 11 } } ) );
  REQUIRE_THAT( colonies_in_rect( Rect{ 3_x, 3_y, 3_w, 3_h } ),
                UnorderedEquals( vector<ColonyId>{
                    ColonyId{ 4 }, ColonyId{ 5 }, ColonyId{ 6 },
                    ColonyId{ 9 }, ColonyId{ 11 } } ) );
  REQUIRE_THAT( colonies_in_rect( Rect{ 3_x, 1_y, 2_w, 8_h } ),
                UnorderedEquals( vector<ColonyId>{
                    ColonyId{ 4 },
                    ColonyId{ 5 },
                    ColonyId{ 7 },
                    ColonyId{ 8 },
                    ColonyId{ 9 },
                    ColonyId{ 11 },
                } ) );
}

TEST_CASE( "[cstate] lua" ) {
  lua::state& st = lua_global_state();
  testing::default_construct_all_game_state();
  auto xp = cstate_create_colony(
      e_nation::english, Coord{ 1_x, 2_y }, "my colony" );
  REQUIRE( xp == ColonyId{ 1 } );
  auto script = R"(
    local colony = cstate.colony_from_id( 1 )
    assert_eq( colony:id(), 1 )
    assert_eq( colony:name(), "my colony" )
    assert_eq( colony:nation(), e.nation.english )
    assert_eq( colony:location(), Coord{x=1,y=2} )
  )";
  REQUIRE( st.script.run_safe( script ) == valid );

  auto xp2 = st.script.run_safe( "cstate.colony_from_id( 2 )" );
  REQUIRE( !xp2.valid() );
  REQUIRE_THAT( xp2.error(),
                Contains( "colony 2_id does not exist" ) );
}

} // namespace
} // namespace rn
