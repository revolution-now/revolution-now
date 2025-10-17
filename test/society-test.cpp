/****************************************************************
**society.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-11-04.
*
* Description: Unit tests for the src/society.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/society.hpp"

// Testing
#include "test/fake/world.hpp"

// Revolution Now
#include "src/visibility.hpp"

// ss
#include "ss/dwelling.rds.hpp"
#include "ss/ref.hpp"
#include "ss/unit-composition.hpp"

// refl
#include "src/refl/to-str.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct world : testing::World {
  using Base = testing::World;
  world() : Base() {
    add_player( e_player::english );
    add_player( e_player::french );
    add_player( e_player::spanish );
    add_player( e_player::dutch );
    set_default_player_type( e_player::english );
    create_default_map();
  }

  void create_default_map() {
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
    // clang-format off
    vector<MapSquare> tiles{
      _, L, _,
      L, L, L,
      _, L, L,
    };
    // clang-format on
    build_map( std::move( tiles ), 3 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[society] flag_color_for_society" ) {
  Society society;

  // European.
  society = Society::european{ .player = e_player::spanish };
  REQUIRE(
      flag_color_for_society( society ) ==
      gfx::pixel{ .r = 0xff, .g = 0xfe, .b = 0x54, .a = 0xff } );

  // Native.
  society = Society::native{ .tribe = e_tribe::cherokee };
  REQUIRE(
      flag_color_for_society( society ) ==
      gfx::pixel{ .r = 0x74, .g = 0xa5, .b = 0x4c, .a = 0xff } );
}

TEST_CASE( "[society] society_on_real_square" ) {
  world W;
  Coord where;
  maybe<Society> expected;

  auto f = [&] {
    return society_on_real_square( W.ss(), where );
  };

  SECTION( "empty" ) {
    where    = { .x = 1, .y = 1 };
    expected = nothing;
    REQUIRE( f() == expected );
  }

  SECTION( "native dwelling" ) {
    where = { .x = 1, .y = 1 };
    W.add_dwelling( where, e_tribe::cherokee );
    expected = Society::native{ .tribe = e_tribe::cherokee };
    REQUIRE( f() == expected );
  }

  SECTION( "native unit" ) {
    where = { .x = 1, .y = 1 };
    Dwelling const& dwelling =
        W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::inca );
    W.add_native_unit_on_map( e_native_unit_type::brave, where,
                              dwelling.id );
    expected = Society::native{ .tribe = e_tribe::inca };
    REQUIRE( f() == expected );
  }

  SECTION( "european unit" ) {
    where = { .x = 1, .y = 1 };
    W.add_unit_on_map( e_unit_type::free_colonist, where,
                       e_player::spanish );
    expected = Society::european{ .player = e_player::spanish };
    REQUIRE( f() == expected );
  }

  SECTION( "colony" ) {
    where = { .x = 1, .y = 1 };
    W.found_colony_with_new_unit( where, e_player::french );
    expected = Society::european{ .player = e_player::french };
    REQUIRE( f() == expected );
  }
}

TEST_CASE( "[society] society_on_visible_square" ) {
  world w;
  Coord where;
  VisibleSociety expected;
  IVisibility const* p_viz = {};

  auto f = [&] {
    BASE_CHECK( p_viz );
    return society_on_visible_square( w.ss(), *p_viz, where );
  };

  VisibilityEntire const viz_entire( w.ss() );
  VisibilityForPlayer const viz_player(
      w.ss(), w.default_player_type() );

  SECTION( "viz_player" ) {
    p_viz = &viz_player;
    SECTION( "hidden" ) {
      where    = { .x = 1, .y = 1 };
      expected = VisibleSociety::hidden{};
      REQUIRE( f() == expected );
    }

    SECTION( "empty / clear" ) {
      w.make_clear( { .x = 1, .y = 1 } );
      where    = { .x = 1, .y = 1 };
      expected = VisibleSociety::empty{};
      REQUIRE( f() == expected );
    }

    SECTION( "empty / fogged" ) {
      w.make_fogged( { .x = 1, .y = 1 } );
      where    = { .x = 1, .y = 1 };
      expected = VisibleSociety::empty{};
      REQUIRE( f() == expected );
    }

    SECTION( "native dwelling / clear" ) {
      where = { .x = 1, .y = 1 };
      w.add_dwelling( where, e_tribe::cherokee );
      w.make_clear( { .x = 1, .y = 1 } );
      expected = VisibleSociety::society{
        .value = Society::native{ .tribe = e_tribe::cherokee } };
      REQUIRE( f() == expected );
    }

    SECTION( "native dwelling / fogged" ) {
      where = { .x = 1, .y = 1 };
      w.add_dwelling( where, e_tribe::cherokee );
      w.make_fogged( { .x = 1, .y = 1 } );
      expected = VisibleSociety::society{
        .value = Society::native{ .tribe = e_tribe::cherokee } };
      REQUIRE( f() == expected );
    }

    SECTION( "native unit / clear" ) {
      where = { .x = 2, .y = 1 };
      Dwelling const& dwelling =
          w.add_dwelling( { .x = 1, .y = 1 }, e_tribe::inca );
      w.add_native_unit_on_map( e_native_unit_type::brave,
                                { .x = 2, .y = 1 },
                                dwelling.id );
      w.make_clear( { .x = 2, .y = 1 } );
      expected = VisibleSociety::society{
        .value = Society::native{ .tribe = e_tribe::inca } };
      REQUIRE( f() == expected );
    }

    SECTION( "native unit / fogged" ) {
      where = { .x = 2, .y = 1 };
      Dwelling const& dwelling =
          w.add_dwelling( { .x = 1, .y = 1 }, e_tribe::inca );
      w.add_native_unit_on_map( e_native_unit_type::brave,
                                { .x = 2, .y = 1 },
                                dwelling.id );
      w.make_fogged( { .x = 2, .y = 1 } );
      expected = VisibleSociety::empty{};
      REQUIRE( f() == expected );
    }

    SECTION( "european unit / foreign / clear" ) {
      where = { .x = 1, .y = 1 };
      w.add_unit_on_map( e_unit_type::free_colonist, where,
                         e_player::spanish );
      w.make_clear( where );
      expected = VisibleSociety::society{
        .value =
            Society::european{ .player = e_player::spanish } };
      REQUIRE( f() == expected );
    }

    SECTION( "european unit / foreign / fogged" ) {
      where = { .x = 1, .y = 1 };
      w.add_unit_on_map( e_unit_type::free_colonist, where,
                         e_player::spanish );
      w.make_fogged( where );
      expected = VisibleSociety::empty{};
      REQUIRE( f() == expected );
    }

    SECTION( "european unit / friendly / clear" ) {
      where = { .x = 1, .y = 1 };
      w.add_unit_on_map( e_unit_type::free_colonist, where,
                         e_player::english );
      w.make_clear( where );
      expected = VisibleSociety::society{
        .value =
            Society::european{ .player = e_player::english } };
      REQUIRE( f() == expected );
    }

    SECTION( "colony / foreign / clear" ) {
      where = { .x = 1, .y = 1 };
      w.found_colony_with_new_unit( where, e_player::french );
      w.make_clear( where );
      expected = VisibleSociety::society{
        .value =
            Society::european{ .player = e_player::french } };
      REQUIRE( f() == expected );
    }

    SECTION( "colony / foreign / fogged" ) {
      where = { .x = 1, .y = 1 };
      w.found_colony_with_new_unit( where, e_player::french );
      w.make_fogged( where );
      expected = VisibleSociety::society{
        .value =
            Society::european{ .player = e_player::french } };
      REQUIRE( f() == expected );
    }

    SECTION( "colony / friendly / clear" ) {
      where = { .x = 1, .y = 1 };
      w.found_colony_with_new_unit( where, e_player::english );
      w.make_clear( where );
      expected = VisibleSociety::society{
        .value =
            Society::european{ .player = e_player::english } };
      REQUIRE( f() == expected );
    }
  }

  SECTION( "viz_entire" ) {
    p_viz = &viz_entire;
    SECTION( "empty" ) {
      where    = { .x = 1, .y = 1 };
      expected = VisibleSociety::empty{};
      REQUIRE( f() == expected );
    }

    SECTION( "native dwelling" ) {
      where = { .x = 1, .y = 1 };
      w.add_dwelling( where, e_tribe::cherokee );
      expected = VisibleSociety::society{
        .value = Society::native{ .tribe = e_tribe::cherokee } };
      REQUIRE( f() == expected );
    }

    SECTION( "native unit" ) {
      where = { .x = 1, .y = 1 };
      Dwelling const& dwelling =
          w.add_dwelling( { .x = 1, .y = 1 }, e_tribe::inca );
      w.add_native_unit_on_map( e_native_unit_type::brave, where,
                                dwelling.id );
      expected = VisibleSociety::society{
        .value = Society::native{ .tribe = e_tribe::inca } };
      REQUIRE( f() == expected );
    }

    SECTION( "european unit" ) {
      where = { .x = 1, .y = 1 };
      w.add_unit_on_map( e_unit_type::free_colonist, where,
                         e_player::spanish );
      expected = VisibleSociety::society{
        .value =
            Society::european{ .player = e_player::spanish } };
      REQUIRE( f() == expected );
    }

    SECTION( "colony" ) {
      where = { .x = 1, .y = 1 };
      w.found_colony_with_new_unit( where, e_player::french );
      expected = VisibleSociety::society{
        .value =
            Society::european{ .player = e_player::french } };
      REQUIRE( f() == expected );
    }
  }
}

} // namespace
} // namespace rn
