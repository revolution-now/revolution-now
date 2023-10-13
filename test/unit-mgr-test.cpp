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

// Must be last.
#include "test/catch-common.hpp"
#include "visibility.rds.hpp"

namespace rn {
namespace {

using namespace std;

using ::mock::matchers::_;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() {
    add_player( e_nation::dutch );
    add_player( e_nation::spanish );
    set_default_player( e_nation::dutch );
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
  World W;

  auto f = [&]( UnitId id ) {
    return current_activity_for_unit( W.units(), W.colonies(),
                                      id );
  };

  SECTION( "expert_farmer carpentry" ) {
    UnitComposition expected;
    Colony&         colony = W.add_colony( W.kLand );
    UnitId          id =
        W.add_unit_indoors( colony.id, e_indoor_job::hammers,
                            e_unit_type::expert_farmer )
            .id();
    REQUIRE( f( id ) == e_unit_activity::carpentry );
  }

  SECTION( "petty_criminal carpentry" ) {
    UnitComposition expected;
    Colony&         colony = W.add_colony( W.kLand );
    UnitId          id =
        W.add_unit_indoors( colony.id, e_indoor_job::hammers,
                            e_unit_type::petty_criminal )
            .id();
    REQUIRE( f( id ) == e_unit_activity::carpentry );
  }

  SECTION( "petty_criminal farmer" ) {
    UnitComposition expected;
    Colony&         colony = W.add_colony( W.kLand );
    UnitId          id =
        W.add_unit_outdoors( colony.id, e_direction::w,
                             e_outdoor_job::food,
                             e_unit_type::petty_criminal )
            .id();
    REQUIRE( f( id ) == e_unit_activity::farming );
  }

  SECTION( "petty_criminal no job" ) {
    UnitComposition expected;
    UnitId          id =
        W.add_unit_on_map( e_unit_type::petty_criminal, W.kLand )
            .id();
    REQUIRE( f( id ) == nothing );
  }

  SECTION( "expert_farmer no job" ) {
    UnitComposition expected;
    UnitId          id =
        W.add_unit_on_map( e_unit_type::expert_farmer, W.kLand )
            .id();
    REQUIRE( f( id ) == nothing );
  }

  SECTION( "expert_farmer dragoon" ) {
    UnitComposition expected;
    UNWRAP_CHECK( initial_ut, UnitType::create(
                                  e_unit_type::dragoon,
                                  e_unit_type::expert_farmer ) );
    UnitId id = W.add_unit_on_map( initial_ut, W.kLand ).id();
    REQUIRE( f( id ) == e_unit_activity::fighting );
  }

  SECTION( "veteran_dragoon" ) {
    UnitComposition expected;
    UNWRAP_CHECK( initial_ut, UnitType::create(
                                  e_unit_type::dragoon,
                                  e_unit_type::expert_farmer ) );
    UnitId id = W.add_unit_on_map( initial_ut, W.kLand ).id();
    REQUIRE( f( id ) == e_unit_activity::fighting );
  }

  SECTION( "pioneer" ) {
    UnitComposition expected;
    UNWRAP_CHECK(
        initial_ut,
        UnitType::create( e_unit_type::pioneer,
                          e_unit_type::petty_criminal ) );
    UnitId id = W.add_unit_on_map( initial_ut, W.kLand ).id();
    REQUIRE( f( id ) == e_unit_activity::pioneering );
  }

  SECTION( "missionary" ) {
    UnitComposition expected;
    Dwelling const& dwelling =
        W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::sioux );
    UnitId const id = W.add_missionary_in_dwelling(
                           e_unit_type::missionary, dwelling.id )
                          .id();
    REQUIRE( f( id ) == e_unit_activity::missioning );
  }
}

TEST_CASE( "[unit-mgr] tribe_type_for_unit" ) {
  World           W;
  Dwelling const& dwelling =
      W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::arawak );
  NativeUnit const& unit = W.add_native_unit_on_map(
      e_native_unit_type::mounted_brave, { .x = 0, .y = 0 },
      dwelling.id );
  REQUIRE( tribe_type_for_unit( W.ss(), unit ) ==
           e_tribe::arawak );
}

TEST_CASE( "[unit-mgr] tribe_for_unit" ) {
  World           W;
  Dwelling const& dwelling =
      W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::arawak );
  NativeUnit const& unit = W.add_native_unit_on_map(
      e_native_unit_type::mounted_brave, { .x = 0, .y = 0 },
      dwelling.id );
  REQUIRE( tribe_for_unit( W.ss(), unit ).type ==
           e_tribe::arawak );
  SSConst const ss_const( W.ss() );
  REQUIRE( tribe_for_unit( ss_const, unit ).type ==
           e_tribe::arawak );
}

TEST_CASE( "[unit-mgr] coord_for_unit_multi_ownership" ) {
  World W;

  SECTION( "colonist in colony" ) {
    W.found_colony_with_new_unit( { .x = 1, .y = 1 } );
    UnitId const id{ 1 };
    REQUIRE(
        !coord_for_unit_indirect( W.units(), id ).has_value() );
    REQUIRE( coord_for_unit_multi_ownership( W.ss(), id ) ==
             Coord{ .x = 1, .y = 1 } );
  }

  SECTION( "missionary in dwelling" ) {
    Dwelling const& dwelling =
        W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::sioux );
    UnitId const id = W.add_missionary_in_dwelling(
                           e_unit_type::missionary, dwelling.id )
                          .id();
    REQUIRE(
        !coord_for_unit_indirect( W.units(), id ).has_value() );
    REQUIRE( coord_for_unit_multi_ownership( W.ss(), id ) ==
             Coord{ .x = 1, .y = 1 } );
  }
}

TEST_CASE( "[unit-mgr] change_unit_type" ) {
  World                     W;
  VisibilityForNation const viz( W.ss(), W.default_nation() );
  Unit& unit = W.add_unit_on_map( e_unit_type::free_colonist,
                                  { .x = 3, .y = 3 } );

  REQUIRE( unit.type() == e_unit_type::free_colonist );
  REQUIRE( viz.visible( { .x = 0, .y = 0 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( viz.visible( { .x = 1, .y = 1 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( viz.visible( { .x = 2, .y = 2 } ) ==
           e_tile_visibility::visible_and_clear );
  REQUIRE( viz.visible( { .x = 3, .y = 3 } ) ==
           e_tile_visibility::visible_and_clear );

  change_unit_type( W.ss(), W.ts(), unit,
                    e_unit_type::expert_farmer );

  REQUIRE( unit.type() == e_unit_type::expert_farmer );
  REQUIRE( viz.visible( { .x = 0, .y = 0 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( viz.visible( { .x = 1, .y = 1 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( viz.visible( { .x = 2, .y = 2 } ) ==
           e_tile_visibility::visible_and_clear );
  REQUIRE( viz.visible( { .x = 3, .y = 3 } ) ==
           e_tile_visibility::visible_and_clear );

  change_unit_type( W.ss(), W.ts(), unit, e_unit_type::scout );

  REQUIRE( unit.type() == e_unit_type::scout );
  REQUIRE( viz.visible( { .x = 0, .y = 0 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( viz.visible( { .x = 1, .y = 1 } ) ==
           e_tile_visibility::visible_and_clear );
  REQUIRE( viz.visible( { .x = 2, .y = 2 } ) ==
           e_tile_visibility::visible_and_clear );
  REQUIRE( viz.visible( { .x = 3, .y = 3 } ) ==
           e_tile_visibility::visible_and_clear );
}

TEST_CASE( "[unit-mgr] change_unit_nation" ) {
  World                     W;
  VisibilityForNation const dutch_viz( W.ss(), e_nation::dutch );
  VisibilityForNation const spanish_viz( W.ss(),
                                         e_nation::spanish );
  Unit&                     unit =
      W.add_unit_on_map( e_unit_type::free_colonist,
                         { .x = 3, .y = 3 }, e_nation::dutch );

  REQUIRE( unit.type() == e_unit_type::free_colonist );
  REQUIRE( dutch_viz.visible( { .x = 0, .y = 0 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( dutch_viz.visible( { .x = 1, .y = 1 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( dutch_viz.visible( { .x = 2, .y = 2 } ) ==
           e_tile_visibility::visible_and_clear );
  REQUIRE( dutch_viz.visible( { .x = 3, .y = 3 } ) ==
           e_tile_visibility::visible_and_clear );
  REQUIRE( spanish_viz.visible( { .x = 0, .y = 0 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( spanish_viz.visible( { .x = 1, .y = 1 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( spanish_viz.visible( { .x = 2, .y = 2 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( spanish_viz.visible( { .x = 3, .y = 3 } ) ==
           e_tile_visibility::hidden );

  change_unit_nation( W.ss(), W.ts(), unit, e_nation::spanish );

  REQUIRE( unit.type() == e_unit_type::free_colonist );
  REQUIRE( dutch_viz.visible( { .x = 0, .y = 0 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( dutch_viz.visible( { .x = 1, .y = 1 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( dutch_viz.visible( { .x = 2, .y = 2 } ) ==
           e_tile_visibility::visible_and_clear );
  REQUIRE( dutch_viz.visible( { .x = 3, .y = 3 } ) ==
           e_tile_visibility::visible_and_clear );
  REQUIRE( spanish_viz.visible( { .x = 0, .y = 0 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( spanish_viz.visible( { .x = 1, .y = 1 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( spanish_viz.visible( { .x = 2, .y = 2 } ) ==
           e_tile_visibility::visible_and_clear );
  REQUIRE( spanish_viz.visible( { .x = 3, .y = 3 } ) ==
           e_tile_visibility::visible_and_clear );

  change_unit_type( W.ss(), W.ts(), unit, e_unit_type::scout );

  REQUIRE( unit.type() == e_unit_type::scout );
  REQUIRE( dutch_viz.visible( { .x = 0, .y = 0 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( dutch_viz.visible( { .x = 1, .y = 1 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( dutch_viz.visible( { .x = 2, .y = 2 } ) ==
           e_tile_visibility::visible_and_clear );
  REQUIRE( dutch_viz.visible( { .x = 3, .y = 3 } ) ==
           e_tile_visibility::visible_and_clear );
  REQUIRE( spanish_viz.visible( { .x = 0, .y = 0 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( spanish_viz.visible( { .x = 1, .y = 1 } ) ==
           e_tile_visibility::visible_and_clear );
  REQUIRE( spanish_viz.visible( { .x = 2, .y = 2 } ) ==
           e_tile_visibility::visible_and_clear );
  REQUIRE( spanish_viz.visible( { .x = 3, .y = 3 } ) ==
           e_tile_visibility::visible_and_clear );
}

TEST_CASE( "[unit-mgr] change_unit_nation_and_move" ) {
  World W;

  Coord const viz_check1{ .x = 0, .y = 0 };
  Coord const src{ .x = 1, .y = 1 };
  Coord const dst{ .x = 2, .y = 2 };
  Coord const viz_check2{ .x = 3, .y = 3 };

  Unit& unit = W.add_unit_on_map( e_unit_type::free_colonist,
                                  src, e_nation::dutch );

  VisibilityForNation const dutch_viz( W.ss(), e_nation::dutch );
  VisibilityForNation const spanish_viz( W.ss(),
                                         e_nation::spanish );

  auto f = [&] {
    change_unit_nation_and_move( W.ss(), W.ts(), unit,
                                 e_nation::spanish, dst );
  };

  REQUIRE( W.units().coord_for( unit.id() ) == src );

  REQUIRE( dutch_viz.visible( src ) ==
           e_tile_visibility::visible_and_clear );
  REQUIRE( dutch_viz.visible( dst ) ==
           e_tile_visibility::visible_and_clear );
  REQUIRE( dutch_viz.visible( viz_check1 ) ==
           e_tile_visibility::visible_and_clear );
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

  REQUIRE( W.units().coord_for( unit.id() ) == dst );

  REQUIRE( dutch_viz.visible( src ) ==
           e_tile_visibility::visible_and_clear );
  REQUIRE( dutch_viz.visible( dst ) ==
           e_tile_visibility::visible_and_clear );
  REQUIRE( spanish_viz.visible( src ) ==
           e_tile_visibility::visible_and_clear );
  REQUIRE( spanish_viz.visible( dst ) ==
           e_tile_visibility::visible_and_clear );

  // These are the real test... if we were not moving and
  // changing nation atomically then one of these would fail.
  REQUIRE( spanish_viz.visible( viz_check1 ) ==
           e_tile_visibility::hidden );
  REQUIRE( spanish_viz.visible( viz_check2 ) ==
           e_tile_visibility::visible_and_clear );
  REQUIRE( dutch_viz.visible( viz_check2 ) ==
           e_tile_visibility::hidden );
}

TEST_CASE( "[unit-mgr] offboard_units_on_ships" ) {
  World          W;
  vector<UnitId> expected;

  auto f = [&] {
    return offboard_units_on_ships( W.ss(), W.ts(), W.kLand );
  };

  auto require_on_land = [&]( Unit const& unit ) {
    REQUIRE( as_const( W.units() ).ownership_of( unit.id() ) ==
             UnitOwnership::world{ .coord = W.kLand } );
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
        W.add_unit_on_map( e_unit_type::free_colonist, W.kLand );

    SECTION( "this" ) {
      expected = {};
      REQUIRE( f() == expected );
    }

    SECTION( "two non-ship" ) {
      Unit const& soldier =
          W.add_unit_on_map( e_unit_type::soldier, W.kLand );

      SECTION( "this" ) {
        expected = {};
        REQUIRE( f() == expected );
      }

      SECTION( "two non-ship, one ship" ) {
        Unit const& galleon =
            W.add_unit_on_map( e_unit_type::galleon, W.kLand );

        SECTION( "this" ) {
          expected = {};
          REQUIRE( f() == expected );
        }

        SECTION( "two non-ship, one ship with one unit" ) {
          Unit const& indentured_servant = W.add_unit_in_cargo(
              e_unit_type::indentured_servant, galleon.id() );

          SECTION( "this" ) {
            expected = { indentured_servant.id() };
            REQUIRE( f() == expected );
          }

          SECTION( "two non-ship, one ship with two units" ) {
            Unit const& dragoon = W.add_unit_in_cargo(
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
  World          W;
  vector<UnitId> expected;
  Unit*          p_unit = nullptr;

  auto f = [&] {
    CHECK( p_unit != nullptr );
    return offboard_units_on_ship( W.ss(), W.ts(), *p_unit );
  };

  auto require_on_land = [&]( Unit const& unit ) {
    REQUIRE( as_const( W.units() ).ownership_of( unit.id() ) ==
             UnitOwnership::world{ .coord = W.kLand } );
  };

  auto require_sentried = [&]( Unit const& unit ) {
    REQUIRE( unit.orders().holds<unit_orders::sentry>() );
  };

  auto require_not_sentried = [&]( Unit const& unit ) {
    REQUIRE( !unit.orders().holds<unit_orders::sentry>() );
  };

  Unit& galleon =
      W.add_unit_on_map( e_unit_type::galleon, W.kLand );

  p_unit   = &galleon;
  expected = {};
  REQUIRE( f() == expected );

  Unit const& free_colonist =
      W.add_unit_on_map( e_unit_type::free_colonist, W.kLand );

  p_unit   = &galleon;
  expected = {};
  REQUIRE( f() == expected );
  require_on_land( free_colonist );
  require_not_sentried( free_colonist );

  Unit const& soldier =
      W.add_unit_on_map( e_unit_type::soldier, W.kLand );

  p_unit   = &galleon;
  expected = {};
  REQUIRE( f() == expected );
  require_on_land( free_colonist );
  require_not_sentried( free_colonist );
  require_on_land( soldier );
  require_not_sentried( soldier );

  Unit const& indentured_servant = W.add_unit_in_cargo(
      e_unit_type::indentured_servant, galleon.id() );

  Unit const& dragoon =
      W.add_unit_in_cargo( e_unit_type::dragoon, galleon.id() );

  p_unit   = &galleon;
  expected = { indentured_servant.id(), dragoon.id() };
  REQUIRE( f() == expected );
  require_on_land( dragoon );
  require_sentried( dragoon );
  require_on_land( indentured_servant );
  require_sentried( indentured_servant );

  Unit const& dragoon2 =
      W.add_unit_in_cargo( e_unit_type::dragoon, galleon.id() );

  p_unit   = &galleon;
  expected = { dragoon2.id() };
  REQUIRE( f() == expected );
  require_on_land( dragoon2 );
  require_sentried( dragoon2 );
}

} // namespace
} // namespace rn
