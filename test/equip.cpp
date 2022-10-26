/****************************************************************
**equip.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-09-28.
*
* Description: Unit tests for the src/equip.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/equip.hpp"

// Testing.
#include "test/fake/world.hpp"
#include "test/mocking.hpp"
#include "test/mocks/igui.hpp"

// Revolution Now
#include "src/market.hpp"

// ss
#include "ss/player.hpp"
#include "ss/ref.hpp"
#include "ss/unit-composer.hpp"
#include "ss/units.hpp"

// refl
#include "refl/to-str.hpp"

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
    init_prices_to_average();
    create_1x1_map();
  }

  vector<HarborEquipOption> call_equip_options(
      UnitComposition comp ) const {
    return harbor_equip_options( ss(), default_player(), comp );
  }

  vector<HarborEquipOption> call_equip_options(
      e_unit_type type ) const {
    return call_equip_options( UnitComposition::create( type ) );
  }

  void create_1x1_map() {
    MapSquare const   L = make_grassland();
    vector<MapSquare> tiles{ L };
    build_map( std::move( tiles ), 1 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[equip] harbor_equip_options" ) {
  World                     W;
  Player&                   player = W.default_player();
  vector<HarborEquipOption> expected;
  using UC = UnitComposition;
  using UT = UnitType;

  auto f = [&]( auto&& what ) {
    return W.call_equip_options( what );
  };

  W.set_tax_rate( 50 );

  SECTION( "money=0" ) {
    // free_colonist
    expected = {
        { .modifier        = e_unit_type_modifier::blessing,
          .modifier_delta  = e_unit_type_modifier_delta::add,
          .money_delta     = 0,
          .can_afford      = true,
          .commodity_delta = nothing,
          .new_comp = UC::create( e_unit_type::missionary ) },
        { .modifier       = e_unit_type_modifier::tools,
          .modifier_delta = e_unit_type_modifier_delta::add,
          .money_delta    = -500,
          .can_afford     = false,
          .commodity_delta =
              Commodity{ .type     = e_commodity::tools,
                         .quantity = 100 },
          .new_comp = UC::create( e_unit_type::pioneer ) },
        { .modifier       = e_unit_type_modifier::horses,
          .modifier_delta = e_unit_type_modifier_delta::add,
          .money_delta    = -300,
          .can_afford     = false,
          .commodity_delta =
              Commodity{ .type     = e_commodity::horses,
                         .quantity = 50 },
          .new_comp = UC::create( e_unit_type::scout ) },
        { .modifier       = e_unit_type_modifier::muskets,
          .modifier_delta = e_unit_type_modifier_delta::add,
          .money_delta    = -550,
          .can_afford     = false,
          .commodity_delta =
              Commodity{ .type     = e_commodity::muskets,
                         .quantity = 50 },
          .new_comp = UC::create( e_unit_type::soldier ) } };
    REQUIRE( f( e_unit_type::free_colonist ) == expected );

    // veteran_soldier
    expected = {
        { .modifier       = e_unit_type_modifier::muskets,
          .modifier_delta = e_unit_type_modifier_delta::del,
          .money_delta    = 250,
          .can_afford     = true,
          .commodity_delta =
              Commodity{ .type     = e_commodity::muskets,
                         .quantity = 50 },
          .new_comp =
              UC::create( e_unit_type::veteran_colonist ) },
        { .modifier       = e_unit_type_modifier::horses,
          .modifier_delta = e_unit_type_modifier_delta::add,
          .money_delta    = -300,
          .can_afford     = false,
          .commodity_delta =
              Commodity{ .type     = e_commodity::horses,
                         .quantity = 50 },
          .new_comp =
              UC::create( e_unit_type::veteran_dragoon ) } };
    REQUIRE( f( e_unit_type::veteran_soldier ) == expected );

    // veteran_dragoon
    expected = {
        { .modifier       = e_unit_type_modifier::muskets,
          .modifier_delta = e_unit_type_modifier_delta::del,
          .money_delta    = 250,
          .can_afford     = true,
          .commodity_delta =
              Commodity{ .type     = e_commodity::muskets,
                         .quantity = 50 },
          .new_comp = UC::create(
              UT::create( e_unit_type::scout,
                          e_unit_type::veteran_colonist )
                  .value() ) },
        { .modifier       = e_unit_type_modifier::horses,
          .modifier_delta = e_unit_type_modifier_delta::del,
          .money_delta    = 125,
          .can_afford     = true,
          .commodity_delta =
              Commodity{ .type     = e_commodity::horses,
                         .quantity = 50 },
          .new_comp =
              UC::create( e_unit_type::veteran_soldier ) } };
    REQUIRE( f( e_unit_type::veteran_dragoon ) == expected );

    // scout
    expected = {
        { .modifier       = e_unit_type_modifier::horses,
          .modifier_delta = e_unit_type_modifier_delta::del,
          .money_delta    = 125,
          .can_afford     = true,
          .commodity_delta =
              Commodity{ .type     = e_commodity::horses,
                         .quantity = 50 },
          .new_comp = UC::create( e_unit_type::free_colonist ) },
        { .modifier       = e_unit_type_modifier::muskets,
          .modifier_delta = e_unit_type_modifier_delta::add,
          .money_delta    = -550,
          .can_afford     = false,
          .commodity_delta =
              Commodity{ .type     = e_commodity::muskets,
                         .quantity = 50 },
          .new_comp = UC::create( e_unit_type::dragoon ) } };
    REQUIRE( f( e_unit_type::scout ) == expected );

    // pioneer/indentured_servant
    expected = {
        { .modifier       = e_unit_type_modifier::tools,
          .modifier_delta = e_unit_type_modifier_delta::del,
          .money_delta    = 160,
          .can_afford     = true,
          .commodity_delta =
              Commodity{ .type     = e_commodity::tools,
                         .quantity = 80 },
          .new_comp =
              UC::create( e_unit_type::indentured_servant ) } };
    UnitComposition const pioneer =
        UnitComposition::create(
            UnitType::create( e_unit_type::pioneer,
                              e_unit_type::indentured_servant )
                .value(),
            UnitComposition::UnitInventoryMap{
                { e_unit_inventory::tools, 80 } } )
            .value();
    REQUIRE( f( pioneer ) == expected );

    // missionary
    expected = {
        { .modifier        = e_unit_type_modifier::blessing,
          .modifier_delta  = e_unit_type_modifier_delta::del,
          .money_delta     = 0,
          .can_afford      = true,
          .commodity_delta = nothing,
          .new_comp =
              UC::create( e_unit_type::free_colonist ) } };
    REQUIRE( f( e_unit_type::missionary ) == expected );
  }
  SECTION( "money=1000" ) {
    player.money = 1000;

    // free_colonist
    expected = {
        { .modifier        = e_unit_type_modifier::blessing,
          .modifier_delta  = e_unit_type_modifier_delta::add,
          .money_delta     = 0,
          .can_afford      = true,
          .commodity_delta = nothing,
          .new_comp = UC::create( e_unit_type::missionary ) },
        { .modifier       = e_unit_type_modifier::tools,
          .modifier_delta = e_unit_type_modifier_delta::add,
          .money_delta    = -500,
          .can_afford     = true,
          .commodity_delta =
              Commodity{ .type     = e_commodity::tools,
                         .quantity = 100 },
          .new_comp = UC::create( e_unit_type::pioneer ) },
        { .modifier       = e_unit_type_modifier::horses,
          .modifier_delta = e_unit_type_modifier_delta::add,
          .money_delta    = -300,
          .can_afford     = true,
          .commodity_delta =
              Commodity{ .type     = e_commodity::horses,
                         .quantity = 50 },
          .new_comp = UC::create( e_unit_type::scout ) },
        { .modifier       = e_unit_type_modifier::muskets,
          .modifier_delta = e_unit_type_modifier_delta::add,
          .money_delta    = -550,
          .can_afford     = true,
          .commodity_delta =
              Commodity{ .type     = e_commodity::muskets,
                         .quantity = 50 },
          .new_comp = UC::create( e_unit_type::soldier ) } };
    REQUIRE( f( e_unit_type::free_colonist ) == expected );

    // veteran_soldier
    expected = {
        { .modifier       = e_unit_type_modifier::muskets,
          .modifier_delta = e_unit_type_modifier_delta::del,
          .money_delta    = 250,
          .can_afford     = true,
          .commodity_delta =
              Commodity{ .type     = e_commodity::muskets,
                         .quantity = 50 },
          .new_comp =
              UC::create( e_unit_type::veteran_colonist ) },
        { .modifier       = e_unit_type_modifier::horses,
          .modifier_delta = e_unit_type_modifier_delta::add,
          .money_delta    = -300,
          .can_afford     = true,
          .commodity_delta =
              Commodity{ .type     = e_commodity::horses,
                         .quantity = 50 },
          .new_comp =
              UC::create( e_unit_type::veteran_dragoon ) } };
    REQUIRE( f( e_unit_type::veteran_soldier ) == expected );

    // veteran_dragoon
    expected = {
        { .modifier       = e_unit_type_modifier::muskets,
          .modifier_delta = e_unit_type_modifier_delta::del,
          .money_delta    = 250,
          .can_afford     = true,
          .commodity_delta =
              Commodity{ .type     = e_commodity::muskets,
                         .quantity = 50 },
          .new_comp = UC::create(
              UT::create( e_unit_type::scout,
                          e_unit_type::veteran_colonist )
                  .value() ) },
        { .modifier       = e_unit_type_modifier::horses,
          .modifier_delta = e_unit_type_modifier_delta::del,
          .money_delta    = 125,
          .can_afford     = true,
          .commodity_delta =
              Commodity{ .type     = e_commodity::horses,
                         .quantity = 50 },
          .new_comp =
              UC::create( e_unit_type::veteran_soldier ) } };
    REQUIRE( f( e_unit_type::veteran_dragoon ) == expected );

    // scout
    expected = {
        { .modifier       = e_unit_type_modifier::horses,
          .modifier_delta = e_unit_type_modifier_delta::del,
          .money_delta    = 125,
          .can_afford     = true,
          .commodity_delta =
              Commodity{ .type     = e_commodity::horses,
                         .quantity = 50 },
          .new_comp = UC::create( e_unit_type::free_colonist ) },
        { .modifier       = e_unit_type_modifier::muskets,
          .modifier_delta = e_unit_type_modifier_delta::add,
          .money_delta    = -550,
          .can_afford     = true,
          .commodity_delta =
              Commodity{ .type     = e_commodity::muskets,
                         .quantity = 50 },
          .new_comp = UC::create( e_unit_type::dragoon ) } };
    REQUIRE( f( e_unit_type::scout ) == expected );

    // pioneer/indentured_servant
    expected = {
        { .modifier       = e_unit_type_modifier::tools,
          .modifier_delta = e_unit_type_modifier_delta::del,
          .money_delta    = 160,
          .can_afford     = true,
          .commodity_delta =
              Commodity{ .type     = e_commodity::tools,
                         .quantity = 80 },
          .new_comp =
              UC::create( e_unit_type::indentured_servant ) } };
    UnitComposition const pioneer =
        UnitComposition::create(
            UnitType::create( e_unit_type::pioneer,
                              e_unit_type::indentured_servant )
                .value(),
            UnitComposition::UnitInventoryMap{
                { e_unit_inventory::tools, 80 } } )
            .value();
    REQUIRE( f( pioneer ) == expected );

    // missionary
    expected = {
        { .modifier        = e_unit_type_modifier::blessing,
          .modifier_delta  = e_unit_type_modifier_delta::del,
          .money_delta     = 0,
          .can_afford      = true,
          .commodity_delta = nothing,
          .new_comp =
              UC::create( e_unit_type::free_colonist ) } };
    REQUIRE( f( e_unit_type::missionary ) == expected );

    // artillery
    expected = {};
    REQUIRE( f( e_unit_type::artillery ) == expected );

    // damaged_artillery
    expected = {};
    REQUIRE( f( e_unit_type::damaged_artillery ) == expected );
  }
  SECTION( "money=499" ) {
    // This one tests that we don't equip the pioneer with a par-
    // tial number of tools (e.g. 80) if that's all that the
    // player can afford. That wouldn't be a problem, but it
    // would be complicated to implement that, so the game does
    // not do it. You either equip with 100 or none.
    player.money = 499;

    // free_colonist
    expected = {
        { .modifier        = e_unit_type_modifier::blessing,
          .modifier_delta  = e_unit_type_modifier_delta::add,
          .money_delta     = 0,
          .can_afford      = true,
          .commodity_delta = nothing,
          .new_comp = UC::create( e_unit_type::missionary ) },
        { .modifier       = e_unit_type_modifier::tools,
          .modifier_delta = e_unit_type_modifier_delta::add,
          .money_delta    = -500,
          .can_afford     = false,
          .commodity_delta =
              Commodity{ .type     = e_commodity::tools,
                         .quantity = 100 },
          .new_comp = UC::create( e_unit_type::pioneer ) },
        { .modifier       = e_unit_type_modifier::horses,
          .modifier_delta = e_unit_type_modifier_delta::add,
          .money_delta    = -300,
          .can_afford     = true,
          .commodity_delta =
              Commodity{ .type     = e_commodity::horses,
                         .quantity = 50 },
          .new_comp = UC::create( e_unit_type::scout ) },
        { .modifier       = e_unit_type_modifier::muskets,
          .modifier_delta = e_unit_type_modifier_delta::add,
          .money_delta    = -550,
          .can_afford     = false,
          .commodity_delta =
              Commodity{ .type     = e_commodity::muskets,
                         .quantity = 50 },
          .new_comp = UC::create( e_unit_type::soldier ) } };
    REQUIRE( f( e_unit_type::free_colonist ) == expected );
  }
  SECTION( "money=1000, muskets/horses boycotted" ) {
    player.money = 1000;

    player.old_world.market.commodities[e_commodity::horses]
        .boycott = true;
    player.old_world.market.commodities[e_commodity::muskets]
        .boycott = true;

    // free_colonist
    expected = {
        { .modifier        = e_unit_type_modifier::blessing,
          .modifier_delta  = e_unit_type_modifier_delta::add,
          .money_delta     = 0,
          .can_afford      = true,
          .commodity_delta = nothing,
          .new_comp = UC::create( e_unit_type::missionary ) },
        { .modifier       = e_unit_type_modifier::tools,
          .modifier_delta = e_unit_type_modifier_delta::add,
          .money_delta    = -500,
          .can_afford     = true,
          .commodity_delta =
              Commodity{ .type     = e_commodity::tools,
                         .quantity = 100 },
          .new_comp = UC::create( e_unit_type::pioneer ) } };
    REQUIRE( f( e_unit_type::free_colonist ) == expected );
  }
}

TEST_CASE( "[equip] harbor_equip_description" ) {
  HarborEquipOption option;

  auto f = [&] { return harbor_equip_description( option ); };

  option = {
      .modifier        = e_unit_type_modifier::horses,
      .modifier_delta  = e_unit_type_modifier_delta::add,
      .money_delta     = -300,
      .can_afford      = true,
      .commodity_delta = Commodity{ .type = e_commodity::horses,
                                    .quantity = 50 },
      .new_comp        = {},
  };
  REQUIRE( f() ==
           "Equip with @[H]Horses@[] (costs @[H]300@[])." );

  option = {
      .modifier        = e_unit_type_modifier::horses,
      .modifier_delta  = e_unit_type_modifier_delta::del,
      .money_delta     = 300,
      .can_afford      = true,
      .commodity_delta = Commodity{ .type = e_commodity::horses,
                                    .quantity = 50 },
      .new_comp        = {},
  };
  REQUIRE( f() == "Sell @[H]Horses@[] (save @[H]300@[])." );

  option = {
      .modifier        = e_unit_type_modifier::muskets,
      .modifier_delta  = e_unit_type_modifier_delta::add,
      .money_delta     = -300,
      .can_afford      = true,
      .commodity_delta = Commodity{ .type = e_commodity::muskets,
                                    .quantity = 50 },
      .new_comp        = {},
  };
  REQUIRE( f() ==
           "Arm with @[H]Muskets@[] (costs @[H]300@[])." );

  option = {
      .modifier        = e_unit_type_modifier::muskets,
      .modifier_delta  = e_unit_type_modifier_delta::del,
      .money_delta     = 300,
      .can_afford      = true,
      .commodity_delta = Commodity{ .type = e_commodity::muskets,
                                    .quantity = 50 },
      .new_comp        = {},
  };
  REQUIRE( f() == "Sell @[H]Muskets@[] (save @[H]300@[])." );

  option = {
      .modifier        = e_unit_type_modifier::tools,
      .modifier_delta  = e_unit_type_modifier_delta::add,
      .money_delta     = -300,
      .can_afford      = true,
      .commodity_delta = Commodity{ .type = e_commodity::tools,
                                    .quantity = 100 },
      .new_comp        = {},
  };
  REQUIRE( f() ==
           "Equip with @[H]Tools@[] (costs @[H]300@[])." );

  option = {
      .modifier        = e_unit_type_modifier::tools,
      .modifier_delta  = e_unit_type_modifier_delta::del,
      .money_delta     = 300,
      .can_afford      = true,
      .commodity_delta = Commodity{ .type = e_commodity::tools,
                                    .quantity = 80 },
      .new_comp        = {},
  };
  REQUIRE( f() == "Sell @[H]80 Tools@[] (save @[H]300@[])." );

  option = {
      .modifier        = e_unit_type_modifier::blessing,
      .modifier_delta  = e_unit_type_modifier_delta::add,
      .money_delta     = 0,
      .can_afford      = true,
      .commodity_delta = {},
      .new_comp        = {},
  };
  REQUIRE( f() == "Bless as @[H]Missionary@[]." );

  option = {
      .modifier        = e_unit_type_modifier::blessing,
      .modifier_delta  = e_unit_type_modifier_delta::del,
      .money_delta     = 0,
      .can_afford      = true,
      .commodity_delta = {},
      .new_comp        = {},
  };
  REQUIRE( f() == "Cancel @[H]Missionary@[] status." );
}

TEST_CASE( "[equip] perform_harbor_equip_option" ) {
  World             W;
  Player&           player = W.default_player();
  HarborEquipOption option;
  UnitId const      unit_id =
      W.add_unit_in_port( e_unit_type::free_colonist );
  PriceChange expected;

  using UC = UnitComposition;

  auto f = [&] {
    return perform_harbor_equip_option( W.ss(), player, unit_id,
                                        option );
  };

  player.money = 10000;

  option   = { .modifier        = e_unit_type_modifier::blessing,
               .modifier_delta  = e_unit_type_modifier_delta::add,
               .money_delta     = 0,
               .can_afford      = true,
               .commodity_delta = nothing,
               .new_comp = UC::create( e_unit_type::missionary ) };
  expected = {};
  REQUIRE( f() == expected );
  REQUIRE( W.ss().units.unit_for( unit_id ).type() ==
           e_unit_type::missionary );
  REQUIRE( player.money == 10000 );

  option = {
      .modifier        = e_unit_type_modifier::tools,
      .modifier_delta  = e_unit_type_modifier_delta::add,
      .money_delta     = -500,
      .can_afford      = true,
      .commodity_delta = Commodity{ .type = e_commodity::tools,
                                    .quantity = 100 },
      .new_comp        = UC::create( e_unit_type::pioneer ) };
  expected =
      create_price_change( player, e_commodity::tools, 0 );
  REQUIRE( f() == expected );
  REQUIRE( W.ss().units.unit_for( unit_id ).type() ==
           e_unit_type::pioneer );
  REQUIRE( player.money == 9500 );

  option = {
      .modifier        = e_unit_type_modifier::horses,
      .modifier_delta  = e_unit_type_modifier_delta::add,
      .money_delta     = -300,
      .can_afford      = true,
      .commodity_delta = Commodity{ .type = e_commodity::horses,
                                    .quantity = 50 },
      .new_comp        = UC::create( e_unit_type::scout ) };
  expected =
      create_price_change( player, e_commodity::horses, 0 );
  REQUIRE( f() == expected );
  REQUIRE( W.ss().units.unit_for( unit_id ).type() ==
           e_unit_type::scout );
  REQUIRE( player.money == 9200 );

  option = {
      .modifier        = e_unit_type_modifier::muskets,
      .modifier_delta  = e_unit_type_modifier_delta::add,
      .money_delta     = -550,
      .can_afford      = true,
      .commodity_delta = Commodity{ .type = e_commodity::muskets,
                                    .quantity = 50 },
      .new_comp        = UC::create( e_unit_type::soldier ) };
  expected =
      create_price_change( player, e_commodity::muskets, 0 );
  REQUIRE( f() == expected );
  REQUIRE( W.ss().units.unit_for( unit_id ).type() ==
           e_unit_type::soldier );
  REQUIRE( player.money == 8650 );

  // Try to force a price change.
  option = {
      .modifier        = e_unit_type_modifier::muskets,
      .modifier_delta  = e_unit_type_modifier_delta::add,
      .money_delta     = -5500,
      .can_afford      = true,
      .commodity_delta = Commodity{ .type = e_commodity::muskets,
                                    .quantity = 500 },
      .new_comp        = UC::create( e_unit_type::soldier ) };
  expected =
      create_price_change( player, e_commodity::muskets, 1 );
  REQUIRE( f() == expected );
  REQUIRE( W.ss().units.unit_for( unit_id ).type() ==
           e_unit_type::soldier );
  REQUIRE( player.money == 3150 );
}

TEST_CASE( "[equip] colony_equip_description" ) {
  e_unit_type input;
  string      expected;

  auto f = [&] {
    ColonyEquipOption const option = {
        .new_comp = UnitComposition::create( input ) };
    return colony_equip_description( option );
  };

  input    = e_unit_type::dragoon;
  expected = "Dragoon";
  REQUIRE( f() == expected );

  input    = e_unit_type::hardy_pioneer;
  expected = "Hardy Pioneer";
  REQUIRE( f() == expected );

  input    = e_unit_type::veteran_soldier;
  expected = "Veteran Soldier";
  REQUIRE( f() == expected );
}

TEST_CASE( "[equip] perform_colony_equip_option" ) {
  World   W;
  Unit&   unit   = W.add_free_unit( e_unit_type::free_colonist );
  Colony& colony = W.add_colony_with_new_unit( Coord{} );
  colony.commodities[e_commodity::sugar]   = 50;
  colony.commodities[e_commodity::muskets] = 50;
  colony.commodities[e_commodity::horses]  = 100;

  ColonyEquipOption const option{
      .commodity_deltas = { { e_commodity::muskets, -50 },
                            { e_commodity::horses, -50 } },
      .new_comp =
          UnitComposition::create( e_unit_type::dragoon ) };

  refl::enum_map<e_commodity, int> expected;

  // Sanity check.
  expected = { { e_commodity::sugar, 50 },
               { e_commodity::horses, 100 },
               { e_commodity::muskets, 50 } };
  REQUIRE( colony.commodities == expected );
  REQUIRE( unit.type() == e_unit_type::free_colonist );

  perform_colony_equip_option( colony, W.default_player(), unit,
                               option );

  expected = { { e_commodity::sugar, 50 },
               { e_commodity::horses, 50 },
               { e_commodity::muskets, 0 } };
  REQUIRE( colony.commodities == expected );
  REQUIRE( unit.type_obj() ==
           UnitType::create( e_unit_type::dragoon,
                             e_unit_type::free_colonist )
               .value() );
}

TEST_CASE( "[equip] colony_equip_options" ) {
  // We're not going to do an exhaustive test of all possible
  // unit transformations here because that is done elsewhere.
  World   W;
  Colony& colony = W.add_colony_with_new_unit( Coord{} );
  colony.commodities[e_commodity::sugar]   = 50;
  colony.commodities[e_commodity::muskets] = 50;
  colony.commodities[e_commodity::horses]  = 100;

  SECTION( "seasoned colonist" ) {
    Unit const& unit =
        W.add_free_unit( e_unit_type::seasoned_colonist );

    auto f = [&] {
      return colony_equip_options( colony, unit.composition() );
    };

    vector<ColonyEquipOption> expected{
        ColonyEquipOption{
            .commodity_deltas = { { e_commodity::muskets, -50 },
                                  { e_commodity::horses, -50 } },
            .new_comp         = UnitComposition::create(
                UnitType::create(
                    e_unit_type::dragoon,
                    e_unit_type::seasoned_colonist )
                    .value() ) },
        ColonyEquipOption{
            .commodity_deltas = {},
            .new_comp         = UnitComposition::create(
                e_unit_type::seasoned_colonist ) },
        ColonyEquipOption{
            .commodity_deltas = {},
            .new_comp         = UnitComposition::create(
                UnitType::create(
                    e_unit_type::missionary,
                    e_unit_type::seasoned_colonist )
                    .value() ) },
        ColonyEquipOption{
            .commodity_deltas = { { e_commodity::horses, -50 } },
            .new_comp         = UnitComposition::create(
                e_unit_type::seasoned_scout ) },
        ColonyEquipOption{
            .commodity_deltas = { { e_commodity::muskets,
                                    -50 } },
            .new_comp         = UnitComposition::create(
                UnitType::create(
                    e_unit_type::soldier,
                    e_unit_type::seasoned_colonist )
                    .value() ) },
    };
    REQUIRE( f() == expected );
  }

  // Make sure that we don't transform to any types that have the
  // "independence" modifier.
  SECTION( "veteran colonist" ) {
    Unit const& unit =
        W.add_free_unit( e_unit_type::veteran_colonist );

    auto f = [&] {
      return colony_equip_options( colony, unit.composition() );
    };

    vector<ColonyEquipOption> expected{
        ColonyEquipOption{ .commodity_deltas = {},
                           .new_comp = UnitComposition::create(
                               e_unit_type::veteran_colonist ) },
        ColonyEquipOption{
            .commodity_deltas = {},
            .new_comp         = UnitComposition::create(
                UnitType::create( e_unit_type::missionary,
                                          e_unit_type::veteran_colonist )
                    .value() ) },
        ColonyEquipOption{
            .commodity_deltas = { { e_commodity::horses, -50 } },
            .new_comp         = UnitComposition::create(
                UnitType::create( e_unit_type::scout,
                                          e_unit_type::veteran_colonist )
                    .value() ) },
        ColonyEquipOption{
            .commodity_deltas = { { e_commodity::muskets, -50 },
                                  { e_commodity::horses, -50 } },
            .new_comp         = UnitComposition::create(
                e_unit_type::veteran_dragoon ) },
        ColonyEquipOption{
            .commodity_deltas = { { e_commodity::muskets,
                                    -50 } },
            .new_comp         = UnitComposition::create(
                e_unit_type::veteran_soldier ) },
    };
    REQUIRE( f() == expected );
  }
}

} // namespace
} // namespace rn
