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
#include "src/refl/to-str.hpp"

// base
#include "src/base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

TEST_CASE( "[igui] optional_enum_choice selects value" ) {
  MockIGui gui;

  gui.EXPECT__choice(
         ChoiceConfig{
             .msg = "my msg",
             .options =
                 vector<ChoiceConfigOption>{
                     { .key = "red", .display_name = "Red" },
                     { .key = "green", .display_name = "Green" },
                     { .key          = "blue",
                       .display_name = "BlueBlue" } } },
         e_input_required::no )
      .returns( make_wait<maybe<string>>( "green" ) );

  EnumChoiceConfig config{ .msg = "my msg" };

  refl::enum_map<e_color, string> names{
      { e_color::red, "Red" },
      { e_color::green, "Green" },
      { e_color::blue, "BlueBlue" },
  };
  wait<maybe<e_color>> w =
      gui.optional_enum_choice( config, names );
  REQUIRE( w.ready() );
  REQUIRE( w->has_value() );
  REQUIRE( **w == e_color::green );
}

TEST_CASE( "[igui] optional_enum_choice selects nothing" ) {
  MockIGui gui;

  gui.EXPECT__choice(
         ChoiceConfig{
             .msg = "my msg",
             .options =
                 vector<ChoiceConfigOption>{
                     { .key = "red", .display_name = "Red" },
                     { .key = "green", .display_name = "Green" },
                     { .key          = "blue",
                       .display_name = "BlueBlue" } } },
         e_input_required::no )
      .returns( make_wait<maybe<string>>( nothing ) );

  EnumChoiceConfig config{ .msg = "my msg" };

  refl::enum_map<e_color, string> names{
      { e_color::red, "Red" },
      { e_color::green, "Green" },
      { e_color::blue, "BlueBlue" },
  };
  wait<maybe<e_color>> w =
      gui.optional_enum_choice( config, names );
  REQUIRE( w.ready() );
  REQUIRE( !w->has_value() );
}

TEST_CASE( "[igui] optional_enum_choice automatic" ) {
  MockIGui gui;

  gui.EXPECT__choice(
         ChoiceConfig{
             .msg = "Select One",
             .options =
                 vector<ChoiceConfigOption>{
                     { .key = "red", .display_name = "Red" },
                     { .key = "green", .display_name = "Green" },
                     { .key          = "blue",
                       .display_name = "Blue" } } },
         e_input_required::no )
      .returns( make_wait<maybe<string>>( "green" ) );

  EnumChoiceConfig config{ .msg = "Select One" };

  wait<maybe<e_color>> w = gui.optional_enum_choice<e_color>();
  REQUIRE( w.ready() );
  REQUIRE( w->has_value() );
  REQUIRE( **w == e_color::green );
}

TEST_CASE( "[igui] required_enum_choice sorted" ) {
  MockIGui gui;

  // Note the elements in the config passed to `choice` will not
  // be sorted; the choice function does the sorting.

  gui.EXPECT__choice(
         ChoiceConfig{
             .msg = "Select One",
             .options =
                 vector<ChoiceConfigOption>{
                     { .key = "red", .display_name = "Red" },
                     { .key = "green", .display_name = "Green" },
                     { .key = "blue", .display_name = "Blue" } },
             .sort = true },
         e_input_required::yes )
      .returns( make_wait<maybe<string>>( "green" ) );

  wait<e_color> w =
      gui.required_enum_choice<e_color>( /*sort=*/true );
  REQUIRE( w.ready() );
  REQUIRE( *w == e_color::green );
}

TEST_CASE( "[igui] partial_optional_enum_choice" ) {
  MockIGui gui;

  gui.EXPECT__choice(
         ChoiceConfig{
             .msg = "Select One",
             .options =
                 vector<ChoiceConfigOption>{
                     { .key = "red", .display_name = "Red" },
                     { .key          = "blue",
                       .display_name = "Blue" } } },
         e_input_required::no )
      .returns( make_wait<maybe<string>>( "blue" ) );

  wait<maybe<e_color>> w =
      gui.partial_optional_enum_choice<e_color>(
          { e_color::red, e_color::blue } );
  REQUIRE( w.ready() );
  REQUIRE( w->has_value() );
  REQUIRE( **w == e_color::blue );
}

TEST_CASE( "[igui] enum_check_boxes" ) {
  MockIGui gui;

  refl::enum_map<e_color, CheckBoxInfo> items{
      { e_color::red,
        CheckBoxInfo{
            .name = "Red", .on = false, .disabled = true } },
      { e_color::green,
        CheckBoxInfo{
            .name = "Green", .on = true, .disabled = false } },
      { e_color::blue,
        CheckBoxInfo{
            .name = "Blue", .on = false, .disabled = false } },
  };
  auto expected               = items;
  expected[e_color::green].on = false;
  expected[e_color::blue].on  = true;
  unordered_map<int, CheckBoxInfo> int_items{
      { 0,
        CheckBoxInfo{
            .name = "Red", .on = false, .disabled = true } },
      { 1,
        CheckBoxInfo{
            .name = "Green", .on = true, .disabled = false } },
      { 2,
        CheckBoxInfo{
            .name = "Blue", .on = false, .disabled = false } },
  };
  unordered_map<int, bool> res_int_items{
      { 0, false },
      { 1, false },
      { 2, true },
  };
  gui.EXPECT__check_box_selector( "hello",
                                  std::move( int_items ) )
      .returns<>( std::move( res_int_items ) );

  wait<> w = gui.enum_check_boxes( "hello", items );
  REQUIRE( !w.exception() );
  REQUIRE( w.ready() );
  REQUIRE( items == expected );
}

} // namespace
} // namespace rn
