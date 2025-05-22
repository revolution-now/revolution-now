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
#include "test/mocks/igui.hpp"
#include "test/util/coro.hpp"

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

using ::gfx::point;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct world : testing::World {
  world() {
    add_player( e_player::english );
    add_player( e_player::french );
    set_default_player_type( e_player::english );
    create_default_map();
  }

  void create_default_map() {
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
    vector<MapSquare> tiles{
      _, L, L, //
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

  CapturableCargoItems items;
  CargoHold src;
  CargoHold dst;
  wrapped::CargoHold expected_src;
  wrapped::CargoHold expected_dst;

  auto f = [&] {
    return transfer_capturable_cargo_items( w.ss(), items, src,
                                            dst );
  };

  using enum e_commodity;

  SECTION( "default" ) {
    f();
    expected_src = {};
    expected_dst = {};
    REQUIRE( src.refl() == expected_src );
    REQUIRE( dst.refl() == expected_dst );
  }

  SECTION( "src=[c] / dst=[_], move none" ) {
    wrapped::CargoHold src_refl;
    wrapped::CargoHold dst_refl;
    w.add_cargo( src_refl,
                 Commodity{ .type = silver, .quantity = 40 },
                 /*slot=*/0 );
    dst_refl.slots.push_back( CargoSlot::empty{} );
    src   = CargoHold( std::move( src_refl ) );
    dst   = CargoHold( std::move( dst_refl ) );
    items = {};
    f();
    expected_src = {
      .slots = {
        CargoSlot::cargo{
          .contents =
              Cargo::commodity{
                .obj = { .type = silver, .quantity = 40 } } },
      } };
    expected_dst = { .slots = {
                       CargoSlot::empty{},
                     } };
    REQUIRE( src.refl() == expected_src );
    REQUIRE( dst.refl() == expected_dst );
  }

  SECTION( "src=[c] / dst=[_], move one" ) {
    wrapped::CargoHold src_refl;
    wrapped::CargoHold dst_refl;
    w.add_cargo( src_refl,
                 Commodity{ .type = silver, .quantity = 40 },
                 /*slot=*/0 );
    dst_refl.slots.push_back( CargoSlot::empty{} );
    src   = CargoHold( std::move( src_refl ) );
    dst   = CargoHold( std::move( dst_refl ) );
    items = { .commodities = {
                { .type = silver, .quantity = 40 },
              } };
    f();
    expected_src = { .slots = {
                       CargoSlot::empty{},
                     } };
    expected_dst = {
      .slots = {
        CargoSlot::cargo{
          .contents =
              Cargo::commodity{
                .obj = { .type = silver, .quantity = 40 } } },
      } };
    REQUIRE( src.refl() == expected_src );
    REQUIRE( dst.refl() == expected_dst );
  }

  SECTION( "src=[c,c] / dst=[_,_], move partial" ) {
    wrapped::CargoHold src_refl;
    wrapped::CargoHold dst_refl;
    w.add_cargo( src_refl,
                 Commodity{ .type = silver, .quantity = 40 },
                 /*slot=*/0 );
    w.add_cargo( src_refl,
                 Commodity{ .type = silver, .quantity = 40 },
                 /*slot=*/1 );
    dst_refl.slots.push_back( CargoSlot::empty{} );
    dst_refl.slots.push_back( CargoSlot::empty{} );
    src   = CargoHold( std::move( src_refl ) );
    dst   = CargoHold( std::move( dst_refl ) );
    items = { .commodities = {
                { .type = silver, .quantity = 40 },
              } };
    f();
    expected_src = {
      .slots = {
        CargoSlot::cargo{
          .contents =
              Cargo::commodity{
                .obj = { .type = silver, .quantity = 40 } } },
        CargoSlot::empty{},
      } };
    expected_dst = {
      .slots = {
        CargoSlot::cargo{
          .contents =
              Cargo::commodity{
                .obj = { .type = silver, .quantity = 40 } } },
        CargoSlot::empty{},
      } };
    REQUIRE( src.refl() == expected_src );
    REQUIRE( dst.refl() == expected_dst );
  }
}

TEST_CASE( "[capture-cargo] select_items_to_capture_ui" ) {
  world w;

  CapturableCargo capturable;
  CapturableCargoItems expected;

  Unit& frigate = w.add_unit_on_map( e_unit_type::frigate,
                                     point{ .x = 0, .y = 0 },
                                     e_player::english );
  Unit& galleon = w.add_unit_on_map( e_unit_type::galleon,
                                     point{ .x = 1, .y = 1 },
                                     e_player::french );

  auto f = [&] {
    return co_await_test( select_items_to_capture_ui(
        w.ss(), w.gui(), galleon.id(), frigate.id(),
        capturable ) );
  };

  using enum e_commodity;

  SECTION( "default" ) { REQUIRE( f() == expected ); }

  SECTION( "nothing to take" ) {
    capturable = { .items    = { .commodities = {} },
                   .max_take = 4 };
    expected   = {};
    REQUIRE( f() == expected );
  }

  SECTION( "can take all" ) {
    capturable = {
      .items    = { .commodities =
                        {
                       { .type = furs, .quantity = 30 },
                       { .type = coats, .quantity = 40 },
                       { .type = trade_goods, .quantity = 50 },
                       { .type = tools, .quantity = 100 },
                     } },
      .max_take = 4 };
    w.gui().EXPECT__message_box(
        "[English Frigate] has captured [30 furs] from [French] "
        "cargo!" );
    w.gui().EXPECT__message_box(
        "[English Frigate] has captured [40 coats] from "
        "[French] "
        "cargo!" );
    w.gui().EXPECT__message_box(
        "[English Frigate] has captured [50 trade goods] from "
        "[French] "
        "cargo!" );
    w.gui().EXPECT__message_box(
        "[English Frigate] has captured [100 tools] from "
        "[French] cargo!" );
    expected = { .commodities = {
                   { .type = furs, .quantity = 30 },
                   { .type = coats, .quantity = 40 },
                   { .type = trade_goods, .quantity = 50 },
                   { .type = tools, .quantity = 100 },
                 } };
    REQUIRE( f() == expected );
  }

  SECTION( "can take some, cancels" ) {
    capturable = {
      .items    = { .commodities =
                        {
                       { .type = furs, .quantity = 30 },
                       { .type = coats, .quantity = 40 },
                       { .type = trade_goods, .quantity = 50 },
                       { .type = tools, .quantity = 100 },
                     } },
      .max_take = 3 };
    ChoiceConfig const config1{
      .msg =
          "Select a cargo item to capture from [French "
          "Galleon]:",
      .options =
          {
            { .key = "0", .display_name = "30 furs" },
            { .key = "1", .display_name = "40 coats" },
            { .key = "2", .display_name = "50 trade goods" },
            { .key = "3", .display_name = "100 tools" },
          },
      .sort = false };
    w.gui().EXPECT__choice( config1 );
    expected = { .commodities = {} };
    REQUIRE( f() == expected );
  }

  SECTION( "can take some, chooses 2" ) {
    capturable = {
      .items    = { .commodities =
                        {
                       { .type = furs, .quantity = 30 },
                       { .type = coats, .quantity = 40 },
                       { .type = trade_goods, .quantity = 50 },
                       { .type = tools, .quantity = 100 },
                     } },
      .max_take = 3 };
    ChoiceConfig const config1{
      .msg =
          "Select a cargo item to capture from [French "
          "Galleon]:",
      .options =
          {
            { .key = "0", .display_name = "30 furs" },
            { .key = "1", .display_name = "40 coats" },
            { .key = "2", .display_name = "50 trade goods" },
            { .key = "3", .display_name = "100 tools" },
          },
      .sort = false };
    w.gui().EXPECT__choice( config1 ).returns<maybe<string>>(
        "2" );
    ChoiceConfig const config2{
      .msg =
          "Select a cargo item to capture from [French "
          "Galleon]:",
      .options =
          {
            { .key = "0", .display_name = "30 furs" },
            { .key = "1", .display_name = "40 coats" },
            { .key = "2", .display_name = "100 tools" },
          },
      .sort = false };
    w.gui().EXPECT__choice( config2 ).returns<maybe<string>>(
        "0" );
    ChoiceConfig const config3{
      .msg =
          "Select a cargo item to capture from [French "
          "Galleon]:",
      .options =
          {
            { .key = "0", .display_name = "40 coats" },
            { .key = "1", .display_name = "100 tools" },
          },
      .sort = false };
    w.gui().EXPECT__choice( config3 );
    expected = { .commodities = {
                   { .type = trade_goods, .quantity = 50 },
                   { .type = furs, .quantity = 30 },
                 } };
    REQUIRE( f() == expected );
  }
}

TEST_CASE( "[capture-cargo] notify_captured_cargo_human" ) {
  world w;

  Commodity stolen;

  Unit& frigate = w.add_unit_on_map( e_unit_type::frigate,
                                     point{ .x = 0, .y = 0 },
                                     e_player::english );

  auto f = [&] {
    return co_await_test( notify_captured_cargo_human(
        w.gui(), w.french(), w.english(), frigate, stolen ) );
  };

  using enum e_commodity;

  stolen = { .type = coats, .quantity = 40 };
  w.gui().EXPECT__message_box(
      "[English Frigate] has captured [40 coats] from [French] "
      "cargo!" );
  f();
}

} // namespace
} // namespace rn
