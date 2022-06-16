/****************************************************************
**igui.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-16.
*
* Description: Unit tests for the src/igui.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/igui.hpp"

// Testing
#include "test/mocking.hpp"
#include "test/rds/testing.rds.hpp"

// Revolution Now
#include "src/igui-mock.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

TEST_CASE( "[igui] enum_choice selects value" ) {
  MockIGui gui;

  EXPECT_CALL(
      gui,
      choice( ChoiceConfig{
          .msg = "my msg",
          .options =
              vector<ChoiceConfigOption>{
                  { .key = "red", .display_name = "Red" },
                  { .key = "green", .display_name = "Green" },
                  { .key          = "blue",
                    .display_name = "BlueBlue" } },
          .key_on_escape = "-" } ) )
      .returns( make_wait<string>( "green" ) );

  EnumChoiceConfig config{ .msg             = "my msg",
                           .choice_required = false };

  refl::enum_map<e_color, string> names{
      { e_color::red, "Red" },
      { e_color::green, "Green" },
      { e_color::blue, "BlueBlue" },
  };
  wait<maybe<e_color>> w = gui.enum_choice( config, names );
  REQUIRE( w.ready() );
  REQUIRE( w->has_value() );
  REQUIRE( **w == e_color::green );
}

TEST_CASE( "[igui] enum_choice cancels" ) {
  MockIGui gui;

  EXPECT_CALL(
      gui,
      choice( ChoiceConfig{
          .msg = "my msg",
          .options =
              vector<ChoiceConfigOption>{
                  { .key = "red", .display_name = "Red" },
                  { .key = "green", .display_name = "Green" },
                  { .key          = "blue",
                    .display_name = "BlueBlue" } },
          .key_on_escape = "-" } ) )
      .returns( make_wait<string>( "-" ) );

  EnumChoiceConfig config{ .msg             = "my msg",
                           .choice_required = false };

  refl::enum_map<e_color, string> names{
      { e_color::red, "Red" },
      { e_color::green, "Green" },
      { e_color::blue, "BlueBlue" },
  };
  wait<maybe<e_color>> w = gui.enum_choice( config, names );
  REQUIRE( w.ready() );
  REQUIRE( !w->has_value() );
}

} // namespace
} // namespace rn
