/****************************************************************
**command-road.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-12-06.
*
* Description: Unit tests for the src/command-road.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/command-road.hpp"

// Testing
#include "test/fake/world.hpp"
#include "test/mocking.hpp"
#include "test/mocks/igui.hpp"

// Revolution Now
#include "src/native-owned.hpp"

// ss
#include "src/ss/natives.hpp"
#include "src/ss/ref.hpp"
#include "src/ss/settings.rds.hpp"
#include "src/ss/terrain.hpp"
#include "src/ss/units.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

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
TEST_CASE( "[command-road] native-owned land" ) {
  World W;
  W.settings().difficulty = e_difficulty::conquistador;
  Dwelling const& dwelling =
      W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::tupi );
  Tribe&      tribe = W.natives().tribe_for( e_tribe::tupi );
  Coord const tile{ .x = 2, .y = 2 };
  Unit const& pioneer =
      W.add_unit_on_map( e_unit_type::pioneer, tile );
  TribeRelationship& relationship =
      tribe.relationship[W.default_nation()];
  relationship.encountered = true;
  for( int y = 0; y < 3; ++y )
    for( int x = 0; x < 3; ++x )
      W.natives().mark_land_owned( dwelling.id,
                                   { .x = x, .y = y } );
  unique_ptr<CommandHandler> handler =
      handle_command( W.ss(), W.ts(), W.default_player(),
                      pioneer.id(), command::road{} );

  REQUIRE( relationship.tribal_alarm == 0 );
  REQUIRE_FALSE( pioneer.mv_pts_exhausted() );

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

  SECTION( "cancel" ) {
    auto config_matcher = Field(
        &ChoiceConfig::msg, StrContains( "Carving a [road]" ) );
    W.gui()
        .EXPECT__choice( std::move( config_matcher ),
                         e_input_required::no )
        .returns<maybe<string>>( "cancel" );
    REQUIRE( confirm() == false );
    REQUIRE( relationship.tribal_alarm == 0 );
    REQUIRE_FALSE( pioneer.mv_pts_exhausted() );
    REQUIRE( relationship.land_squares_paid_for == 0 );
    REQUIRE( is_land_native_owned( W.ss(), W.default_player(),
                                   tile ) );
    REQUIRE( pioneer.orders() == unit_orders::none{} );
  }

  SECTION( "take" ) {
    auto config_matcher = Field(
        &ChoiceConfig::msg, StrContains( "Carving a [road]" ) );
    W.gui()
        .EXPECT__choice( std::move( config_matcher ),
                         e_input_required::no )
        .returns<maybe<string>>( "take" );
    REQUIRE( confirm() == true );
    REQUIRE( relationship.tribal_alarm == 10 );
    REQUIRE_FALSE( pioneer.mv_pts_exhausted() );
    REQUIRE( relationship.land_squares_paid_for == 0 );
    REQUIRE_FALSE(
        is_land_native_owned( W.ss(), W.default_player(), tile )
            .has_value() );
    perform();
    REQUIRE( pioneer.orders() ==
             unit_orders::road{ .turns_worked = 0 } );
    REQUIRE_FALSE( pioneer.mv_pts_exhausted() );
  }
}

} // namespace
} // namespace rn
