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
    add( e_unit_type::soldier ); // 1
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
    expected = { UnitId{ 9 }, UnitId{ 8 }, UnitId{ 10 },
                 UnitId{ 7 }, UnitId{ 4 }, UnitId{ 6 },
                 UnitId{ 5 }, UnitId{ 3 }, UnitId{ 2 },
                 UnitId{ 1 } };
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
    expected = { NativeUnitId{ 2 }, NativeUnitId{ 4 },
                 NativeUnitId{ 1 }, NativeUnitId{ 5 },
                 NativeUnitId{ 3 } };
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
    sort_unit_stack( W.ss(), units, /*force_top=*/nothing );
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
    expected = { GenericUnitId{ 5 }, GenericUnitId{ 4 },
                 GenericUnitId{ 2 }, GenericUnitId{ 6 },
                 GenericUnitId{ 1 }, GenericUnitId{ 7 },
                 GenericUnitId{ 3 } };
    sort_unit_stack( W.ss(), units, /*force_top=*/nothing );
    REQUIRE( units == expected );
  }

  SECTION( "multiple with forced" ) {
    add( e_native_unit_type::mounted_brave );   // 1
    add( e_native_unit_type::mounted_warrior ); // 2
    add( e_native_unit_type::brave );           // 3
    add( e_unit_type::veteran_soldier );        // 4
    add( e_unit_type::caravel );                // 5
    add( e_native_unit_type::armed_brave );     // 6
    add( e_native_unit_type::brave );           // 7
    add( e_native_unit_type::brave );           // 8
    expected = { GenericUnitId{ 7 }, GenericUnitId{ 5 },
                 GenericUnitId{ 4 }, GenericUnitId{ 2 },
                 GenericUnitId{ 6 }, GenericUnitId{ 1 },
                 GenericUnitId{ 8 }, GenericUnitId{ 3 } };
    sort_unit_stack( W.ss(), units,
                     /*force_top=*/GenericUnitId{ 7 } );
    REQUIRE( units == expected );
  }
}

} // namespace
} // namespace rn
