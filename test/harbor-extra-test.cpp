/****************************************************************
**harbor-extra-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-02-16.
*
* Description: Unit tests for the harbor-extra module.
*
*****************************************************************/
#include "harbor-extra.hpp"
#include "test/testing.hpp"

// Under test.
#include "src/harbor-extra.hpp"

// Testing.
#include "test/fake/world.hpp"

// ss
#include "src/ss/players.rds.hpp"
#include "src/ss/ref.hpp"
#include "src/ss/unit.hpp"

// refl
#include "src/refl/to-str.hpp"

// base
#include "src/base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

using ::gfx::point;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct world : testing::World {
  world() {
    add_default_player();
    create_default_map();
  }

  void create_default_map() {
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
    vector<MapSquare> tiles{
      L, L, L, //
      L, _, L, //
      L, L, L, //
    };
    build_map( std::move( tiles ), 3 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[harbor-extra] some test" ) {
  world w;
  UnitId unit_id = {};
  HarborUnloadables expected;

  auto const f = [&] {
    return find_unloadable_slots_in_harbor( w.ss(), unit_id );
  };

  Unit const& free_colonist =
      w.add_unit_on_map( e_unit_type::free_colonist, point{} );
  unit_id  = free_colonist.id();
  expected = {};
  REQUIRE( f() == expected );

  Unit& galleon = w.add_unit_in_port( e_unit_type::galleon );
  unit_id       = galleon.id();
  expected      = {};
  REQUIRE( f() == expected );

  w.set_current_bid_price( e_commodity::ore, 5 );
  w.set_current_bid_price( e_commodity::silver, 19 );
  w.set_current_bid_price( e_commodity::food, 0 );
  w.set_current_bid_price( e_commodity::muskets, 6 );
  w.set_current_bid_price( e_commodity::trade_goods, 3 );

  // pre-tax value = 5*50 = 250.
  add_commodity_to_cargo(
      w.units(),
      Commodity{ .type = e_commodity::ore, .quantity = 50 },
      galleon.cargo(), /*slot=*/0,
      /*try_other_slots=*/false );

  // pre-tax value = 19*1 = 19.
  add_commodity_to_cargo(
      w.units(),
      Commodity{ .type = e_commodity::silver, .quantity = 1 },
      galleon.cargo(), /*slot=*/1,
      /*try_other_slots=*/false );

  // pre-tax value = 0*100 = 0.
  add_commodity_to_cargo(
      w.units(),
      Commodity{ .type = e_commodity::food, .quantity = 100 },
      galleon.cargo(), /*slot=*/2,
      /*try_other_slots=*/false );

  // pre-tax value = 6*100 = 600.
  add_commodity_to_cargo(
      w.units(),
      Commodity{ .type = e_commodity::muskets, .quantity = 100 },
      galleon.cargo(), /*slot=*/4,
      /*try_other_slots=*/false );

  // pre-tax value = 3*100 = 300.
  add_commodity_to_cargo(
      w.units(),
      Commodity{ .type     = e_commodity::trade_goods,
                 .quantity = 100 },
      galleon.cargo(), /*slot=*/5,
      /*try_other_slots=*/false );

  unit_id        = galleon.id();
  expected.items = {
    HarborUnloadable{
      .slot    = 2,
      .comm    = Commodity{ .type     = e_commodity::food,
                            .quantity = 100 },
      .boycott = false,
    },
    HarborUnloadable{
      .slot    = 1,
      .comm    = Commodity{ .type     = e_commodity::silver,
                            .quantity = 1 },
      .boycott = false,
    },
    HarborUnloadable{
      .slot = 0,
      .comm =
          Commodity{ .type = e_commodity::ore, .quantity = 50 },
      .boycott = false,
    },
    HarborUnloadable{
      .slot    = 5,
      .comm    = Commodity{ .type     = e_commodity::trade_goods,
                            .quantity = 100 },
      .boycott = false,
    },
    HarborUnloadable{
      .slot    = 4,
      .comm    = Commodity{ .type     = e_commodity::muskets,
                            .quantity = 100 },
      .boycott = false,
    },
  };
  REQUIRE( f() == expected );

  w.add_unit_in_cargo( e_unit_type::free_colonist,
                       galleon.id() );
  REQUIRE( f() == expected );

  w.default_player()
      .old_world.market.commodities[e_commodity::muskets]
      .boycott = true;

  expected.items[4].boycott = true;
  REQUIRE( f() == expected );
}

} // namespace
} // namespace rn
