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
#include "test/mocking.hpp"
#include "test/mocks/ieuro-agent.hpp"
#include "test/mocks/imap-updater.hpp"
#include "test/util/coro.hpp"

// ss
#include "ss/dwelling.rds.hpp"
#include "ss/map-square.rds.hpp"
#include "ss/native-unit.rds.hpp"
#include "ss/natives.hpp"
#include "ss/ref.hpp"
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

using ::mock::matchers::_;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct world : testing::World {
  using Base = testing::World;
  world() : Base() {
    add_default_player();
    create_default_map();
  }

  void create_default_map() {
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
    vector<MapSquare> tiles{
      _, L, _, L, L, L, //
      L, L, L, L, L, L, //
      _, L, L, L, L, L, //
      L, L, L, L, L, L, //
      L, L, L, L, L, L, //
      L, L, L, L, L, L, //
      L, L, L, L, L, L, //
    };
    build_map( std::move( tiles ), 3 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[tribe-mgr] destroy_dwelling" ) {
  world w;

  Tribe& iroquois         = w.add_tribe( e_tribe::iroquois );
  iroquois.muskets        = 9;
  iroquois.horse_herds    = 8;
  iroquois.horse_breeding = 7;

  DwellingId const dwelling1_id =
      w.add_dwelling( { .x = 1, .y = 1 }, e_tribe::iroquois ).id;
  DwellingId const dwelling2_id =
      w.add_dwelling( { .x = 2, .y = 1 }, e_tribe::iroquois ).id;
  NativeUnitId const brave1_id =
      w.add_native_unit_on_map( e_native_unit_type::brave,
                                { .x = 1, .y = 0 },
                                dwelling1_id )
          .id;
  NativeUnitId const brave2_id =
      w.add_native_unit_on_map( e_native_unit_type::brave,
                                { .x = 1, .y = 0 },
                                dwelling2_id )
          .id;
  NativeUnitId const brave3_id =
      w.add_native_unit_on_map( e_native_unit_type::brave,
                                { .x = 1, .y = 2 },
                                dwelling2_id )
          .id;
  UnitId const missionary_id =
      w.add_missionary_in_dwelling( e_unit_type::missionary,
                                    dwelling1_id,
                                    w.default_player_type() )
          .id();
  w.square( { .x = 1, .y = 1 } ).road = true;
  w.square( { .x = 2, .y = 1 } ).road = true;
  w.natives().mark_land_owned( dwelling1_id,
                               { .x = 1, .y = 0 } );
  w.natives().mark_land_owned( dwelling1_id,
                               { .x = 1, .y = 2 } );
  w.natives().mark_land_owned( dwelling2_id,
                               { .x = 2, .y = 0 } );
  w.natives().mark_land_owned( dwelling2_id,
                               { .x = 2, .y = 2 } );

  auto read_owned_land = [&]( Coord coord ) {
    return base::lookup(
        w.natives().testing_only_owned_land_without_minuit(),
        coord );
  };

  auto destroy = [&]( DwellingId dwelling_id ) {
    destroy_dwelling( w.ss(), w.map_updater(), dwelling_id );
  };

  // Sanity check.
  REQUIRE( w.natives().dwelling_exists( dwelling1_id ) );
  REQUIRE( w.natives().dwelling_exists( dwelling2_id ) );
  REQUIRE( w.units().exists( brave1_id ) );
  REQUIRE( w.units().exists( brave2_id ) );
  REQUIRE( w.units().exists( brave3_id ) );
  REQUIRE( w.units().exists( missionary_id ) );
  REQUIRE( w.square( { .x = 1, .y = 1 } ).road );
  REQUIRE( w.square( { .x = 2, .y = 1 } ).road );
  REQUIRE( read_owned_land( { .x = 1, .y = 0 } ) ==
           dwelling1_id );
  REQUIRE( read_owned_land( { .x = 1, .y = 2 } ) ==
           dwelling1_id );
  REQUIRE( read_owned_land( { .x = 2, .y = 0 } ) ==
           dwelling2_id );
  REQUIRE( read_owned_land( { .x = 2, .y = 2 } ) ==
           dwelling2_id );
  REQUIRE( iroquois.muskets == 9 );
  REQUIRE( iroquois.horse_herds == 8 );
  REQUIRE( iroquois.horse_breeding == 7 );

  destroy( dwelling1_id );

  REQUIRE_FALSE( w.natives().dwelling_exists( dwelling1_id ) );
  REQUIRE( w.natives().dwelling_exists( dwelling2_id ) );
  REQUIRE_FALSE( w.units().exists( brave1_id ) );
  REQUIRE( w.units().exists( brave2_id ) );
  REQUIRE( w.units().exists( brave3_id ) );
  REQUIRE_FALSE( w.units().exists( missionary_id ) );
  REQUIRE_FALSE( w.square( { .x = 1, .y = 1 } ).road );
  REQUIRE( w.square( { .x = 2, .y = 1 } ).road );
  REQUIRE_FALSE( read_owned_land( { .x = 1, .y = 0 } ) ==
                 dwelling1_id );
  REQUIRE_FALSE( read_owned_land( { .x = 1, .y = 2 } ) ==
                 dwelling1_id );
  REQUIRE( read_owned_land( { .x = 2, .y = 0 } ) ==
           dwelling2_id );
  REQUIRE( read_owned_land( { .x = 2, .y = 2 } ) ==
           dwelling2_id );
  REQUIRE( iroquois.muskets == 9 );
  REQUIRE( iroquois.horse_herds == 4 );
  REQUIRE( iroquois.horse_breeding == 4 );

  destroy( dwelling2_id );

  REQUIRE_FALSE( w.natives().dwelling_exists( dwelling1_id ) );
  REQUIRE_FALSE( w.natives().dwelling_exists( dwelling2_id ) );
  REQUIRE_FALSE( w.units().exists( brave1_id ) );
  REQUIRE_FALSE( w.units().exists( brave2_id ) );
  REQUIRE_FALSE( w.units().exists( brave3_id ) );
  REQUIRE_FALSE( w.units().exists( missionary_id ) );
  REQUIRE_FALSE( w.square( { .x = 1, .y = 1 } ).road );
  REQUIRE_FALSE( w.square( { .x = 2, .y = 1 } ).road );
  REQUIRE_FALSE( read_owned_land( { .x = 1, .y = 0 } ) ==
                 dwelling1_id );
  REQUIRE_FALSE( read_owned_land( { .x = 1, .y = 2 } ) ==
                 dwelling1_id );
  REQUIRE_FALSE( read_owned_land( { .x = 2, .y = 0 } ) ==
                 dwelling2_id );
  REQUIRE_FALSE( read_owned_land( { .x = 2, .y = 2 } ) ==
                 dwelling2_id );
  REQUIRE( iroquois.muskets == 0 );
  REQUIRE( iroquois.horse_herds == 0 );
  REQUIRE( iroquois.horse_breeding == 0 );

  REQUIRE( w.natives().tribe_exists( e_tribe::iroquois ) );
  auto dwellings_for_tribe =
      w.natives().dwellings_for_tribe( e_tribe::iroquois );
  REQUIRE( dwellings_for_tribe.empty() );
}

TEST_CASE(
    "[tribe-mgr] clears road after destroying dwelling" ) {
  world w;
  MockIMapUpdater mock_map_updater;

  w.add_tribe( e_tribe::iroquois );
  Coord const kTile{ .x = 1, .y = 1 };

  DwellingId const dwelling_id =
      w.add_dwelling( kTile, e_tribe::iroquois ).id;
  w.square( kTile ).road = true;

  auto destroy = [&]( DwellingId dwelling_id ) {
    destroy_dwelling( w.ss(), mock_map_updater, dwelling_id );
  };

  // Sanity check.
  REQUIRE( w.natives().dwelling_exists( dwelling_id ) );
  REQUIRE( w.square( kTile ).road );

  bool cleared_road = false;
  mock_map_updater.EXPECT__modify_map_square( kTile, _ )
      .invokes( [&] { cleared_road = true; } );

  mock_map_updater
      .EXPECT__force_redraw_tiles( vector<Coord>{ kTile } )
      .returns( vector<BuffersUpdated>{
        BuffersUpdated{ .tile = kTile, .landscape = true } } )
      .invokes( [&] {
        // This ensures that the dwelling has already been de-
        // stroyed and the road removed before we force the re-
        // draw. This way we ensure that we render properly both
        // with respect to the road and any prime resources that
        // might appear after the dwelling is gone.
        REQUIRE_FALSE(
            w.natives().dwelling_exists( dwelling_id ) );
        REQUIRE( cleared_road );
      } );

  destroy( dwelling_id );

  REQUIRE_FALSE( w.natives().dwelling_exists( dwelling_id ) );
}

TEST_CASE( "[tribe-mgr] destroy_tribe" ) {
  world w;

  Tribe& iroquois         = w.add_tribe( e_tribe::iroquois );
  iroquois.muskets        = 9;
  iroquois.horse_herds    = 8;
  iroquois.horse_breeding = 7;

  // Iroquois.
  DwellingId const dwelling1_id =
      w.add_dwelling( { .x = 1, .y = 1 }, e_tribe::iroquois ).id;
  DwellingId const dwelling2_id =
      w.add_dwelling( { .x = 2, .y = 1 }, e_tribe::iroquois ).id;
  NativeUnitId const brave1_id =
      w.add_native_unit_on_map( e_native_unit_type::brave,
                                { .x = 1, .y = 0 },
                                dwelling1_id )
          .id;
  NativeUnitId const brave2_id =
      w.add_native_unit_on_map( e_native_unit_type::brave,
                                { .x = 1, .y = 0 },
                                dwelling2_id )
          .id;
  NativeUnitId const brave3_id =
      w.add_native_unit_on_map( e_native_unit_type::brave,
                                { .x = 1, .y = 2 },
                                dwelling2_id )
          .id;
  UnitId const missionary_id =
      w.add_missionary_in_dwelling( e_unit_type::missionary,
                                    dwelling1_id,
                                    w.default_player_type() )
          .id();
  w.square( { .x = 1, .y = 1 } ).road = true;
  w.square( { .x = 2, .y = 1 } ).road = true;
  w.natives().mark_land_owned( dwelling1_id,
                               { .x = 1, .y = 0 } );
  w.natives().mark_land_owned( dwelling1_id,
                               { .x = 1, .y = 2 } );
  w.natives().mark_land_owned( dwelling2_id,
                               { .x = 2, .y = 0 } );
  w.natives().mark_land_owned( dwelling2_id,
                               { .x = 2, .y = 2 } );

  // Sioux.
  Tribe& sioux         = w.add_tribe( e_tribe::sioux );
  sioux.muskets        = 1000;
  sioux.horse_herds    = 1000;
  sioux.horse_breeding = 1000;
  DwellingId const dwelling3_id =
      w.add_dwelling( { .x = 0, .y = 5 }, e_tribe::sioux ).id;
  NativeUnitId const brave4_id =
      w.add_native_unit_on_map( e_native_unit_type::brave,
                                { .x = 1, .y = 4 },
                                dwelling3_id )
          .id;
  w.square( { .x = 0, .y = 5 } ).road = true;
  w.natives().mark_land_owned( dwelling3_id,
                               { .x = 0, .y = 4 } );
  w.natives().mark_land_owned( dwelling3_id,
                               { .x = 0, .y = 6 } );

  auto read_owned_land = [&]( Coord coord ) {
    return base::lookup(
        w.natives().testing_only_owned_land_without_minuit(),
        coord );
  };

  auto destroy = [&]( e_tribe tribe ) {
    destroy_tribe( w.ss(), w.map_updater(), tribe );
  };

  // Sanity check.
  REQUIRE( w.natives().dwelling_exists( dwelling1_id ) );
  REQUIRE( w.natives().dwelling_exists( dwelling2_id ) );
  REQUIRE( w.units().exists( brave1_id ) );
  REQUIRE( w.units().exists( brave2_id ) );
  REQUIRE( w.units().exists( brave3_id ) );
  REQUIRE( w.units().exists( missionary_id ) );
  REQUIRE( w.square( { .x = 1, .y = 1 } ).road );
  REQUIRE( w.square( { .x = 2, .y = 1 } ).road );
  REQUIRE( read_owned_land( { .x = 1, .y = 0 } ) ==
           dwelling1_id );
  REQUIRE( read_owned_land( { .x = 1, .y = 2 } ) ==
           dwelling1_id );
  REQUIRE( read_owned_land( { .x = 2, .y = 0 } ) ==
           dwelling2_id );
  REQUIRE( read_owned_land( { .x = 2, .y = 2 } ) ==
           dwelling2_id );
  REQUIRE( w.natives().tribe_exists( e_tribe::iroquois ) );
  REQUIRE( w.natives().dwelling_exists( dwelling3_id ) );
  REQUIRE( w.units().exists( brave4_id ) );
  REQUIRE( w.square( { .x = 0, .y = 5 } ).road );
  REQUIRE( read_owned_land( { .x = 0, .y = 4 } ) ==
           dwelling3_id );
  REQUIRE( read_owned_land( { .x = 0, .y = 6 } ) ==
           dwelling3_id );
  REQUIRE( w.natives().tribe_exists( e_tribe::sioux ) );
  REQUIRE_FALSE( w.natives()
                     .dwellings_for_tribe( e_tribe::iroquois )
                     .empty() );
  REQUIRE_FALSE( w.natives()
                     .dwellings_for_tribe( e_tribe::sioux )
                     .empty() );
  REQUIRE( iroquois.muskets == 9 );
  REQUIRE( iroquois.horse_herds == 8 );
  REQUIRE( iroquois.horse_breeding == 7 );

  destroy( e_tribe::iroquois );

  REQUIRE_FALSE( w.natives().dwelling_exists( dwelling1_id ) );
  REQUIRE_FALSE( w.natives().dwelling_exists( dwelling2_id ) );
  REQUIRE_FALSE( w.units().exists( brave1_id ) );
  REQUIRE_FALSE( w.units().exists( brave2_id ) );
  REQUIRE_FALSE( w.units().exists( brave3_id ) );
  REQUIRE_FALSE( w.units().exists( missionary_id ) );
  REQUIRE_FALSE( w.square( { .x = 1, .y = 1 } ).road );
  REQUIRE_FALSE( w.square( { .x = 2, .y = 1 } ).road );
  REQUIRE_FALSE( read_owned_land( { .x = 1, .y = 0 } ) ==
                 dwelling1_id );
  REQUIRE_FALSE( read_owned_land( { .x = 1, .y = 2 } ) ==
                 dwelling1_id );
  REQUIRE_FALSE( read_owned_land( { .x = 2, .y = 0 } ) ==
                 dwelling2_id );
  REQUIRE_FALSE( read_owned_land( { .x = 2, .y = 2 } ) ==
                 dwelling2_id );
  REQUIRE_FALSE( w.natives().tribe_exists( e_tribe::iroquois ) );
  REQUIRE( w.natives().dwelling_exists( dwelling3_id ) );
  REQUIRE( w.units().exists( brave4_id ) );
  REQUIRE( w.square( { .x = 0, .y = 5 } ).road );
  REQUIRE( read_owned_land( { .x = 0, .y = 4 } ) ==
           dwelling3_id );
  REQUIRE( read_owned_land( { .x = 0, .y = 6 } ) ==
           dwelling3_id );
  REQUIRE( w.natives().tribe_exists( e_tribe::sioux ) );
  REQUIRE( w.natives()
               .dwellings_for_tribe( e_tribe::iroquois )
               .empty() );
  REQUIRE_FALSE( w.natives()
                     .dwellings_for_tribe( e_tribe::sioux )
                     .empty() );
  REQUIRE( iroquois.muskets == 0 );
  REQUIRE( iroquois.horse_herds == 0 );
  REQUIRE( iroquois.horse_breeding == 0 );

  // Call again; should be no-op.
  destroy( e_tribe::iroquois );

  destroy( e_tribe::sioux );

  REQUIRE_FALSE( w.natives().dwelling_exists( dwelling1_id ) );
  REQUIRE_FALSE( w.natives().dwelling_exists( dwelling2_id ) );
  REQUIRE_FALSE( w.units().exists( brave1_id ) );
  REQUIRE_FALSE( w.units().exists( brave2_id ) );
  REQUIRE_FALSE( w.units().exists( brave3_id ) );
  REQUIRE_FALSE( w.units().exists( missionary_id ) );
  REQUIRE_FALSE( w.square( { .x = 1, .y = 1 } ).road );
  REQUIRE_FALSE( w.square( { .x = 2, .y = 1 } ).road );
  REQUIRE_FALSE( read_owned_land( { .x = 1, .y = 0 } ) ==
                 dwelling1_id );
  REQUIRE_FALSE( read_owned_land( { .x = 1, .y = 2 } ) ==
                 dwelling1_id );
  REQUIRE_FALSE( read_owned_land( { .x = 2, .y = 0 } ) ==
                 dwelling2_id );
  REQUIRE_FALSE( read_owned_land( { .x = 2, .y = 2 } ) ==
                 dwelling2_id );
  REQUIRE_FALSE( w.natives().tribe_exists( e_tribe::iroquois ) );
  REQUIRE_FALSE( w.natives().dwelling_exists( dwelling3_id ) );
  REQUIRE_FALSE( w.units().exists( brave4_id ) );
  REQUIRE_FALSE( w.square( { .x = 0, .y = 5 } ).road );
  REQUIRE_FALSE( read_owned_land( { .x = 0, .y = 4 } ) ==
                 dwelling3_id );
  REQUIRE_FALSE( read_owned_land( { .x = 0, .y = 6 } ) ==
                 dwelling3_id );
  REQUIRE_FALSE( w.natives().tribe_exists( e_tribe::sioux ) );
  REQUIRE( w.natives()
               .dwellings_for_tribe( e_tribe::iroquois )
               .empty() );
  REQUIRE( w.natives()
               .dwellings_for_tribe( e_tribe::sioux )
               .empty() );
  REQUIRE( sioux.muskets == 0 );
  REQUIRE( sioux.horse_herds == 0 );
  REQUIRE( sioux.horse_breeding == 0 );
  // Call again; should be no-op.
  destroy( e_tribe::sioux );
}

TEST_CASE( "[tribe-mgr] destroy_tribe_interactive" ) {
  world w;
  w.add_tribe( e_tribe::aztec );
  w.euro_agent().EXPECT__message_box(
      "The [Aztec] tribe has been wiped out." );
  co_await_test( destroy_tribe_interactive(
      w.ss(), w.euro_agent(), w.map_updater(),
      e_tribe::aztec ) );
  REQUIRE( !w.natives().tribe_exists( e_tribe::aztec ) );
}

TEST_CASE( "[tribe-mgr] tribe_wiped_out_message" ) {
  world w;
  w.add_tribe( e_tribe::aztec );
  w.euro_agent().EXPECT__message_box(
      "The [Aztec] tribe has been wiped out." );
  co_await_test( tribe_wiped_out_message( w.euro_agent(),
                                          e_tribe::aztec ) );
  REQUIRE( w.natives().tribe_exists( e_tribe::aztec ) );
}

TEST_CASE( "[tribe-mgr] tribe_for_dwelling" ) {
  world w;
  Dwelling dwelling;
  e_tribe expected = {};

  auto f = [&] {
    return tribe_type_for_dwelling( w.ss(), dwelling );
  };

  // Test with a frozen dwelling.
  dwelling = {
    .id     = 0,
    .frozen = FrozenDwelling{ .tribe = e_tribe::cherokee } };
  expected = e_tribe::cherokee;
  REQUIRE( f() == expected );

  dwelling = w.add_dwelling( { .x = 1, .y = 2 }, e_tribe::tupi );
  expected = e_tribe::tupi;
  REQUIRE( f() == expected );

  dwelling =
      w.add_dwelling( { .x = 1, .y = 0 }, e_tribe::aztec );
  expected = e_tribe::aztec;
  REQUIRE( f() == expected );
}

} // namespace
} // namespace rn
