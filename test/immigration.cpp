/****************************************************************
**immigration.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-05-28.
*
* Description: Unit tests for the src/immigration.* module.
*
*****************************************************************/
#include "test/mocking.hpp"
#include "test/testing.hpp"

// Under test.
#include "src/immigration.hpp"

// Revolution Now
#include "src/igui-mock.hpp"
#include "src/igui.hpp"

// Rds
#include "old-world-state.rds.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

TEST_CASE( "[immigration] ask_player_to_choose_immigrant" ) {
  ImmigrationState immigration{
      .immigrants_pool =
          {
              e_unit_type::expert_farmer,
              e_unit_type::veteran_soldier,
              e_unit_type::seasoned_scout,
          },
  };
  MockIGui gui;

  EXPECT_CALL( gui,
               choice( ChoiceConfig{
                   .msg = "please select one",
                   .options =
                       {
                           { .key          = "0",
                             .display_name = "Expert Farmer" },
                           { .key          = "1",
                             .display_name = "Veteran Soldier" },
                           { .key          = "2",
                             .display_name = "Seasoned Scout" },
                       },
                   .key_on_escape = "-",
               } ) )
      .returns( make_wait<string>( "1" ) );

  wait<maybe<int>> w = ask_player_to_choose_immigrant(
      gui, immigration, "please select one" );
  REQUIRE( w.ready() );
  REQUIRE( w->has_value() );
  REQUIRE( **w == 1 );
}

} // namespace
} // namespace rn
