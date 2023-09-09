/****************************************************************
**command-build.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-12-06.
*
* Description: Unit tests for the src/command-build.* module.
*
*****************************************************************/
#include "test/mocking.hpp"
#include "test/testing.hpp"

// Under test.
#include "src/command-build.hpp"

// Testing
#include "test/fake/world.hpp"
#include "test/mocks/icolony-viewer.hpp"
#include "test/mocks/igui.hpp"

// ss
#include "src/ss/colonies.hpp"
#include "src/ss/ref.hpp"
#include "src/ss/terrain.hpp"
#include "src/ss/units.hpp"
#include "src/ss/woodcut.rds.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

using ::mock::matchers::_;
using ::mock::matchers::Field;
using ::mock::matchers::StrContains;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() {
    add_default_player();
    create_default_map();
    update_terrain_connectivity();
  }

  void create_default_map() {
    MapSquare const   _ = make_ocean();
    MapSquare const   L = make_grassland();
    vector<MapSquare> tiles{
        _, L, _, L, L, //
        L, L, L, L, L, //
        _, _, L, L, L, //
        _, L, L, L, L, //
        _, L, L, _, L, //
    };
    build_map( std::move( tiles ), 5 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[command-build] build colony" ) {
#ifdef COMPILER_GCC
  return;
#endif
  World       W;
  Coord const tile{ .x = 2, .y = 2 };
  Unit const& unit =
      W.add_unit_on_map( e_unit_type::free_colonist, tile );
  unique_ptr<CommandHandler> handler =
      handle_command( W.ss(), W.ts(), W.default_player(),
                      unit.id(), command::build{} );

  REQUIRE_FALSE( unit.mv_pts_exhausted() );

  auto confirm = [&] {
    wait<bool> w_confirm = handler->confirm();
    REQUIRE( !w_confirm.exception() );
    REQUIRE( w_confirm.ready() );
    return *w_confirm;
  };

  auto perform = [&] {
    wait<> w_confirm = handler->perform();
    REQUIRE( !w_confirm.exception() );
    REQUIRE( w_confirm.ready() );
  };

  REQUIRE( W.colonies().last_colony_id() == nothing );

  W.gui()
      .EXPECT__display_woodcut(
          e_woodcut::building_first_colony )
      .returns();
  W.gui()
      .EXPECT__string_input( _, e_input_required::no )
      .returns<maybe<string>>( "my colony" );
  REQUIRE( confirm() == true );
  REQUIRE_FALSE( unit.mv_pts_exhausted() );

  W.colony_viewer()
      .EXPECT__show( _, ColonyId{ 1 } )
      .returns( e_colony_abandoned::no );
  perform();
  REQUIRE( unit.orders().to_enum() == unit_orders::e::none );
  REQUIRE_FALSE( unit.mv_pts_exhausted() );
  REQUIRE( W.colonies().last_colony_id() == 1 );
  Colony const& colony = W.colonies().colony_for( 1 );
  REQUIRE( colony.name == "my colony" );
  REQUIRE( colony.location == tile );
}

TEST_CASE( "[command-build] build colony no ocean access" ) {
#ifdef COMPILER_GCC
  return;
#endif
  World       W;
  Coord const tile{ .x = 3, .y = 3 };
  Unit const& unit =
      W.add_unit_on_map( e_unit_type::free_colonist, tile );
  unique_ptr<CommandHandler> handler =
      handle_command( W.ss(), W.ts(), W.default_player(),
                      unit.id(), command::build{} );

  REQUIRE_FALSE( unit.mv_pts_exhausted() );

  auto confirm = [&] {
    wait<bool> w_confirm = handler->confirm();
    REQUIRE( !w_confirm.exception() );
    REQUIRE( w_confirm.ready() );
    return *w_confirm;
  };

  REQUIRE( W.colonies().last_colony_id() == nothing );

  auto config_matcher =
      Field( &ChoiceConfig::msg, StrContains( "ocean access" ) );
  W.gui()
      .EXPECT__choice( config_matcher, e_input_required::no )
      .returns<maybe<string>>( "no" );
  REQUIRE( confirm() == false );
  REQUIRE_FALSE( unit.mv_pts_exhausted() );
  REQUIRE( unit.orders().to_enum() == unit_orders::e::none );
  REQUIRE( W.colonies().last_colony_id() == nothing );
}

} // namespace
} // namespace rn
