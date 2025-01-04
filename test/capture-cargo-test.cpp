/****************************************************************
**capture-cargo-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-01-04.
*
* Description: Unit tests for the capture-cargo module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/capture-cargo.hpp"

// Testing.
#include "test/fake/world.hpp"

// ss
#include "src/ss/cargo.hpp"
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

  void add_cargo( wrapped::CargoHold& cargo, Commodity const& c,
                  int const slot ) {
    auto& slots = cargo.slots;
    if( slot >= ssize( slots ) ) slots.resize( slot + 1 );
    slots[slot]
        .emplace<CargoSlot::cargo>()
        .contents.emplace<Cargo::commodity>()
        .obj = c;
    ;
  }

  void add_cargo( wrapped::CargoHold& cargo,
                  UnitId const unit_id, int const slot ) {
    auto& slots = cargo.slots;
    if( slot >= ssize( slots ) ) slots.resize( slot + 1 );
    slots[slot]
        .emplace<CargoSlot::cargo>()
        .contents.emplace<Cargo::unit>()
        .id = unit_id;
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[capture-cargo] capturable_cargo_items" ) {
  world w;

  CapturableCargo expected;
  wrapped::CargoHold src;
  wrapped::CargoHold dst;

  auto f = [&] {
    return capturable_cargo_items( w.ss(),
                                   CargoHold( auto( src ) ),
                                   CargoHold( auto( dst ) ) );
  };

  using enum e_commodity;

  SECTION( "default" ) {
    expected = {};
    REQUIRE( f() == expected );
  }

  SECTION( "src=[c] / dst=[_]" ) {
    w.add_cargo( src, Commodity{ .type = food, .quantity = 40 },
                 /*slot=*/0 );
    dst.slots.push_back( CargoSlot::empty{} );
    expected = {
      .items    = { .commodities =
                        {
                       { .type = food, .quantity = 40 },
                     } },
      .max_take = 1 };
    REQUIRE( f() == expected );
  }

  SECTION( "src=[c] / dst=[c]" ) {
    w.add_cargo( src, Commodity{ .type = food, .quantity = 40 },
                 /*slot=*/0 );
    w.add_cargo( dst, Commodity{ .type = sugar, .quantity = 1 },
                 /*slot=*/0 );
    expected = {
      .items    = { .commodities =
                        {
                       { .type = food, .quantity = 40 },
                     } },
      .max_take = 0 };
    REQUIRE( f() == expected );
  }

  SECTION( "src=[c,c] / dst=[_]" ) {
    w.add_cargo( src,
                 Commodity{ .type = silver, .quantity = 40 },
                 /*slot=*/0 );
    w.add_cargo( src,
                 Commodity{ .type = silver, .quantity = 40 },
                 /*slot=*/1 );
    dst.slots.push_back( CargoSlot::empty{} );
    expected = {
      .items    = { .commodities =
                        {
                       { .type = silver, .quantity = 80 },
                     } },
      .max_take = 1 };
    REQUIRE( f() == expected );
  }

  SECTION( "src=[c1,c2] / dst=[_]" ) {
    w.add_cargo( src,
                 Commodity{ .type = silver, .quantity = 40 },
                 /*slot=*/0 );
    w.add_cargo( src, Commodity{ .type = ore, .quantity = 30 },
                 /*slot=*/1 );
    dst.slots.push_back( CargoSlot::empty{} );
    expected = {
      .items    = { .commodities =
                        {
                       { .type = ore, .quantity = 30 },
                       { .type = silver, .quantity = 40 },
                     } },
      .max_take = 1 };
    REQUIRE( f() == expected );
  }

  SECTION( "src=[c1,c2] / dst=[_,_,_]" ) {
    w.add_cargo( src,
                 Commodity{ .type = silver, .quantity = 40 },
                 /*slot=*/0 );
    w.add_cargo( src, Commodity{ .type = ore, .quantity = 30 },
                 /*slot=*/1 );
    dst.slots.push_back( CargoSlot::empty{} );
    dst.slots.push_back( CargoSlot::empty{} );
    dst.slots.push_back( CargoSlot::empty{} );
    expected = {
      .items    = { .commodities =
                        {
                       { .type = ore, .quantity = 30 },
                       { .type = silver, .quantity = 40 },
                     } },
      .max_take = 3 };
    REQUIRE( f() == expected );
  }

  SECTION( "src=[c1,c2] / dst=[c,_,c]" ) {
    w.add_cargo( src,
                 Commodity{ .type = silver, .quantity = 40 },
                 /*slot=*/0 );
    w.add_cargo( src, Commodity{ .type = ore, .quantity = 30 },
                 /*slot=*/1 );
    w.add_cargo( dst, Commodity{ .type = ore, .quantity = 40 },
                 /*slot=*/0 );
    w.add_cargo( dst, Commodity{ .type = ore, .quantity = 30 },
                 /*slot=*/2 );
    expected = {
      .items    = { .commodities =
                        {
                       { .type = ore, .quantity = 30 },
                       { .type = silver, .quantity = 40 },
                     } },
      .max_take = 2 };
    REQUIRE( f() == expected );
  }

  SECTION( "src=[_] / dst=[_,_,_]" ) {
    src.slots.push_back( CargoSlot::empty{} );
    dst.slots.push_back( CargoSlot::empty{} );
    dst.slots.push_back( CargoSlot::empty{} );
    dst.slots.push_back( CargoSlot::empty{} );
    expected = { .items = { .commodities = {} }, .max_take = 3 };
    REQUIRE( f() == expected );
  }

  SECTION( "src=[c1,c2] / dst=[u,_,u]" ) {
    w.add_cargo( src,
                 Commodity{ .type = silver, .quantity = 40 },
                 /*slot=*/0 );
    w.add_cargo( src, Commodity{ .type = ore, .quantity = 30 },
                 /*slot=*/1 );
    UnitId const free_colonist_id =
        w.add_free_unit( e_unit_type::free_colonist ).id();
    UnitId const expert_ore_miner_id =
        w.add_free_unit( e_unit_type::expert_ore_miner ).id();
    w.add_cargo( dst, free_colonist_id,
                 /*slot=*/0 );
    w.add_cargo( dst, expert_ore_miner_id,
                 /*slot=*/2 );
    expected = {
      .items    = { .commodities =
                        {
                       { .type = ore, .quantity = 30 },
                       { .type = silver, .quantity = 40 },
                     } },
      .max_take = 1 };
    REQUIRE( f() == expected );
  }

  SECTION( "src=[c1,_,u] / dst=[c,_,c]" ) {
    UnitId const free_colonist_id =
        w.add_free_unit( e_unit_type::free_colonist ).id();
    w.add_cargo( src,
                 Commodity{ .type = silver, .quantity = 40 },
                 /*slot=*/0 );
    w.add_cargo( dst, free_colonist_id,
                 /*slot=*/2 );
    w.add_cargo( dst, Commodity{ .type = ore, .quantity = 40 },
                 /*slot=*/0 );
    w.add_cargo( dst, Commodity{ .type = ore, .quantity = 30 },
                 /*slot=*/2 );
    expected = {
      .items    = { .commodities =
                        {
                       { .type = silver, .quantity = 40 },
                     } },
      .max_take = 2 };
    REQUIRE( f() == expected );
  }
}

TEST_CASE( "[capture-cargo] transfer_capturable_cargo_items" ) {
  world w;
}

TEST_CASE( "[capture-cargo] select_items_to_capture_ui" ) {
  world w;
}

} // namespace
} // namespace rn
