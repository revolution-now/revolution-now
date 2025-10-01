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
struct world : testing::World {
  using Base = testing::World;
  world() : Base() {
    add_player( e_player::english );
    add_player( e_player::french );
    set_default_player_type( e_player::french );
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
  world W;

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
  world W;
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

TEST_CASE(
    "[units] validation: ship in port has cleared orders" ) {
  world W;
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

TEST_CASE(
    "[units] validation: units on map don't mix kinds or "
    "nations on the same tile" ) {
  world w;
  base::valid_or<string> v = valid;

  REQUIRE( w.units().validate() == valid );

  w.add_unit_on_map( e_unit_type::free_colonist,
                     { .x = 0, .y = 0 }, e_player::french );
  REQUIRE( w.units().validate() == valid );

  SECTION( "mixes kind" ) {
    w.add_dwelling_and_brave( { .x = 0, .y = 0 },
                              e_tribe::iroquois );
    v = w.units().validate();
    REQUIRE( v != valid );
    REQUIRE_THAT( v.error(),
                  Contains( "mixes units of different kinds" ) );
  }

  SECTION( "mixes nations" ) {
    w.add_unit_on_map( e_unit_type::free_colonist,
                       { .x = 0, .y = 0 }, e_player::english );
    v = w.units().validate();
    REQUIRE( v != valid );
    REQUIRE_THAT(
        v.error(),
        Contains(
            "mixes european units with different players" ) );
  }
}

// It's seems that it's tricky to get things into the invalid
// state that we need to test by way of standard APIs, so we have
// to build the underlying wrapped state directly.
TEST_CASE(
    "[units] validation: only ships can be inbound/outboubnd" ) {
  world w;
  base::valid_or<string> v = valid;

  SECTION( "default" ) {
    wrapped::UnitsState o;
    UnitsState const units_state( std::move( o ) );
    REQUIRE( units_state.validate() == valid );
  }

  SECTION( "inbound" ) {
    wrapped::UnitsState o;
    wrapped::Unit o_unit;
    o_unit.id                   = UnitId{ 1 };
    o_unit.composition          = e_unit_type::free_colonist;
    o.units[GenericUnitId{ 1 }] = UnitState::euro{
      .unit      = Unit( std::move( o_unit ) ),
      .ownership = UnitOwnership::harbor{
        .port_status = PortStatus::inbound{ .turns = 1 } } };
    UnitsState const units_state( std::move( o ) );
    v = units_state.validate();
    REQUIRE( v != valid );
    REQUIRE_THAT( v.error(),
                  Contains( "`inbound` state, but that state is "
                            "reserved only for ships" ) );
  }

  SECTION( "outbound" ) {
    wrapped::UnitsState o;
    wrapped::Unit o_unit;
    o_unit.id                   = UnitId{ 1 };
    o_unit.composition          = e_unit_type::free_colonist;
    o.units[GenericUnitId{ 1 }] = UnitState::euro{
      .unit      = Unit( std::move( o_unit ) ),
      .ownership = UnitOwnership::harbor{
        .port_status = PortStatus::outbound{ .turns = 1 } } };
    UnitsState const units_state( std::move( o ) );
    v = units_state.validate();
    REQUIRE( v != valid );
    REQUIRE_THAT( v.error(),
                  Contains( "`outbound` state, but that state "
                            "is reserved only for ships" ) );
  }
}

TEST_CASE( "[units] units added/removed from ordering map" ) {
  world W;

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

TEST_CASE(
    "[units] validation: units in holders have holders that "
    "exist" ) {
  world w;
  base::valid_or<string> v = valid;

  REQUIRE( w.units().validate() == valid );

  UnitId const caravel_id =
      w.add_unit_on_map( e_unit_type::caravel,
                         { .x = 0, .y = 0 } )
          .id();
  REQUIRE( w.units().validate() == valid );

  w.add_unit_in_cargo( e_unit_type::free_colonist, caravel_id );
  REQUIRE( w.units().validate() == valid );

  testing_friend_destroy_unit( w.units(), caravel_id );

  v = w.units().validate();
  REQUIRE( v != valid );
  REQUIRE_THAT(
      v.error(),
      Contains( "unit 2 is being held in the cargo of unit 1 "
                "but the latter unit id does not exist." ) );
}

TEST_CASE(
    "[units] validation: units in goto mode must be on the map "
    "or indirectly on the map" ) {
  world w;
  base::valid_or<string> v = valid;

  REQUIRE( w.units().validate() == valid );

  Colony& colony = w.add_colony( { .x = 1, .y = 0 } );
  Unit& statesman =
      w.add_unit_indoors( colony.id, e_indoor_job::bells );
  Unit& caravel  = w.add_unit_on_map( e_unit_type::caravel,
                                      { .x = 0, .y = 0 } );
  Unit& colonist = w.add_unit_in_cargo(
      e_unit_type::free_colonist, caravel.id() );

  REQUIRE( w.units().validate() == valid );

  caravel.orders() = unit_orders::go_to{};
  REQUIRE( w.units().validate() == valid );

  colonist.orders() = unit_orders::go_to{};
  REQUIRE( w.units().validate() == valid );

  statesman.orders() = unit_orders::go_to{};
  v                  = w.units().validate();
  REQUIRE( v != valid );
  REQUIRE_THAT(
      v.error(),
      Contains( "unit 1 is in goto mode but is not directly or "
                "indirectly on the map." ) );
}

} // namespace
} // namespace rn
