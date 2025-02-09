/****************************************************************
**units.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-12-21.
*
* Description: Unit tests for the src/ss/units.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/ss/units.hpp"

// Testing
#include "test/fake/world.hpp"

// ss
#include "src/ss/dwelling.rds.hpp"

// refl
#include "src/refl/to-str.hpp"

// base
#include "src/base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {

void testing_friend_disown_unit( UnitsState& units,
                                 UnitId const id ) {
  units.disown_unit( id );
}

void testing_friend_destroy_unit( UnitsState& units,
                                  UnitId const id ) {
  units.destroy_unit( id );
}

namespace {

using namespace std;

using ::base::valid;
using ::Catch::Matchers::Contains;
using ::gfx::point;

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
TEST_CASE( "[units] dwelling_for" ) {
  World W;

  Dwelling const& dwelling1 =
      W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::apache );
  Dwelling const& dwelling2 =
      W.add_dwelling( { .x = 1, .y = 2 }, e_tribe::inca );
  NativeUnit const& unit_id1 = W.add_native_unit_on_map(
      e_native_unit_type::armed_brave, { .x = 0, .y = 0 },
      dwelling1.id );
  NativeUnit const& unit_id2 = W.add_native_unit_on_map(
      e_native_unit_type::brave, { .x = 0, .y = 1 },
      dwelling2.id );

  REQUIRE( W.units().dwelling_for( unit_id1.id ) ==
           dwelling1.id );
  REQUIRE( W.units().dwelling_for( unit_id2.id ) ==
           dwelling2.id );
}

TEST_CASE( "[units] braves_for_dwelling" ) {
  World W;
  unordered_set<NativeUnitId> expected;

  Dwelling const& dwelling1 =
      W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::apache );
  Dwelling const& dwelling2 =
      W.add_dwelling( { .x = 1, .y = 2 }, e_tribe::inca );
  NativeUnit const& unit_id1 = W.add_native_unit_on_map(
      e_native_unit_type::armed_brave, { .x = 0, .y = 0 },
      dwelling1.id );
  NativeUnit const& unit_id2 = W.add_native_unit_on_map(
      e_native_unit_type::brave, { .x = 0, .y = 1 },
      dwelling2.id );
  NativeUnit const& unit_id3 = W.add_native_unit_on_map(
      e_native_unit_type::brave, { .x = 0, .y = 1 },
      dwelling2.id );

  expected = { unit_id1.id };
  REQUIRE( W.units().braves_for_dwelling( dwelling1.id ) ==
           expected );

  expected = { unit_id2.id, unit_id3.id };
  REQUIRE( W.units().braves_for_dwelling( dwelling2.id ) ==
           expected );

  expected = {};
  REQUIRE( W.units().braves_for_dwelling( DwellingId{ 3 } ) ==
           expected );
}

TEST_CASE( "[units] validation" ) {
  World W;
  base::valid_or<string> v = valid;

  REQUIRE( W.units().validate() == valid );

  // Test that a ship in port fails validation if it does not
  // have cleared orders.
  Unit& caravel = W.add_unit_in_port( e_unit_type::caravel );
  REQUIRE( W.units().validate() == valid );
  Unit& free_colonist =
      W.add_unit_in_port( e_unit_type::free_colonist );
  REQUIRE( W.units().validate() == valid );
  free_colonist.sentry();
  REQUIRE( W.units().validate() == valid );
  caravel.orders() = unit_orders::damaged{};
  REQUIRE( W.units().validate() == valid );
  caravel.sentry();
  v = W.units().validate();
  REQUIRE( v != valid );
  REQUIRE_THAT( v.error(),
                Contains( "unit 1 in port is not damaged but "
                          "does not have cleared orders." ) );
}

TEST_CASE( "[units] units added/removed from ordering map" ) {
  World W;

  map<UnitId, int64_t> expected;

  REQUIRE( W.units().refl().unit_ordering.empty() );

  UnitId const in_port_id =
      W.add_unit_in_port( e_unit_type::merchantman ).id();
  expected = {
    { UnitId{ 1 }, 1 },
  };
  REQUIRE( W.units().refl().unit_ordering == expected );
  REQUIRE( W.units().unit_ordering( in_port_id ) == 1 );

  UnitId const in_cargo_id =
      W.add_unit_in_cargo( e_unit_type::free_colonist,
                           in_port_id )
          .id();
  expected = {
    { UnitId{ 1 }, 1 },
  };
  REQUIRE( W.units().refl().unit_ordering == expected );
  REQUIRE( W.units().unit_ordering( in_port_id ) == 1 );

  UnitId const on_map_id =
      W.add_unit_on_map( e_unit_type::free_colonist,
                         point{ .x = 1, .y = 1 } )
          .id();
  expected = {
    { UnitId{ 1 }, 1 },
    { UnitId{ 3 }, 2 },
  };
  REQUIRE( W.units().refl().unit_ordering == expected );
  REQUIRE( W.units().unit_ordering( in_port_id ) == 1 );
  REQUIRE( W.units().unit_ordering( on_map_id ) == 2 );

  testing_friend_disown_unit( W.units(), in_cargo_id );
  expected = {
    { UnitId{ 1 }, 1 },
    { UnitId{ 3 }, 2 },
  };
  REQUIRE( W.units().refl().unit_ordering == expected );
  REQUIRE( W.units().unit_ordering( in_port_id ) == 1 );
  REQUIRE( W.units().unit_ordering( on_map_id ) == 2 );

  W.units().bump_unit_ordering( UnitId{ 1 } );
  expected = {
    { UnitId{ 1 }, 3 },
    { UnitId{ 3 }, 2 },
  };
  REQUIRE( W.units().refl().unit_ordering == expected );
  REQUIRE( W.units().unit_ordering( in_port_id ) == 3 );
  REQUIRE( W.units().unit_ordering( on_map_id ) == 2 );

  W.units().bump_unit_ordering( UnitId{ 3 } );
  expected = {
    { UnitId{ 1 }, 3 },
    { UnitId{ 3 }, 4 },
  };
  REQUIRE( W.units().refl().unit_ordering == expected );
  REQUIRE( W.units().unit_ordering( in_port_id ) == 3 );
  REQUIRE( W.units().unit_ordering( on_map_id ) == 4 );

  testing_friend_disown_unit( W.units(), in_port_id );
  expected = {
    { UnitId{ 3 }, 4 },
  };
  REQUIRE( W.units().refl().unit_ordering == expected );
  REQUIRE( W.units().unit_ordering( on_map_id ) == 4 );

  // This one uses "destroy" instead of "disown".
  testing_friend_destroy_unit( W.units(), on_map_id );
  expected = {};
  REQUIRE( W.units().refl().unit_ordering == expected );

  // ============================================================
  // Validation: This should be last.
  // ============================================================
  // Need to get rid of these guys because they are in the "free"
  // state which is not allowed during validation.
  testing_friend_destroy_unit( W.units(), in_port_id );
  testing_friend_destroy_unit( W.units(), in_cargo_id );
  REQUIRE( W.units().refl().validate() == valid );
}

} // namespace
} // namespace rn
