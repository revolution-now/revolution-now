/****************************************************************
**unit-stack.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-01-07.
*
* Description: Unit tests for the src/unit-stack.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/unit-stack.hpp"

// Testing
#include "test/fake/world.hpp"

// ss
#include "ss/dwelling.rds.hpp"
#include "ss/ref.hpp"
#include "ss/units.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

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
TEST_CASE( "[unit-stack] sort_euro_unit_stack" ) {
  World          W;
  vector<UnitId> units, expected;

  auto add = [&]( e_unit_type type ) {
    units.push_back( W.add_free_unit( type ).id() );
  };

  SECTION( "single" ) {
    add( e_unit_type::privateer ); // 1
    sort_euro_unit_stack( W.ss(), units );
    expected = {
        UnitId{ 1 },
    };
    REQUIRE( units == expected );
  }

  SECTION( "multiple" ) {
    add( e_unit_type::petty_criminal );    // 1
    add( e_unit_type::free_colonist );     // 2
    add( e_unit_type::soldier );           // 3
    add( e_unit_type::veteran_dragoon );   // 4
    add( e_unit_type::veteran_soldier );   // 5
    add( e_unit_type::damaged_artillery ); // 6
    add( e_unit_type::continental_army );  // 7
    add( e_unit_type::caravel );           // 8
    add( e_unit_type::privateer );         // 9
    add( e_unit_type::artillery );         // 10
    expected = { UnitId{ 9 }, UnitId{ 10 }, UnitId{ 4 },
                 UnitId{ 7 }, UnitId{ 5 },  UnitId{ 6 },
                 UnitId{ 3 }, UnitId{ 8 },  UnitId{ 1 },
                 UnitId{ 2 } };
    sort_euro_unit_stack( W.ss(), units );
    REQUIRE( units == expected );
  }
}

TEST_CASE( "[unit-stack] sort_native_unit_stack" ) {
  World                W;
  vector<NativeUnitId> units, expected;

  auto add = [&]( e_native_unit_type type ) {
    units.push_back( W.add_free_unit( type ).id );
  };

  SECTION( "single" ) {
    add( e_native_unit_type::brave ); // 1
    sort_native_unit_stack( W.ss(), units );
    expected = {
        NativeUnitId{ 1 },
    };
    REQUIRE( units == expected );
  }

  SECTION( "multiple" ) {
    add( e_native_unit_type::mounted_brave );   // 1
    add( e_native_unit_type::mounted_warrior ); // 2
    add( e_native_unit_type::brave );           // 3
    add( e_native_unit_type::armed_brave );     // 4
    add( e_native_unit_type::brave );           // 5
    expected = { NativeUnitId{ 2 }, NativeUnitId{ 1 },
                 NativeUnitId{ 4 }, NativeUnitId{ 3 },
                 NativeUnitId{ 5 } };
    sort_native_unit_stack( W.ss(), units );
    REQUIRE( units == expected );
  }
}

TEST_CASE( "[unit-stack] sort_unit_stack" ) {
  World                 W;
  vector<GenericUnitId> units, expected;

  auto add = [&]<typename T>( T type ) {
    if constexpr( is_same_v<T, e_unit_type> )
      units.push_back( W.add_free_unit( type ).id() );
    else
      units.push_back( W.add_free_unit( type ).id );
  };

  SECTION( "single" ) {
    add( e_native_unit_type::brave ); // 1
    sort_unit_stack( W.ss(), units );
    expected = {
        GenericUnitId{ 1 },
    };
    REQUIRE( units == expected );
  }

  SECTION( "multiple" ) {
    add( e_native_unit_type::mounted_brave );   // 1
    add( e_native_unit_type::mounted_warrior ); // 2
    add( e_native_unit_type::brave );           // 3
    add( e_unit_type::veteran_soldier );        // 4
    add( e_unit_type::caravel );                // 5
    add( e_native_unit_type::armed_brave );     // 6
    add( e_native_unit_type::brave );           // 7
    expected = { GenericUnitId{ 2 }, GenericUnitId{ 4 },
                 GenericUnitId{ 1 }, GenericUnitId{ 5 },
                 GenericUnitId{ 6 }, GenericUnitId{ 3 },
                 GenericUnitId{ 7 } };
    sort_unit_stack( W.ss(), units );
    REQUIRE( units == expected );
  }
}

TEST_CASE( "[unit-stack] select_euro_unit_defender" ) {
  World       W;
  Coord const coord{ .x = 1, .y = 1 };

  auto f = [&] {
    return select_euro_unit_defender( W.ss(), coord );
  };

  auto add = [&]( e_unit_type type ) {
    return W.add_unit_on_map( type, coord ).id();
  };

  UnitId const petty_criminal_id1 =
      add( e_unit_type::petty_criminal );
  REQUIRE( f() == petty_criminal_id1 );

  add( e_unit_type::petty_criminal );
  REQUIRE( f() == petty_criminal_id1 );

  UnitId const soldier_id = add( e_unit_type::soldier );
  REQUIRE( f() == soldier_id );

  UnitId const veteran_dragoon_id =
      add( e_unit_type::veteran_dragoon );
  REQUIRE( f() == veteran_dragoon_id );

  add( e_unit_type::dragoon );
  REQUIRE( f() == veteran_dragoon_id );

  add( e_unit_type::caravel );
  REQUIRE( f() == veteran_dragoon_id );

  UnitId const cavalry_id = add( e_unit_type::cavalry );
  REQUIRE( f() == cavalry_id );

  UnitId const frigate_id = add( e_unit_type::frigate );
  REQUIRE( f() == frigate_id );
}

TEST_CASE( "[unit-stack] select_native_unit_defender" ) {
  World            W;
  Coord const      coord{ .x = 1, .y = 1 };
  DwellingId const dwelling_id =
      W.add_dwelling( coord, e_tribe::inca ).id;

  auto f = [&] {
    return select_native_unit_defender( W.ss(), coord );
  };

  auto add = [&]( e_native_unit_type type ) {
    return W.add_native_unit_on_map( type, coord, dwelling_id )
        .id;
  };

  NativeUnitId const mounted_brave_id =
      add( e_native_unit_type::mounted_brave );
  REQUIRE( f() == mounted_brave_id );

  add( e_native_unit_type::armed_brave );
  REQUIRE( f() == mounted_brave_id );

  add( e_native_unit_type::brave );
  REQUIRE( f() == mounted_brave_id );

  NativeUnitId const mounted_warrior_id =
      add( e_native_unit_type::mounted_warrior );
  REQUIRE( f() == mounted_warrior_id );
}

TEST_CASE( "[unit-stack] select_colony_defender" ) {
  World       W;
  Coord const coord{ .x = 1, .y = 1 };
  Colony&     colony = W.add_colony( coord );

  auto f = [&] {
    return select_colony_defender( W.ss(), colony );
  };

  UnitId const fisherman_id =
      W.add_unit_outdoors( colony.id, e_direction::s,
                           e_outdoor_job::fish )
          .id();
  REQUIRE( f() == fisherman_id );

  UnitId const cotton_planter_id =
      W.add_unit_outdoors( colony.id, e_direction::sw,
                           e_outdoor_job::cotton )
          .id();
  REQUIRE( f() == cotton_planter_id );

  W.add_unit_outdoors( colony.id, e_direction::se,
                       e_outdoor_job::tobacco );
  REQUIRE( f() == cotton_planter_id );

  UnitId const free_colonist_id =
      W.add_unit_on_map( e_unit_type::free_colonist, coord )
          .id();
  REQUIRE( f() == free_colonist_id );

  W.add_unit_on_map( e_unit_type::frigate, coord );
  REQUIRE( f() == free_colonist_id );

  UnitId const veteran_dragoon_id =
      W.add_unit_on_map( e_unit_type::veteran_dragoon, coord )
          .id();
  REQUIRE( f() == veteran_dragoon_id );

  W.add_unit_on_map( e_unit_type::dragoon, coord );
  REQUIRE( f() == veteran_dragoon_id );
}

} // namespace
} // namespace rn
