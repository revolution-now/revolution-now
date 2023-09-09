/****************************************************************
**tribe-mgr.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-02-06.
*
* Description: Unit tests for the src/tribe-mgr.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/tribe-mgr.hpp"

// Testing
#include "test/fake/world.hpp"
#include "test/mocks/igui.hpp"

// ss
#include "ss/dwelling.rds.hpp"
#include "ss/map-square.rds.hpp"
#include "ss/native-unit.rds.hpp"
#include "ss/natives.hpp"
#include "ss/unit-type.hpp"
#include "ss/unit.hpp"
#include "ss/units.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/keyval.hpp"
#include "base/to-str-ext-std.hpp"

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
TEST_CASE( "[tribe-mgr] destroy_dwelling" ) {
  World W;

  DwellingId const dwelling1_id =
      W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::iroquois ).id;
  DwellingId const dwelling2_id =
      W.add_dwelling( { .x = 2, .y = 1 }, e_tribe::iroquois ).id;
  NativeUnitId const brave1_id =
      W.add_native_unit_on_map( e_native_unit_type::brave,
                                { .x = 1, .y = 0 },
                                dwelling1_id )
          .id;
  NativeUnitId const brave2_id =
      W.add_native_unit_on_map( e_native_unit_type::brave,
                                { .x = 1, .y = 0 },
                                dwelling2_id )
          .id;
  NativeUnitId const brave3_id =
      W.add_native_unit_on_map( e_native_unit_type::brave,
                                { .x = 1, .y = 2 },
                                dwelling2_id )
          .id;
  UnitId const missionary_id =
      W.add_missionary_in_dwelling( e_unit_type::missionary,
                                    dwelling1_id,
                                    W.default_nation() )
          .id();
  W.square( { .x = 1, .y = 1 } ).road = true;
  W.square( { .x = 2, .y = 1 } ).road = true;
  W.natives().mark_land_owned( dwelling1_id,
                               { .x = 1, .y = 0 } );
  W.natives().mark_land_owned( dwelling1_id,
                               { .x = 1, .y = 2 } );
  W.natives().mark_land_owned( dwelling2_id,
                               { .x = 2, .y = 0 } );
  W.natives().mark_land_owned( dwelling2_id,
                               { .x = 2, .y = 2 } );

  auto read_owned_land = [&]( Coord coord ) {
    return base::lookup(
        W.natives().testing_only_owned_land_without_minuit(),
        coord );
  };

  auto destroy = [&]( DwellingId dwelling_id ) {
    destroy_dwelling( W.ss(), W.ts(), dwelling_id );
  };

  // Sanity check.
  REQUIRE( W.natives().dwelling_exists( dwelling1_id ) );
  REQUIRE( W.natives().dwelling_exists( dwelling2_id ) );
  REQUIRE( W.units().exists( brave1_id ) );
  REQUIRE( W.units().exists( brave2_id ) );
  REQUIRE( W.units().exists( brave3_id ) );
  REQUIRE( W.units().exists( missionary_id ) );
  REQUIRE( W.square( { .x = 1, .y = 1 } ).road );
  REQUIRE( W.square( { .x = 2, .y = 1 } ).road );
  REQUIRE( read_owned_land( { .x = 1, .y = 0 } ) ==
           dwelling1_id );
  REQUIRE( read_owned_land( { .x = 1, .y = 2 } ) ==
           dwelling1_id );
  REQUIRE( read_owned_land( { .x = 2, .y = 0 } ) ==
           dwelling2_id );
  REQUIRE( read_owned_land( { .x = 2, .y = 2 } ) ==
           dwelling2_id );

  destroy( dwelling1_id );

  REQUIRE_FALSE( W.natives().dwelling_exists( dwelling1_id ) );
  REQUIRE( W.natives().dwelling_exists( dwelling2_id ) );
  REQUIRE_FALSE( W.units().exists( brave1_id ) );
  REQUIRE( W.units().exists( brave2_id ) );
  REQUIRE( W.units().exists( brave3_id ) );
  REQUIRE_FALSE( W.units().exists( missionary_id ) );
  REQUIRE_FALSE( W.square( { .x = 1, .y = 1 } ).road );
  REQUIRE( W.square( { .x = 2, .y = 1 } ).road );
  REQUIRE_FALSE( read_owned_land( { .x = 1, .y = 0 } ) ==
                 dwelling1_id );
  REQUIRE_FALSE( read_owned_land( { .x = 1, .y = 2 } ) ==
                 dwelling1_id );
  REQUIRE( read_owned_land( { .x = 2, .y = 0 } ) ==
           dwelling2_id );
  REQUIRE( read_owned_land( { .x = 2, .y = 2 } ) ==
           dwelling2_id );

  destroy( dwelling2_id );

  REQUIRE_FALSE( W.natives().dwelling_exists( dwelling1_id ) );
  REQUIRE_FALSE( W.natives().dwelling_exists( dwelling2_id ) );
  REQUIRE_FALSE( W.units().exists( brave1_id ) );
  REQUIRE_FALSE( W.units().exists( brave2_id ) );
  REQUIRE_FALSE( W.units().exists( brave3_id ) );
  REQUIRE_FALSE( W.units().exists( missionary_id ) );
  REQUIRE_FALSE( W.square( { .x = 1, .y = 1 } ).road );
  REQUIRE_FALSE( W.square( { .x = 2, .y = 1 } ).road );
  REQUIRE_FALSE( read_owned_land( { .x = 1, .y = 0 } ) ==
                 dwelling1_id );
  REQUIRE_FALSE( read_owned_land( { .x = 1, .y = 2 } ) ==
                 dwelling1_id );
  REQUIRE_FALSE( read_owned_land( { .x = 2, .y = 0 } ) ==
                 dwelling2_id );
  REQUIRE_FALSE( read_owned_land( { .x = 2, .y = 2 } ) ==
                 dwelling2_id );

  REQUIRE( W.natives().tribe_exists( e_tribe::iroquois ) );
  auto dwellings_for_tribe =
      W.natives().dwellings_for_tribe( e_tribe::iroquois );
  REQUIRE( dwellings_for_tribe.has_value() );
  REQUIRE( dwellings_for_tribe->empty() );
}

TEST_CASE( "[tribe-mgr] destroy_tribe" ) {
  World W;

  // Iroquois.
  DwellingId const dwelling1_id =
      W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::iroquois ).id;
  DwellingId const dwelling2_id =
      W.add_dwelling( { .x = 2, .y = 1 }, e_tribe::iroquois ).id;
  NativeUnitId const brave1_id =
      W.add_native_unit_on_map( e_native_unit_type::brave,
                                { .x = 1, .y = 0 },
                                dwelling1_id )
          .id;
  NativeUnitId const brave2_id =
      W.add_native_unit_on_map( e_native_unit_type::brave,
                                { .x = 1, .y = 0 },
                                dwelling2_id )
          .id;
  NativeUnitId const brave3_id =
      W.add_native_unit_on_map( e_native_unit_type::brave,
                                { .x = 1, .y = 2 },
                                dwelling2_id )
          .id;
  UnitId const missionary_id =
      W.add_missionary_in_dwelling( e_unit_type::missionary,
                                    dwelling1_id,
                                    W.default_nation() )
          .id();
  W.square( { .x = 1, .y = 1 } ).road = true;
  W.square( { .x = 2, .y = 1 } ).road = true;
  W.natives().mark_land_owned( dwelling1_id,
                               { .x = 1, .y = 0 } );
  W.natives().mark_land_owned( dwelling1_id,
                               { .x = 1, .y = 2 } );
  W.natives().mark_land_owned( dwelling2_id,
                               { .x = 2, .y = 0 } );
  W.natives().mark_land_owned( dwelling2_id,
                               { .x = 2, .y = 2 } );

  // Sioux.
  DwellingId const dwelling3_id =
      W.add_dwelling( { .x = 0, .y = 1 }, e_tribe::sioux ).id;
  NativeUnitId const brave4_id =
      W.add_native_unit_on_map( e_native_unit_type::brave,
                                { .x = 1, .y = 0 },
                                dwelling3_id )
          .id;
  W.square( { .x = 0, .y = 1 } ).road = true;
  W.natives().mark_land_owned( dwelling3_id,
                               { .x = 0, .y = 0 } );
  W.natives().mark_land_owned( dwelling3_id,
                               { .x = 0, .y = 2 } );

  auto read_owned_land = [&]( Coord coord ) {
    return base::lookup(
        W.natives().testing_only_owned_land_without_minuit(),
        coord );
  };

  auto destroy = [&]( e_tribe tribe ) {
    destroy_tribe( W.ss(), W.ts(), tribe );
  };

  // Sanity check.
  REQUIRE( W.natives().dwelling_exists( dwelling1_id ) );
  REQUIRE( W.natives().dwelling_exists( dwelling2_id ) );
  REQUIRE( W.units().exists( brave1_id ) );
  REQUIRE( W.units().exists( brave2_id ) );
  REQUIRE( W.units().exists( brave3_id ) );
  REQUIRE( W.units().exists( missionary_id ) );
  REQUIRE( W.square( { .x = 1, .y = 1 } ).road );
  REQUIRE( W.square( { .x = 2, .y = 1 } ).road );
  REQUIRE( read_owned_land( { .x = 1, .y = 0 } ) ==
           dwelling1_id );
  REQUIRE( read_owned_land( { .x = 1, .y = 2 } ) ==
           dwelling1_id );
  REQUIRE( read_owned_land( { .x = 2, .y = 0 } ) ==
           dwelling2_id );
  REQUIRE( read_owned_land( { .x = 2, .y = 2 } ) ==
           dwelling2_id );
  REQUIRE( W.natives().tribe_exists( e_tribe::iroquois ) );
  REQUIRE( W.natives().dwelling_exists( dwelling3_id ) );
  REQUIRE( W.units().exists( brave4_id ) );
  REQUIRE( W.square( { .x = 0, .y = 1 } ).road );
  REQUIRE( read_owned_land( { .x = 0, .y = 0 } ) ==
           dwelling3_id );
  REQUIRE( read_owned_land( { .x = 0, .y = 2 } ) ==
           dwelling3_id );
  REQUIRE( W.natives().tribe_exists( e_tribe::sioux ) );
  REQUIRE( W.natives().dwellings_for_tribe(
               e_tribe::iroquois ) != nothing );
  REQUIRE( W.natives().dwellings_for_tribe( e_tribe::sioux ) !=
           nothing );

  destroy( e_tribe::iroquois );

  REQUIRE_FALSE( W.natives().dwelling_exists( dwelling1_id ) );
  REQUIRE_FALSE( W.natives().dwelling_exists( dwelling2_id ) );
  REQUIRE_FALSE( W.units().exists( brave1_id ) );
  REQUIRE_FALSE( W.units().exists( brave2_id ) );
  REQUIRE_FALSE( W.units().exists( brave3_id ) );
  REQUIRE_FALSE( W.units().exists( missionary_id ) );
  REQUIRE_FALSE( W.square( { .x = 1, .y = 1 } ).road );
  REQUIRE_FALSE( W.square( { .x = 2, .y = 1 } ).road );
  REQUIRE_FALSE( read_owned_land( { .x = 1, .y = 0 } ) ==
                 dwelling1_id );
  REQUIRE_FALSE( read_owned_land( { .x = 1, .y = 2 } ) ==
                 dwelling1_id );
  REQUIRE_FALSE( read_owned_land( { .x = 2, .y = 0 } ) ==
                 dwelling2_id );
  REQUIRE_FALSE( read_owned_land( { .x = 2, .y = 2 } ) ==
                 dwelling2_id );
  REQUIRE_FALSE( W.natives().tribe_exists( e_tribe::iroquois ) );
  REQUIRE( W.natives().dwelling_exists( dwelling3_id ) );
  REQUIRE( W.units().exists( brave4_id ) );
  REQUIRE( W.square( { .x = 0, .y = 1 } ).road );
  REQUIRE( read_owned_land( { .x = 0, .y = 0 } ) ==
           dwelling3_id );
  REQUIRE( read_owned_land( { .x = 0, .y = 2 } ) ==
           dwelling3_id );
  REQUIRE( W.natives().tribe_exists( e_tribe::sioux ) );
  REQUIRE( W.natives().dwellings_for_tribe(
               e_tribe::iroquois ) == nothing );
  REQUIRE( W.natives().dwellings_for_tribe( e_tribe::sioux ) !=
           nothing );
  // Call again; should be no-op.
  destroy( e_tribe::iroquois );

  destroy( e_tribe::sioux );

  REQUIRE_FALSE( W.natives().dwelling_exists( dwelling1_id ) );
  REQUIRE_FALSE( W.natives().dwelling_exists( dwelling2_id ) );
  REQUIRE_FALSE( W.units().exists( brave1_id ) );
  REQUIRE_FALSE( W.units().exists( brave2_id ) );
  REQUIRE_FALSE( W.units().exists( brave3_id ) );
  REQUIRE_FALSE( W.units().exists( missionary_id ) );
  REQUIRE_FALSE( W.square( { .x = 1, .y = 1 } ).road );
  REQUIRE_FALSE( W.square( { .x = 2, .y = 1 } ).road );
  REQUIRE_FALSE( read_owned_land( { .x = 1, .y = 0 } ) ==
                 dwelling1_id );
  REQUIRE_FALSE( read_owned_land( { .x = 1, .y = 2 } ) ==
                 dwelling1_id );
  REQUIRE_FALSE( read_owned_land( { .x = 2, .y = 0 } ) ==
                 dwelling2_id );
  REQUIRE_FALSE( read_owned_land( { .x = 2, .y = 2 } ) ==
                 dwelling2_id );
  REQUIRE_FALSE( W.natives().tribe_exists( e_tribe::iroquois ) );
  REQUIRE_FALSE( W.natives().dwelling_exists( dwelling3_id ) );
  REQUIRE_FALSE( W.units().exists( brave4_id ) );
  REQUIRE_FALSE( W.square( { .x = 0, .y = 1 } ).road );
  REQUIRE_FALSE( read_owned_land( { .x = 0, .y = 0 } ) ==
                 dwelling3_id );
  REQUIRE_FALSE( read_owned_land( { .x = 0, .y = 2 } ) ==
                 dwelling3_id );
  REQUIRE_FALSE( W.natives().tribe_exists( e_tribe::sioux ) );
  REQUIRE( W.natives().dwellings_for_tribe(
               e_tribe::iroquois ) == nothing );
  REQUIRE( W.natives().dwellings_for_tribe( e_tribe::sioux ) ==
           nothing );
  // Call again; should be no-op.
  destroy( e_tribe::sioux );
}

TEST_CASE( "[tribe-mgr] destroy_tribe_interactive" ) {
  World W;
  W.add_tribe( e_tribe::aztec );
  W.gui()
      .EXPECT__message_box(
          "The [Aztec] tribe has been wiped out." )
      .returns();
  wait<> const w = destroy_tribe_interactive( W.ss(), W.ts(),
                                              e_tribe::aztec );
  REQUIRE( !w.exception() );
  REQUIRE( w.ready() );
}

} // namespace
} // namespace rn
