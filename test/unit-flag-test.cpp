/****************************************************************
**unit-flag.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-29.
*
* Description: Unit tests for the src/unit-flag.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/unit-flag.hpp"

// Testing
#include "test/fake/world.hpp"

// Revolution Now
#include "src/unit-mgr.hpp"

// config
#include "config/tile-enum.rds.hpp"

// ss
#include "ss/dwelling.rds.hpp"
#include "ss/ref.hpp"

// refl
#include "refl/to-str.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

using ::gfx::point;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() {
    add_player( e_player::english );
    add_player( e_player::french );
    set_default_player_type( e_player::english );
    create_default_map();
  }

  void create_default_map() {
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
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
TEST_CASE( "[unit-flag] euro_unit_type_orders_flag_info" ) {
  UnitFlagRenderInfo expected;
  e_unit_type unit_type = {};
  unit_orders orders    = {};
  e_player player       = {};

  auto f = [&] {
    return euro_unit_type_flag_info( unit_type, orders, player );
  };

  // Common.
  expected = {
    .stacked = false,
    .size    = { .w = 14, .h = 14 },
    .offsets = { .offset_first = { .w = 32 - 14, .h = 32 - 14 },
                 .offset_stacked = { .w = 32 - 2 - 14,
                                     .h = 32 - 2 - 14 } },
    .outline_color    = { .r = 0xcc,
                          .g = 0x07,
                          .b = 0x22,
                          .a = 0xff },
    .background_color = { .r = 255, .a = 255 },
    .contents =
        UnitFlagContents::character{ .value = '-',
                                     .color = { .r = 0x22,
                                                .g = 0x22,
                                                .b = 0x22,
                                                .a = 0xff } },
    .in_front = false,
  };
  auto baseline = expected;

  // free_colonist.
  unit_type = e_unit_type::free_colonist;
  orders    = unit_orders::none{};
  player    = e_player::english;
  REQUIRE( f() == expected );

  // Fortified.
  orders   = unit_orders::fortifying{};
  expected = baseline;
  expected.contents.get<UnitFlagContents::character>().value =
      'F';
  REQUIRE( f() == expected );

  // Fortified.
  orders   = unit_orders::fortified{};
  expected = baseline;
  expected.contents.get<UnitFlagContents::character>().value =
      'F';
  expected.contents.get<UnitFlagContents::character>().color = {
    .r = 0x77, .g = 0x77, .b = 0x77, .a = 0xff };
  REQUIRE( f() == expected );

  // Man-o-war.
  unit_type         = e_unit_type::man_o_war;
  orders            = unit_orders::none{};
  player            = e_player::english;
  expected          = baseline;
  expected.in_front = true;
  expected.offsets  = {
     .offset_first   = { .w = 32 - 14, .h = 0 },
     .offset_stacked = { .w = 32 - 2 - 14, .h = 2 } };
  REQUIRE( f() == expected );
}

TEST_CASE( "[unit-flag] euro_unit_flag_render_info" ) {
  World W;
  UnitFlagRenderInfo expected;
  Unit unit;
  maybe<e_player> viewer;
  UnitFlagOptions options;

  auto create = [&]( e_unit_type type ) {
    return create_unregistered_unit( W.default_player(), type );
  };

  auto f = [&] {
    return euro_unit_flag_render_info( unit, viewer, options );
  };

  // Common.
  expected = {
    .stacked = false,
    .size    = { .w = 14, .h = 14 },
    .offsets = { .offset_first = { .w = 32 - 14, .h = 32 - 14 },
                 .offset_stacked = { .w = 32 - 2 - 14,
                                     .h = 32 - 2 - 14 } },
    .outline_color    = { .r = 0xcc,
                          .g = 0x07,
                          .b = 0x22,
                          .a = 0xff },
    .background_color = { .r = 255, .a = 255 },
    .contents =
        UnitFlagContents::character{ .value = '-',
                                     .color = { .r = 0x22,
                                                .g = 0x22,
                                                .b = 0x22,
                                                .a = 0xff } },
    .in_front = false,
  };
  auto baseline = expected;

  // free_colonist.
  unit    = create( e_unit_type::free_colonist );
  viewer  = e_player::english;
  options = { .flag_count = e_flag_count::single,
              .type       = e_flag_char_type::normal };
  REQUIRE( f() == expected );

  // Fortified.
  unit.start_fortify();
  expected = baseline;
  expected.contents.get<UnitFlagContents::character>().value =
      'F';
  REQUIRE( f() == expected );

  // Fortified.
  unit.fortify();
  expected = baseline;
  expected.contents.get<UnitFlagContents::character>().value =
      'F';
  expected.contents.get<UnitFlagContents::character>().color = {
    .r = 0x77, .g = 0x77, .b = 0x77, .a = 0xff };
  REQUIRE( f() == expected );

  // Man-o-war.
  unit              = create( e_unit_type::man_o_war );
  viewer            = e_player::english;
  options           = { .flag_count = e_flag_count::single,
                        .type       = e_flag_char_type::normal };
  expected          = baseline;
  expected.in_front = true;
  expected.offsets  = {
     .offset_first   = { .w = 32 - 14, .h = 0 },
     .offset_stacked = { .w = 32 - 2 - 14, .h = 2 } };
  REQUIRE( f() == expected );

  // Privateer / visible.
  unit             = create( e_unit_type::privateer );
  viewer           = e_player::english;
  options          = { .flag_count = e_flag_count::multiple,
                       .type       = e_flag_char_type::normal };
  expected         = baseline;
  expected.stacked = true;
  expected.offsets = { .offset_first   = { .w = 0, .h = 0 },
                       .offset_stacked = { .w = 2, .h = 2 } };
  REQUIRE( f() == expected );

  // Privateer / all visible.
  viewer           = nothing;
  options          = { .flag_count = e_flag_count::single,
                       .type       = e_flag_char_type::normal };
  expected         = baseline;
  expected.offsets = { .offset_first   = { .w = 0, .h = 0 },
                       .offset_stacked = { .w = 2, .h = 2 } };
  REQUIRE( f() == expected );

  // Privateer / hidden.
  viewer                 = e_player::french;
  expected               = baseline;
  expected.offsets       = { .offset_first   = { .w = 0, .h = 0 },
                             .offset_stacked = { .w = 2, .h = 2 } };
  expected.outline_color = {
    .r = 0x1b, .g = 0x1b, .b = 0x1b, .a = 0xff };
  expected.background_color = {
    .r = 0x22, .g = 0x22, .b = 0x22, .a = 0xff };
  expected.contents =
      UnitFlagContents::icon{ .tile = e_tile::privateer_x };
  REQUIRE( f() == expected );
}

TEST_CASE( "[unit-flag] euro_unit_flag_render_info/cargo" ) {
  World W;
  UnitFlagRenderInfo expected;
  Unit* unit = {};
  maybe<e_player> viewer;
  UnitFlagOptions options;

  Unit& merchantman =
      W.add_unit_on_map( e_unit_type::merchantman, point{} );
  Unit& privateer =
      W.add_unit_on_map( e_unit_type::privateer, point{} );

  auto f = [&] {
    BASE_CHECK( unit != nullptr );
    return euro_unit_flag_render_info( *unit, viewer, options );
  };

  // Common.
  expected = {
    .stacked          = false,
    .size             = { .w = 14, .h = 14 },
    .offsets          = { .offset_first   = { .w = 0, .h = 0 },
                          .offset_stacked = { .w = 2, .h = 2 } },
    .outline_color    = { .r = 0xcc,
                          .g = 0x07,
                          .b = 0x22,
                          .a = 0xff },
    .background_color = { .r = 255, .a = 255 },
    .contents =
        UnitFlagContents::character{ .value = '-',
                                     .color = { .r = 0x22,
                                                .g = 0x22,
                                                .b = 0x22,
                                                .a = 0xff } },
    .in_front = false,
  };

  SECTION( "Merchantman (friendly, no cargo)" ) {
    unit              = &merchantman;
    viewer            = e_player::english;
    options           = { .flag_count = e_flag_count::single,
                          .type       = e_flag_char_type::normal };
    expected.in_front = false;
    expected.offsets  = { .offset_first   = { .w = 0, .h = 0 },
                          .offset_stacked = { .w = 2, .h = 2 } };
    REQUIRE( f() == expected );
  }

  SECTION( "Merchantman (friendly, unit cargo)" ) {
    unit = &merchantman;
    W.add_unit_in_cargo( e_unit_type::free_colonist,
                         merchantman.id() );
    viewer            = e_player::english;
    options           = { .flag_count = e_flag_count::single,
                          .type       = e_flag_char_type::normal };
    expected.in_front = false;
    expected.offsets  = { .offset_first   = { .w = 0, .h = 0 },
                          .offset_stacked = { .w = 2, .h = 2 } };
    REQUIRE( f() == expected );
  }

  SECTION( "Merchantman (friendly, commodity cargo)" ) {
    unit = &merchantman;
    W.add_commodity_in_cargo( e_commodity::food,
                              merchantman.id() );
    viewer            = e_player::english;
    options           = { .flag_count = e_flag_count::single,
                          .type       = e_flag_char_type::normal };
    expected.in_front = false;
    expected.offsets  = { .offset_first   = { .w = 0, .h = 0 },
                          .offset_stacked = { .w = 2, .h = 2 } };
    REQUIRE( f() == expected );
  }

  SECTION( "Merchantman (friendly, unit+commodity cargo)" ) {
    unit = &merchantman;
    W.add_unit_in_cargo( e_unit_type::free_colonist,
                         merchantman.id() );
    W.add_commodity_in_cargo( e_commodity::food,
                              merchantman.id() );
    viewer            = e_player::english;
    options           = { .flag_count = e_flag_count::single,
                          .type       = e_flag_char_type::normal };
    expected.in_front = false;
    expected.offsets  = { .offset_first   = { .w = 0, .h = 0 },
                          .offset_stacked = { .w = 2, .h = 2 } };
    REQUIRE( f() == expected );
  }

  SECTION( "Merchantman (viewing all, no cargo)" ) {
    unit              = &merchantman;
    viewer            = nothing;
    options           = { .flag_count = e_flag_count::single,
                          .type       = e_flag_char_type::normal };
    expected.in_front = false;
    expected.offsets  = { .offset_first   = { .w = 0, .h = 0 },
                          .offset_stacked = { .w = 2, .h = 2 } };
    REQUIRE( f() == expected );
  }

  SECTION( "Merchantman (viewing all, unit cargo)" ) {
    unit = &merchantman;
    W.add_unit_in_cargo( e_unit_type::free_colonist,
                         merchantman.id() );
    viewer            = nothing;
    options           = { .flag_count = e_flag_count::single,
                          .type       = e_flag_char_type::normal };
    expected.in_front = false;
    expected.offsets  = { .offset_first   = { .w = 0, .h = 0 },
                          .offset_stacked = { .w = 2, .h = 2 } };
    REQUIRE( f() == expected );
  }

  SECTION( "Merchantman (viewing all, commodity cargo)" ) {
    unit = &merchantman;
    W.add_commodity_in_cargo( e_commodity::food,
                              merchantman.id() );
    viewer            = nothing;
    options           = { .flag_count = e_flag_count::single,
                          .type       = e_flag_char_type::normal };
    expected.in_front = false;
    expected.offsets  = { .offset_first   = { .w = 0, .h = 0 },
                          .offset_stacked = { .w = 2, .h = 2 } };
    REQUIRE( f() == expected );
  }

  SECTION( "Merchantman (viewing all, unit+commodity cargo)" ) {
    unit = &merchantman;
    W.add_unit_in_cargo( e_unit_type::free_colonist,
                         merchantman.id() );
    W.add_commodity_in_cargo( e_commodity::food,
                              merchantman.id() );
    viewer            = nothing;
    options           = { .flag_count = e_flag_count::single,
                          .type       = e_flag_char_type::normal };
    expected.in_front = false;
    expected.offsets  = { .offset_first   = { .w = 0, .h = 0 },
                          .offset_stacked = { .w = 2, .h = 2 } };
    REQUIRE( f() == expected );
  }

  SECTION( "Merchantman (foreign, no cargo)" ) {
    unit              = &merchantman;
    viewer            = e_player::french;
    options           = { .flag_count = e_flag_count::single,
                          .type       = e_flag_char_type::normal };
    expected.in_front = false;
    expected.offsets  = { .offset_first   = { .w = 0, .h = 0 },
                          .offset_stacked = { .w = 2, .h = 2 } };
    expected.contents = UnitFlagContents::character{
      .value = '0',
      .color = { .r = 0x22, .g = 0x22, .b = 0x22, .a = 0xff } };
    REQUIRE( f() == expected );
  }

  SECTION( "Merchantman (foreign, unit cargo)" ) {
    unit = &merchantman;
    W.add_unit_in_cargo( e_unit_type::free_colonist,
                         merchantman.id() );
    viewer            = e_player::french;
    options           = { .flag_count = e_flag_count::single,
                          .type       = e_flag_char_type::normal };
    expected.in_front = false;
    expected.offsets  = { .offset_first   = { .w = 0, .h = 0 },
                          .offset_stacked = { .w = 2, .h = 2 } };
    expected.contents = UnitFlagContents::character{
      .value = '0',
      .color = { .r = 0x22, .g = 0x22, .b = 0x22, .a = 0xff } };
    REQUIRE( f() == expected );
  }

  SECTION( "Merchantman (foreign, commodity cargo)" ) {
    unit = &merchantman;
    W.add_commodity_in_cargo( e_commodity::food,
                              merchantman.id() );
    viewer            = e_player::french;
    options           = { .flag_count = e_flag_count::single,
                          .type       = e_flag_char_type::normal };
    expected.in_front = false;
    expected.offsets  = { .offset_first   = { .w = 0, .h = 0 },
                          .offset_stacked = { .w = 2, .h = 2 } };
    expected.contents = UnitFlagContents::character{
      .value = '1',
      .color = { .r = 0x22, .g = 0x22, .b = 0x22, .a = 0xff } };
    REQUIRE( f() == expected );
  }

  SECTION( "Merchantman (foreign, unit+commodity cargo)" ) {
    unit = &merchantman;
    W.add_unit_in_cargo( e_unit_type::free_colonist,
                         merchantman.id() );
    W.add_commodity_in_cargo( e_commodity::food,
                              merchantman.id() );
    viewer            = e_player::french;
    options           = { .flag_count = e_flag_count::single,
                          .type       = e_flag_char_type::normal };
    expected.in_front = false;
    expected.offsets  = { .offset_first   = { .w = 0, .h = 0 },
                          .offset_stacked = { .w = 2, .h = 2 } };
    expected.contents = UnitFlagContents::character{
      .value = '1',
      .color = { .r = 0x22, .g = 0x22, .b = 0x22, .a = 0xff } };
    REQUIRE( f() == expected );
  }

  SECTION( "Merchantman (foreign, unit+commodity x3 cargo)" ) {
    unit = &merchantman;
    W.add_unit_in_cargo( e_unit_type::free_colonist,
                         merchantman.id() );
    W.add_commodity_in_cargo( e_commodity::food,
                              merchantman.id() );
    W.add_commodity_in_cargo( e_commodity::food,
                              merchantman.id() );
    W.add_commodity_in_cargo( e_commodity::silver,
                              merchantman.id() );
    viewer            = e_player::french;
    options           = { .flag_count = e_flag_count::single,
                          .type       = e_flag_char_type::normal };
    expected.in_front = false;
    expected.offsets  = { .offset_first   = { .w = 0, .h = 0 },
                          .offset_stacked = { .w = 2, .h = 2 } };
    expected.contents = UnitFlagContents::character{
      .value = '3',
      .color = { .r = 0x22, .g = 0x22, .b = 0x22, .a = 0xff } };
    REQUIRE( f() == expected );
  }

  SECTION( "Privateer / visible" ) {
    unit = &privateer;
    W.add_unit_in_cargo( e_unit_type::free_colonist,
                         privateer.id() );
    W.add_commodity_in_cargo( e_commodity::food,
                              privateer.id() );
    viewer           = e_player::english;
    options          = { .flag_count = e_flag_count::multiple,
                         .type       = e_flag_char_type::normal };
    expected.stacked = true;
    expected.offsets = { .offset_first   = { .w = 0, .h = 0 },
                         .offset_stacked = { .w = 2, .h = 2 } };
    REQUIRE( f() == expected );
  }

  SECTION( "Privateer / all visible" ) {
    unit = &privateer;
    W.add_unit_in_cargo( e_unit_type::free_colonist,
                         privateer.id() );
    W.add_commodity_in_cargo( e_commodity::food,
                              privateer.id() );
    viewer           = nothing;
    options          = { .flag_count = e_flag_count::single,
                         .type       = e_flag_char_type::normal };
    expected.offsets = { .offset_first   = { .w = 0, .h = 0 },
                         .offset_stacked = { .w = 2, .h = 2 } };
    REQUIRE( f() == expected );
  }

  SECTION( "Privateer / hidden" ) {
    unit = &privateer;
    W.add_unit_in_cargo( e_unit_type::free_colonist,
                         privateer.id() );
    W.add_commodity_in_cargo( e_commodity::food,
                              privateer.id() );
    viewer           = e_player::french;
    expected.offsets = { .offset_first   = { .w = 0, .h = 0 },
                         .offset_stacked = { .w = 2, .h = 2 } };
    expected.outline_color = {
      .r = 0x1b, .g = 0x1b, .b = 0x1b, .a = 0xff };
    expected.background_color = {
      .r = 0x22, .g = 0x22, .b = 0x22, .a = 0xff };
    expected.contents =
        UnitFlagContents::icon{ .tile = e_tile::privateer_x };
    REQUIRE( f() == expected );
  }
}

TEST_CASE( "[unit-flag] native_unit_flag_render_info" ) {
  World W;
  UnitFlagRenderInfo expected;
  UnitFlagOptions options;

  Dwelling const& dwelling =
      W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::sioux );
  NativeUnit const& brave = W.add_native_unit_on_map(
      e_native_unit_type::brave, { .x = 1, .y = 1 },
      dwelling.id );
  NativeUnit const& mounted_brave = W.add_native_unit_on_map(
      e_native_unit_type::mounted_brave, { .x = 1, .y = 1 },
      dwelling.id );

  auto f = [&]( NativeUnit const& unit ) {
    return native_unit_flag_render_info( W.ss(), unit, options );
  };

  // Common.
  expected = {
    .stacked = false,
    .size    = { .w = 14, .h = 14 },
    .offsets = { .offset_first = { .w = 32 - 14, .h = 32 - 14 },
                 .offset_stacked = { .w = 32 - 2 - 14,
                                     .h = 32 - 2 - 14 } },
    .outline_color    = { .r = 0x6f,
                          .g = 0x04,
                          .b = 0x12,
                          .a = 0xff },
    .background_color = { .r = 0x91,
                          .g = 0x00,
                          .b = 0x00,
                          .a = 255 },
    .contents =
        UnitFlagContents::character{ .value = '-',
                                     .color = { .r = 0x22,
                                                .g = 0x22,
                                                .b = 0x22,
                                                .a = 0xff } },
    .in_front = false,
  };

  // brave.
  options = { .flag_count = e_flag_count::single,
              .type       = e_flag_char_type::normal };
  REQUIRE( f( brave ) == expected );

  // mounted_brave.
  options = { .flag_count = e_flag_count::single,
              .type       = e_flag_char_type::normal };
  REQUIRE( f( mounted_brave ) == expected );
}

TEST_CASE( "[unit-flag] native_unit_type_flag_render_info" ) {
  World W;
  UnitFlagRenderInfo expected;
  UnitFlagOptions options;

  auto f = [&]( e_native_unit_type type ) {
    return native_unit_type_flag_info( type, e_tribe::sioux,
                                       options );
  };

  // Common.
  expected = {
    .stacked = false,
    .size    = { .w = 14, .h = 14 },
    .offsets = { .offset_first = { .w = 32 - 14, .h = 32 - 14 },
                 .offset_stacked = { .w = 32 - 2 - 14,
                                     .h = 32 - 2 - 14 } },
    .outline_color    = { .r = 0x6f,
                          .g = 0x04,
                          .b = 0x12,
                          .a = 0xff },
    .background_color = { .r = 0x91,
                          .g = 0x00,
                          .b = 0x00,
                          .a = 255 },
    .contents =
        UnitFlagContents::character{ .value = '-',
                                     .color = { .r = 0x22,
                                                .g = 0x22,
                                                .b = 0x22,
                                                .a = 0xff } },
    .in_front = false,
  };

  // brave.
  options = { .flag_count = e_flag_count::single,
              .type       = e_flag_char_type::normal };
  REQUIRE( f( e_native_unit_type::brave ) == expected );

  // mounted_brave.
  options = { .flag_count = e_flag_count::single,
              .type       = e_flag_char_type::normal };
  REQUIRE( f( e_native_unit_type::mounted_brave ) == expected );
}

} // namespace
} // namespace rn
