/****************************************************************
**unit-mgr.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-17.
*
* Description: Unit tests for the src/unit-mgr.* module.
*
*****************************************************************/
// Under test.
#include "src/unit-mgr.hpp"

// Testing.
#include "test/fake/world.hpp"
#include "test/mocking.hpp"
#include "test/mocks/ieuro-mind.hpp"
#include "test/mocks/igui.hpp"
#include "test/testing.hpp"

// Revolution Now
#include "src/visibility.hpp"

// ss
#include "src/ss/dwelling.rds.hpp"
#include "src/ss/ref.hpp"
#include "src/ss/tribe.rds.hpp"
#include "src/ss/units.hpp"
#include "src/ss/woodcut.rds.hpp"

// refl
#include "src/refl/to-str.hpp"

// base
#include "src/base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp"
#include "visibility.rds.hpp"

namespace rn {
namespace {

using namespace std;
using namespace gfx::literals;

using ::Catch::UnorderedEquals;
using ::gfx::point;
using ::mock::matchers::_;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct world : testing::World {
  using Base = testing::World;
  world() : Base() {
    add_player( e_player::dutch );
    add_player( e_player::spanish );
    set_default_player_type( e_player::dutch );
    create_default_map();
  }

  void create_default_map() {
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
    // clang-format off
    vector<MapSquare> tiles{
      _, L, _, L, L, L,
      L, L, L, L, L, L,
      _, L, L, L, L, L,
      _, L, L, L, L, L,
      _, L, L, L, L, L,
      _, L, L, L, L, L,
    };
    // clang-format on
    build_map( std::move( tiles ), 6 );
  }

  inline static Coord const kLand = Coord{ .x = 1, .y = 1 };
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[unit-mgr] current_activity_for_unit" ) {
  world w;

  auto f = [&]( UnitId id ) {
    return current_activity_for_unit( w.units(), w.colonies(),
                                      id );
  };

  SECTION( "expert_farmer carpentry" ) {
    UnitComposition expected;
    Colony& colony = w.add_colony( w.kLand );
    UnitId id =
        w.add_unit_indoors( colony.id, e_indoor_job::hammers,
                            e_unit_type::expert_farmer )
            .id();
    REQUIRE( f( id ) == e_unit_activity::carpentry );
  }

  SECTION( "petty_criminal carpentry" ) {
    UnitComposition expected;
    Colony& colony = w.add_colony( w.kLand );
    UnitId id =
        w.add_unit_indoors( colony.id, e_indoor_job::hammers,
                            e_unit_type::petty_criminal )
            .id();
    REQUIRE( f( id ) == e_unit_activity::carpentry );
  }

  SECTION( "petty_criminal farmer" ) {
    UnitComposition expected;
    Colony& colony = w.add_colony( w.kLand );
    UnitId id =
        w.add_unit_outdoors( colony.id, e_direction::w,
                             e_outdoor_job::food,
                             e_unit_type::petty_criminal )
            .id();
    REQUIRE( f( id ) == e_unit_activity::farming );
  }

  SECTION( "petty_criminal no job" ) {
    UnitComposition expected;
    UnitId id =
        w.add_unit_on_map( e_unit_type::petty_criminal, w.kLand )
            .id();
    REQUIRE( f( id ) == nothing );
  }

  SECTION( "expert_farmer no job" ) {
    UnitComposition expected;
    UnitId id =
        w.add_unit_on_map( e_unit_type::expert_farmer, w.kLand )
            .id();
    REQUIRE( f( id ) == nothing );
  }

  SECTION( "expert_farmer dragoon" ) {
    UnitComposition expected;
    UNWRAP_CHECK( initial_ut, UnitType::create(
                                  e_unit_type::dragoon,
                                  e_unit_type::expert_farmer ) );
    UnitId id = w.add_unit_on_map( initial_ut, w.kLand ).id();
    REQUIRE( f( id ) == e_unit_activity::fighting );
  }

  SECTION( "veteran_dragoon" ) {
    UnitComposition expected;
    UNWRAP_CHECK( initial_ut, UnitType::create(
                                  e_unit_type::dragoon,
                                  e_unit_type::expert_farmer ) );
    UnitId id = w.add_unit_on_map( initial_ut, w.kLand ).id();
    REQUIRE( f( id ) == e_unit_activity::fighting );
  }

  SECTION( "pioneer" ) {
    UnitComposition expected;
    UNWRAP_CHECK(
        initial_ut,
        UnitType::create( e_unit_type::pioneer,
                          e_unit_type::petty_criminal ) );
    UnitId id = w.add_unit_on_map( initial_ut, w.kLand ).id();
    REQUIRE( f( id ) == e_unit_activity::pioneering );
  }

  SECTION( "missionary" ) {
    UnitComposition expected;
    Dwelling const& dwelling =
        w.add_dwelling( { .x = 1, .y = 1 }, e_tribe::sioux );
    UnitId const id = w.add_missionary_in_dwelling(
                           e_unit_type::missionary, dwelling.id )
                          .id();
    REQUIRE( f( id ) == e_unit_activity::missioning );
  }
}

TEST_CASE( "[unit-mgr] tribe_type_for_unit" ) {
  world w;
  Dwelling const& dwelling =
      w.add_dwelling( { .x = 1, .y = 1 }, e_tribe::arawak );
  NativeUnit const& unit = w.add_native_unit_on_map(
      e_native_unit_type::mounted_brave, { .x = 0, .y = 0 },
      dwelling.id );
  REQUIRE( tribe_type_for_unit( w.ss(), unit ) ==
           e_tribe::arawak );
}

TEST_CASE( "[unit-mgr] tribe_for_unit" ) {
  world w;
  Dwelling const& dwelling =
      w.add_dwelling( { .x = 1, .y = 1 }, e_tribe::arawak );
  NativeUnit const& unit = w.add_native_unit_on_map(
      e_native_unit_type::mounted_brave, { .x = 0, .y = 0 },
      dwelling.id );
  REQUIRE( tribe_for_unit( w.ss(), unit ).type ==
           e_tribe::arawak );
  SSConst const ss_const( w.ss() );
  REQUIRE( tribe_for_unit( ss_const, unit ).type ==
           e_tribe::arawak );
}

TEST_CASE( "[unit-mgr] coord_for_unit_multi_ownership" ) {
  world w;

  SECTION( "colonist in colony" ) {
    w.found_colony_with_new_unit( { .x = 1, .y = 1 } );
    UnitId const id{ 1 };
    REQUIRE(
        !coord_for_unit_indirect( w.units(), id ).has_value() );
    REQUIRE( coord_for_unit_multi_ownership( w.ss(), id ) ==
             Coord{ .x = 1, .y = 1 } );
  }

  SECTION( "missionary in dwelling" ) {
    Dwelling const& dwelling =
        w.add_dwelling( { .x = 1, .y = 1 }, e_tribe::sioux );
    UnitId const id = w.add_missionary_in_dwelling(
                           e_unit_type::missionary, dwelling.id )
                          .id();
    REQUIRE(
        !coord_for_unit_indirect( w.units(), id ).has_value() );
    REQUIRE( coord_for_unit_multi_ownership( w.ss(), id ) ==
             Coord{ .x = 1, .y = 1 } );
  }
}

TEST_CASE( "[unit-mgr] change_unit_type" ) {
  world w;
  VisibilityForPlayer const viz( w.ss(),
                                 w.default_player_type() );
  Unit& unit = w.add_unit_on_map( e_unit_type::free_colonist,
                                  { .x = 3, .y = 3 } );

  REQUIRE( unit.type() == e_unit_type::free_colonist );
  REQUIRE( viz.visible( { .x = 0, .y = 0 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( viz.visible( { .x = 1, .y = 1 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( viz.visible( { .x = 2, .y = 2 } ) ==
           e_tile_visibility::clear );
  REQUIRE( viz.visible( { .x = 3, .y = 3 } ) ==
           e_tile_visibility::clear );

  change_unit_type( w.ss(), w.ts(), unit,
                    e_unit_type::expert_farmer );

  REQUIRE( unit.type() == e_unit_type::expert_farmer );
  REQUIRE( viz.visible( { .x = 0, .y = 0 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( viz.visible( { .x = 1, .y = 1 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( viz.visible( { .x = 2, .y = 2 } ) ==
           e_tile_visibility::clear );
  REQUIRE( viz.visible( { .x = 3, .y = 3 } ) ==
           e_tile_visibility::clear );

  change_unit_type( w.ss(), w.ts(), unit, e_unit_type::scout );

  REQUIRE( unit.type() == e_unit_type::scout );
  REQUIRE( viz.visible( { .x = 0, .y = 0 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( viz.visible( { .x = 1, .y = 1 } ) ==
           e_tile_visibility::clear );
  REQUIRE( viz.visible( { .x = 2, .y = 2 } ) ==
           e_tile_visibility::clear );
  REQUIRE( viz.visible( { .x = 3, .y = 3 } ) ==
           e_tile_visibility::clear );
}

TEST_CASE( "[unit-mgr] change_unit_player" ) {
  world w;
  VisibilityForPlayer const dutch_viz( w.ss(), e_player::dutch );
  VisibilityForPlayer const spanish_viz( w.ss(),
                                         e_player::spanish );
  Unit& unit =
      w.add_unit_on_map( e_unit_type::free_colonist,
                         { .x = 3, .y = 3 }, e_player::dutch );

  REQUIRE( unit.type() == e_unit_type::free_colonist );
  REQUIRE( dutch_viz.visible( { .x = 0, .y = 0 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( dutch_viz.visible( { .x = 1, .y = 1 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( dutch_viz.visible( { .x = 2, .y = 2 } ) ==
           e_tile_visibility::clear );
  REQUIRE( dutch_viz.visible( { .x = 3, .y = 3 } ) ==
           e_tile_visibility::clear );
  REQUIRE( spanish_viz.visible( { .x = 0, .y = 0 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( spanish_viz.visible( { .x = 1, .y = 1 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( spanish_viz.visible( { .x = 2, .y = 2 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( spanish_viz.visible( { .x = 3, .y = 3 } ) ==
           e_tile_visibility::hidden );

  change_unit_player( w.ss(), w.ts(), unit, e_player::spanish );

  REQUIRE( unit.type() == e_unit_type::free_colonist );
  REQUIRE( dutch_viz.visible( { .x = 0, .y = 0 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( dutch_viz.visible( { .x = 1, .y = 1 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( dutch_viz.visible( { .x = 2, .y = 2 } ) ==
           e_tile_visibility::clear );
  REQUIRE( dutch_viz.visible( { .x = 3, .y = 3 } ) ==
           e_tile_visibility::clear );
  REQUIRE( spanish_viz.visible( { .x = 0, .y = 0 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( spanish_viz.visible( { .x = 1, .y = 1 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( spanish_viz.visible( { .x = 2, .y = 2 } ) ==
           e_tile_visibility::clear );
  REQUIRE( spanish_viz.visible( { .x = 3, .y = 3 } ) ==
           e_tile_visibility::clear );

  // No changes with expanded sighting radius because we are
  // changing to the units current player, in which the function
  // should be a no-op.
  w.spanish().fathers.has[e_founding_father::hernando_de_soto] =
      true;
  change_unit_player( w.ss(), w.ts(), unit, e_player::spanish );

  REQUIRE( unit.type() == e_unit_type::free_colonist );
  REQUIRE( dutch_viz.visible( { .x = 0, .y = 0 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( dutch_viz.visible( { .x = 1, .y = 1 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( dutch_viz.visible( { .x = 2, .y = 2 } ) ==
           e_tile_visibility::clear );
  REQUIRE( dutch_viz.visible( { .x = 3, .y = 3 } ) ==
           e_tile_visibility::clear );
  REQUIRE( spanish_viz.visible( { .x = 0, .y = 0 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( spanish_viz.visible( { .x = 1, .y = 1 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( spanish_viz.visible( { .x = 2, .y = 2 } ) ==
           e_tile_visibility::clear );
  REQUIRE( spanish_viz.visible( { .x = 3, .y = 3 } ) ==
           e_tile_visibility::clear );

  change_unit_type( w.ss(), w.ts(), unit, e_unit_type::scout );

  REQUIRE( unit.type() == e_unit_type::scout );
  REQUIRE( dutch_viz.visible( { .x = 0, .y = 0 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( dutch_viz.visible( { .x = 1, .y = 1 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( dutch_viz.visible( { .x = 2, .y = 2 } ) ==
           e_tile_visibility::clear );
  REQUIRE( dutch_viz.visible( { .x = 3, .y = 3 } ) ==
           e_tile_visibility::clear );
  REQUIRE( spanish_viz.visible( { .x = 0, .y = 0 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( spanish_viz.visible( { .x = 1, .y = 1 } ) ==
           e_tile_visibility::clear );
  REQUIRE( spanish_viz.visible( { .x = 2, .y = 2 } ) ==
           e_tile_visibility::clear );
  REQUIRE( spanish_viz.visible( { .x = 3, .y = 3 } ) ==
           e_tile_visibility::clear );
}

TEST_CASE( "[unit-mgr] change_unit_player_and_move" ) {
  world w;

  Coord const viz_check1{ .x = 0, .y = 0 };
  Coord const src{ .x = 1, .y = 1 };
  Coord const dst{ .x = 2, .y = 2 };
  Coord const viz_check2{ .x = 3, .y = 3 };

  Unit& unit = w.add_unit_on_map( e_unit_type::free_colonist,
                                  src, e_player::dutch );

  VisibilityForPlayer const dutch_viz( w.ss(), e_player::dutch );
  VisibilityForPlayer const spanish_viz( w.ss(),
                                         e_player::spanish );

  auto f = [&] {
    change_unit_player_and_move( w.ss(), w.ts(), unit,
                                 e_player::spanish, dst );
  };

  REQUIRE( w.units().coord_for( unit.id() ) == src );

  REQUIRE( dutch_viz.visible( src ) ==
           e_tile_visibility::clear );
  REQUIRE( dutch_viz.visible( dst ) ==
           e_tile_visibility::clear );
  REQUIRE( dutch_viz.visible( viz_check1 ) ==
           e_tile_visibility::clear );
  REQUIRE( dutch_viz.visible( viz_check2 ) ==
           e_tile_visibility::hidden );
  REQUIRE( spanish_viz.visible( src ) ==
           e_tile_visibility::hidden );
  REQUIRE( spanish_viz.visible( dst ) ==
           e_tile_visibility::hidden );
  REQUIRE( spanish_viz.visible( viz_check1 ) ==
           e_tile_visibility::hidden );
  REQUIRE( spanish_viz.visible( viz_check2 ) ==
           e_tile_visibility::hidden );

  f();

  REQUIRE( w.units().coord_for( unit.id() ) == dst );

  REQUIRE( dutch_viz.visible( src ) ==
           e_tile_visibility::clear );
  REQUIRE( dutch_viz.visible( dst ) ==
           e_tile_visibility::clear );
  REQUIRE( spanish_viz.visible( src ) ==
           e_tile_visibility::clear );
  REQUIRE( spanish_viz.visible( dst ) ==
           e_tile_visibility::clear );

  // These are the real test... if we were not moving and
  // changing player atomically then one of these would fail.
  REQUIRE( spanish_viz.visible( viz_check1 ) ==
           e_tile_visibility::hidden );
  REQUIRE( spanish_viz.visible( viz_check2 ) ==
           e_tile_visibility::clear );
  REQUIRE( dutch_viz.visible( viz_check2 ) ==
           e_tile_visibility::hidden );
}

TEST_CASE( "[unit-mgr] offboard_units_on_ships" ) {
  world w;
  vector<UnitId> expected;

  auto f = [&] {
    return offboard_units_on_ships( w.ss(), w.ts(), w.kLand );
  };

  auto require_on_land = [&]( Unit const& unit ) {
    REQUIRE( as_const( w.units() ).ownership_of( unit.id() ) ==
             UnitOwnership::world{ .coord = w.kLand } );
  };

  auto require_sentried = [&]( Unit const& unit ) {
    REQUIRE( unit.orders().holds<unit_orders::sentry>() );
  };

  auto require_not_sentried = [&]( Unit const& unit ) {
    REQUIRE( !unit.orders().holds<unit_orders::sentry>() );
  };

  // No units.
  SECTION( "this" ) {
    expected = {};
    REQUIRE( f() == expected );
  }

  SECTION( "one non-ship" ) {
    Unit const& free_colonist =
        w.add_unit_on_map( e_unit_type::free_colonist, w.kLand );

    SECTION( "this" ) {
      expected = {};
      REQUIRE( f() == expected );
    }

    SECTION( "two non-ship" ) {
      Unit const& soldier =
          w.add_unit_on_map( e_unit_type::soldier, w.kLand );

      SECTION( "this" ) {
        expected = {};
        REQUIRE( f() == expected );
      }

      SECTION( "two non-ship, one ship" ) {
        Unit const& galleon =
            w.add_unit_on_map( e_unit_type::galleon, w.kLand );

        SECTION( "this" ) {
          expected = {};
          REQUIRE( f() == expected );
        }

        SECTION( "two non-ship, one ship with one unit" ) {
          Unit const& indentured_servant = w.add_unit_in_cargo(
              e_unit_type::indentured_servant, galleon.id() );

          SECTION( "this" ) {
            expected = { indentured_servant.id() };
            REQUIRE( f() == expected );
          }

          SECTION( "two non-ship, one ship with two units" ) {
            Unit const& dragoon = w.add_unit_in_cargo(
                e_unit_type::dragoon, galleon.id() );
            expected = { indentured_servant.id(), dragoon.id() };
            REQUIRE( f() == expected );

            require_on_land( dragoon );
            require_sentried( dragoon );
          }

          require_on_land( indentured_servant );
          require_sentried( indentured_servant );
        }

        require_on_land( galleon );
        require_not_sentried( galleon );
      }

      require_on_land( soldier );
      require_not_sentried( soldier );
    }

    require_on_land( free_colonist );
    require_not_sentried( free_colonist );
  }
}

TEST_CASE( "[unit-mgr] offboard_units_on_ship" ) {
  world w;
  vector<UnitId> expected;
  Unit* p_unit = nullptr;

  auto f = [&] {
    CHECK( p_unit != nullptr );
    return offboard_units_on_ship( w.ss(), w.ts(), *p_unit );
  };

  auto require_on_land = [&]( Unit const& unit ) {
    REQUIRE( as_const( w.units() ).ownership_of( unit.id() ) ==
             UnitOwnership::world{ .coord = w.kLand } );
  };

  auto require_sentried = [&]( Unit const& unit ) {
    REQUIRE( unit.orders().holds<unit_orders::sentry>() );
  };

  auto require_not_sentried = [&]( Unit const& unit ) {
    REQUIRE( !unit.orders().holds<unit_orders::sentry>() );
  };

  Unit& galleon =
      w.add_unit_on_map( e_unit_type::galleon, w.kLand );

  p_unit   = &galleon;
  expected = {};
  REQUIRE( f() == expected );

  Unit const& free_colonist =
      w.add_unit_on_map( e_unit_type::free_colonist, w.kLand );

  p_unit   = &galleon;
  expected = {};
  REQUIRE( f() == expected );
  require_on_land( free_colonist );
  require_not_sentried( free_colonist );

  Unit const& soldier =
      w.add_unit_on_map( e_unit_type::soldier, w.kLand );

  p_unit   = &galleon;
  expected = {};
  REQUIRE( f() == expected );
  require_on_land( free_colonist );
  require_not_sentried( free_colonist );
  require_on_land( soldier );
  require_not_sentried( soldier );

  Unit const& indentured_servant = w.add_unit_in_cargo(
      e_unit_type::indentured_servant, galleon.id() );

  Unit const& dragoon =
      w.add_unit_in_cargo( e_unit_type::dragoon, galleon.id() );

  p_unit   = &galleon;
  expected = { indentured_servant.id(), dragoon.id() };
  REQUIRE( f() == expected );
  require_on_land( dragoon );
  require_sentried( dragoon );
  require_on_land( indentured_servant );
  require_sentried( indentured_servant );

  Unit const& dragoon2 =
      w.add_unit_in_cargo( e_unit_type::dragoon, galleon.id() );

  p_unit   = &galleon;
  expected = { dragoon2.id() };
  REQUIRE( f() == expected );
  require_on_land( dragoon2 );
  require_sentried( dragoon2 );
}

TEST_CASE( "[unit-mgr] units_for_tribe_ordered" ) {
  world w;

  e_tribe tribe_type = {};
  set<NativeUnitId> expected;

  auto f = [&] {
    return units_for_tribe_ordered( w.ss(), tribe_type );
  };

  expected   = {};
  tribe_type = e_tribe::apache;
  REQUIRE( f() == expected );
  expected   = {};
  tribe_type = e_tribe::aztec;
  REQUIRE( f() == expected );

  auto [apache_dwelling_id_1, apache_brave_id_1] =
      w.add_dwelling_and_brave_ids( { .x = 1, .y = 1 },
                                    e_tribe::apache );
  expected   = { apache_brave_id_1 };
  tribe_type = e_tribe::apache;
  REQUIRE( f() == expected );
  expected   = {};
  tribe_type = e_tribe::aztec;
  REQUIRE( f() == expected );

  NativeUnitId const apache_brave_id_2 =
      w.add_native_unit_on_map(
           e_native_unit_type::mounted_brave, { .x = 2, .y = 2 },
           apache_dwelling_id_1 )
          .id;
  expected   = { apache_brave_id_1, apache_brave_id_2 };
  tribe_type = e_tribe::apache;
  REQUIRE( f() == expected );
  expected   = {};
  tribe_type = e_tribe::aztec;
  REQUIRE( f() == expected );

  auto [apache_dwelling_id_2, apache_brave_id_3] =
      w.add_dwelling_and_brave_ids( { .x = 2, .y = 1 },
                                    e_tribe::apache );
  expected   = { apache_brave_id_1, apache_brave_id_2,
                 apache_brave_id_3 };
  tribe_type = e_tribe::apache;
  REQUIRE( f() == expected );
  expected   = {};
  tribe_type = e_tribe::aztec;
  REQUIRE( f() == expected );

  auto [aztec_dwelling_id_1, aztec_brave_id_1] =
      w.add_dwelling_and_brave_ids( { .x = 2, .y = 2 },
                                    e_tribe::aztec );
  expected   = { apache_brave_id_1, apache_brave_id_2,
                 apache_brave_id_3 };
  tribe_type = e_tribe::apache;
  REQUIRE( f() == expected );
  expected   = { aztec_brave_id_1 };
  tribe_type = e_tribe::aztec;
  REQUIRE( f() == expected );
}

TEST_CASE( "[unit-mgr] units_for_tribe_unordered" ) {
  world w;

  e_tribe tribe_type = {};
  vector<NativeUnitId> expected;

  auto f = [&] {
    return units_for_tribe_unordered( w.ss(), tribe_type );
  };

  expected   = {};
  tribe_type = e_tribe::apache;
  REQUIRE_THAT( f(), UnorderedEquals( expected ) );
  expected   = {};
  tribe_type = e_tribe::aztec;
  REQUIRE_THAT( f(), UnorderedEquals( expected ) );

  auto [apache_dwelling_id_1, apache_brave_id_1] =
      w.add_dwelling_and_brave_ids( { .x = 1, .y = 1 },
                                    e_tribe::apache );
  expected   = { apache_brave_id_1 };
  tribe_type = e_tribe::apache;
  REQUIRE_THAT( f(), UnorderedEquals( expected ) );
  expected   = {};
  tribe_type = e_tribe::aztec;
  REQUIRE_THAT( f(), UnorderedEquals( expected ) );

  NativeUnitId const apache_brave_id_2 =
      w.add_native_unit_on_map(
           e_native_unit_type::mounted_brave, { .x = 2, .y = 2 },
           apache_dwelling_id_1 )
          .id;
  expected   = { apache_brave_id_1, apache_brave_id_2 };
  tribe_type = e_tribe::apache;
  REQUIRE_THAT( f(), UnorderedEquals( expected ) );
  expected   = {};
  tribe_type = e_tribe::aztec;
  REQUIRE_THAT( f(), UnorderedEquals( expected ) );

  auto [apache_dwelling_id_2, apache_brave_id_3] =
      w.add_dwelling_and_brave_ids( { .x = 2, .y = 1 },
                                    e_tribe::apache );
  expected   = { apache_brave_id_1, apache_brave_id_2,
                 apache_brave_id_3 };
  tribe_type = e_tribe::apache;
  REQUIRE_THAT( f(), UnorderedEquals( expected ) );
  expected   = {};
  tribe_type = e_tribe::aztec;
  REQUIRE_THAT( f(), UnorderedEquals( expected ) );

  auto [aztec_dwelling_id_1, aztec_brave_id_1] =
      w.add_dwelling_and_brave_ids( { .x = 2, .y = 2 },
                                    e_tribe::aztec );
  expected   = { apache_brave_id_1, apache_brave_id_2,
                 apache_brave_id_3 };
  tribe_type = e_tribe::apache;
  REQUIRE_THAT( f(), UnorderedEquals( expected ) );
  expected   = { aztec_brave_id_1 };
  tribe_type = e_tribe::aztec;
  REQUIRE_THAT( f(), UnorderedEquals( expected ) );
}

TEST_CASE( "[unit-mgr] units_from_coord_recursive" ) {
  world w;

  point tile;
  vector<GenericUnitId> expected;

  auto f = [&] {
    return units_from_coord_recursive( w.units(), tile );
  };

  tile     = ( 1_x, 1_y );
  expected = {};
  REQUIRE( f() == expected );

  UnitId const free_colonist =
      w.add_unit_on_map( e_unit_type::free_colonist,
                         ( 1_x, 0_y ) )
          .id();

  tile     = ( 1_x, 1_y );
  expected = {};
  REQUIRE( f() == expected );

  tile     = ( 1_x, 0_y );
  expected = { free_colonist };
  REQUIRE( f() == expected );

  UnitId const privateer =
      w.add_unit_on_map( e_unit_type::privateer, ( 0_x, 0_y ) )
          .id();

  tile     = ( 1_x, 1_y );
  expected = {};
  REQUIRE( f() == expected );

  tile     = ( 1_x, 0_y );
  expected = { free_colonist };
  REQUIRE( f() == expected );

  tile     = ( 0_x, 0_y );
  expected = { privateer };
  REQUIRE( f() == expected );

  UnitId const artillery =
      w.add_unit_in_cargo( e_unit_type::artillery, privateer )
          .id();

  tile     = ( 0_x, 0_y );
  expected = { privateer, artillery };
  REQUIRE( f() == expected );

  UnitId const caravel =
      w.add_unit_on_map( e_unit_type::privateer, ( 1_x, 0_y ) )
          .id();
  UnitId const scout =
      w.add_unit_in_cargo( e_unit_type::scout, caravel ).id();

  tile     = ( 1_x, 0_y );
  expected = { free_colonist, caravel, scout };
  REQUIRE( f() == expected );

  DwellingId const dwelling_id =
      w.add_dwelling( ( 2_x, 2_y ), e_tribe::sioux ).id;
  NativeUnitId const armed_brave =
      w.add_native_unit_on_map( e_native_unit_type::armed_brave,
                                ( 1_x, 0_y ), dwelling_id )
          .id;

  tile     = ( 1_x, 0_y );
  expected = { free_colonist, caravel, scout, armed_brave };
  REQUIRE( f() == expected );
}

TEST_CASE( "[unit-mgr] euro_units_from_coord_recursive" ) {
  world w;

  point tile;
  vector<UnitId> expected;

  auto f = [&] {
    return euro_units_from_coord_recursive( w.units(), tile );
  };

  tile     = ( 1_x, 1_y );
  expected = {};
  REQUIRE( f() == expected );

  UnitId const free_colonist =
      w.add_unit_on_map( e_unit_type::free_colonist,
                         ( 1_x, 0_y ) )
          .id();

  tile     = ( 1_x, 1_y );
  expected = {};
  REQUIRE( f() == expected );

  tile     = ( 1_x, 0_y );
  expected = { free_colonist };
  REQUIRE( f() == expected );

  UnitId const privateer =
      w.add_unit_on_map( e_unit_type::privateer, ( 0_x, 0_y ) )
          .id();

  tile     = ( 1_x, 1_y );
  expected = {};
  REQUIRE( f() == expected );

  tile     = ( 1_x, 0_y );
  expected = { free_colonist };
  REQUIRE( f() == expected );

  tile     = ( 0_x, 0_y );
  expected = { privateer };
  REQUIRE( f() == expected );

  UnitId const artillery =
      w.add_unit_in_cargo( e_unit_type::artillery, privateer )
          .id();

  tile     = ( 0_x, 0_y );
  expected = { privateer, artillery };
  REQUIRE( f() == expected );

  UnitId const caravel =
      w.add_unit_on_map( e_unit_type::caravel, ( 1_x, 0_y ) )
          .id();
  UnitId const scout =
      w.add_unit_in_cargo( e_unit_type::scout, caravel ).id();

  tile     = ( 1_x, 0_y );
  expected = { free_colonist, caravel, scout };
  REQUIRE( f() == expected );
}

TEST_CASE(
    "[unit-mgr] euro_units_from_coord_recursive/player" ) {
  world w;

  point tile;
  e_player player = e_player::dutch;
  vector<UnitId> expected;

  auto f = [&] {
    return euro_units_from_coord_recursive( w.units(), player,
                                            tile );
  };

  tile     = ( 1_x, 1_y );
  expected = {};
  REQUIRE( f() == expected );

  UnitId const free_colonist =
      w.add_unit_on_map( e_unit_type::free_colonist,
                         ( 1_x, 0_y ), e_player::spanish )
          .id();

  tile     = ( 1_x, 1_y );
  player   = e_player::dutch;
  expected = {};
  REQUIRE( f() == expected );

  tile     = ( 1_x, 1_y );
  player   = e_player::spanish;
  expected = {};
  REQUIRE( f() == expected );

  tile     = ( 1_x, 0_y );
  player   = e_player::dutch;
  expected = {};
  REQUIRE( f() == expected );

  tile     = ( 1_x, 0_y );
  player   = e_player::spanish;
  expected = { free_colonist };
  REQUIRE( f() == expected );
}

} // namespace
} // namespace rn
