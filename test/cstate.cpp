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

// Under test.
#include "cstate.hpp"

// Revolution Now
#include "colony-mgr.hpp"
#include "game-state.hpp"
#include "lua.hpp"

// ss
#include "ss/colonies.hpp"

// luapp
#include "luapp/state.hpp"

// Must be last.
#include "catch-common.hpp"

namespace rn {
namespace {

using namespace std;

using Catch::Contains;

TEST_CASE( "[cstate] lua create colony" ) {
  lua::state& st = lua_global_state();
  testing::default_construct_all_game_state();
  ColoniesState& colonies_state = GameState::colonies();
  auto           xp             = create_empty_colony(
                            colonies_state, e_nation::english, Coord{ .x = 1, .y = 2 },
                            "my colony" );
  REQUIRE( xp == ColonyId{ 1 } );
  auto script = R"(
    local colony = cstate.colony_from_id( 1 )
    assert_eq( colony.id, 1 )
    assert_eq( colony.name, "my colony" )
    assert_eq( colony.nation, "english" )
    assert_eq( colony.location, { x=1, y=2 } )
  )";
  REQUIRE( st.script.run_safe( script ) == valid );

  auto xp2 = st.script.run_safe( "cstate.colony_from_id( 2 )" );
  REQUIRE( !xp2.valid() );
  REQUIRE_THAT( xp2.error(),
                Contains( "colony 2 does not exist" ) );
}

} // namespace
} // namespace rn
