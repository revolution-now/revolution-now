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
#include "test/mocks/igui.hpp"
#include "test/rds/testing.rds.hpp"

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

TEST_CASE( "[igui] enum_choice automatic" ) {
  MockIGui gui;

  EXPECT_CALL(
      gui,
      choice( ChoiceConfig{
          .msg = "Select One",
          .options =
              vector<ChoiceConfigOption>{
                  { .key = "red", .display_name = "Red" },
                  { .key = "green", .display_name = "Green" },
                  { .key = "blue", .display_name = "Blue" } },
          .key_on_escape = "-" } ) )
      .returns( make_wait<string>( "green" ) );

  EnumChoiceConfig config{ .msg             = "Select One",
                           .choice_required = false };

  wait<maybe<e_color>> w = gui.enum_choice<e_color>();
  REQUIRE( w.ready() );
  REQUIRE( w->has_value() );
  REQUIRE( **w == e_color::green );
}

TEST_CASE( "[igui] enum_choice sorted" ) {
  MockIGui gui;

  // Note the elements in the config passed to `choice` will not
  // be sorted; the choice function does the sorting.
  EXPECT_CALL(
      gui,
      choice( ChoiceConfig{
          .msg = "Select One",
          .options =
              vector<ChoiceConfigOption>{
                  { .key = "red", .display_name = "Red" },
                  { .key = "green", .display_name = "Green" },
                  { .key = "blue", .display_name = "Blue" } },
          .key_on_escape = "-",
          .sort          = true } ) )
      .returns( make_wait<string>( "green" ) );

  wait<maybe<e_color>> w =
      gui.enum_choice<e_color>( /*sort=*/true );
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

TEST_CASE( "[igui] partial_enum_choice" ) {
  MockIGui gui;

  EXPECT_CALL(
      gui,
      choice( ChoiceConfig{
          .msg = "Select One",
          .options =
              vector<ChoiceConfigOption>{
                  { .key = "red", .display_name = "Red" },
                  { .key = "blue", .display_name = "Blue" } },
          .key_on_escape = "-" } ) )
      .returns( make_wait<string>( "blue" ) );

  wait<maybe<e_color>> w = gui.partial_enum_choice<e_color>(
      { e_color::red, e_color::blue } );
  REQUIRE( w.ready() );
  REQUIRE( w->has_value() );
  REQUIRE( **w == e_color::blue );
}

} // namespace
} // namespace rn
