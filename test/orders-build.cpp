/****************************************************************
**orders-build.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-12-06.
*
* Description: Unit tests for the src/orders-build.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/orders-build.hpp"

// Testing
#include "test/fake/world.hpp"
#include "test/mocking.hpp"
#include "test/mocks/igui.hpp"

// ss
#include "src/ss/colonies.hpp"
#include "src/ss/ref.hpp"
#include "src/ss/terrain.hpp"
#include "src/ss/units.hpp"

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
  }

  void create_default_map() {
    MapSquare const   _ = make_ocean();
    MapSquare const   L = make_grassland();
    vector<MapSquare> tiles{
        _, L, _, //
        L, L, L, //
        _, L, L, //
    };
    build_map( std::move( tiles ), 3 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[orders-build] build colony" ) {
#ifdef COMPILER_GCC
  return;
#endif
  World        W;
  Coord const  tile{ .x = 2, .y = 2 };
  UnitId const colonist_id =
      W.add_unit_on_map( e_unit_type::free_colonist, tile );
  Unit const& unit = W.units().unit_for( colonist_id );
  unique_ptr<OrdersHandler> handler = handle_orders(
      W.planes(), W.ss(), W.ts(), W.default_player(),
      colonist_id, orders::build{} );

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

  REQUIRE( W.colonies().last_colony_id() == 0 );

  EXPECT_CALL(
      W.gui(),
      choice( Field( &ChoiceConfig::msg,
                     StrContains( "Build colony here" ) ),
              e_input_required::no ) )
      .returns<maybe<string>>( "yes" );
  EXPECT_CALL( W.gui(), string_input( _, e_input_required::no ) )
      .returns<maybe<string>>( "my colony" );
  REQUIRE( confirm() == true );
  REQUIRE_FALSE( unit.mv_pts_exhausted() );

  perform();
  REQUIRE( unit.orders() == e_unit_orders::none );
  REQUIRE_FALSE( unit.mv_pts_exhausted() );
  REQUIRE( W.colonies().last_colony_id() == 1 );
  Colony const& colony = W.colonies().colony_for( 1 );
  REQUIRE( colony.name == "my colony" );
  REQUIRE( colony.location == tile );
}

} // namespace
} // namespace rn
