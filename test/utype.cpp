/****************************************************************
**utype.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-09-27.
*
* Description: Unit tests for the src/utype.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/utype.hpp"

// Revolution Now
#include "src/lua.hpp"         // FIXME: remove if not needed.
#include "src/luapp/state.hpp" // FIXME: remove if not needed.

// Must be last.
#include "test/catch-common.hpp"

FMT_TO_CATCH( ::rn::UnitPromotion_t );
FMT_TO_CATCH( ::rn::e_unit_type_modifier );

namespace rn {
namespace {

using namespace std;

using Catch::Contains;

TEST_CASE( "[utype] unit type attributes deserialization" ) {
  SECTION( "expert_cotton_planter" ) {
    UnitTypeAttributes const& desc =
        unit_attr( e_unit_type::expert_cotton_planter );
    REQUIRE( desc.name == "Expert Cotton Planter" );
    REQUIRE( desc.tile == e_tile::expert_cotton_planter );
    REQUIRE( desc.nat_icon_front == false );
    REQUIRE( desc.nat_icon_position == e_direction::sw );
    REQUIRE( desc.ship == false );
    REQUIRE( desc.visibility == 1 );
    REQUIRE( desc.movement_points == 1 );
    REQUIRE( desc.attack_points == 0 );
    REQUIRE( desc.defense_points == 1 );
    REQUIRE( desc.cargo_slots == 0 );
    REQUIRE( desc.cargo_slots_occupies == 1 );
    REQUIRE( desc.on_death.holds<UnitDeathAction::capture>() );
    REQUIRE( desc.canonical_base == nothing );
    REQUIRE( desc.expertise ==
             e_unit_activity::cotton_planting );
    REQUIRE( desc.promotion == nothing );
    unordered_map<e_unit_type,
                  unordered_set<e_unit_type_modifier>>
        expected_modifiers{
            { e_unit_type::missionary,
              { e_unit_type_modifier::blessing } },
            { e_unit_type::scout,
              { e_unit_type_modifier::horses } },
            { e_unit_type::pioneer,
              { e_unit_type_modifier::tools } },
            { e_unit_type::soldier,
              { e_unit_type_modifier::muskets } },
            { e_unit_type::dragoon,
              { e_unit_type_modifier::horses,
                e_unit_type_modifier::muskets } },
        };
    REQUIRE( desc.modifiers == expected_modifiers );
    // Derived fields.
    REQUIRE( desc.type == e_unit_type::expert_cotton_planter );
    REQUIRE( desc.is_derived == false );
    REQUIRE( desc.can_attack() == false );
    REQUIRE( desc.is_military_unit() == false );
  }
  SECTION( "petty_criminal" ) {
    UnitTypeAttributes const& desc =
        unit_attr( e_unit_type::petty_criminal );
    REQUIRE( desc.name == "Petty Criminal" );
    REQUIRE( desc.tile == e_tile::petty_criminal );
    REQUIRE( desc.nat_icon_front == false );
    REQUIRE( desc.nat_icon_position == e_direction::sw );
    REQUIRE( desc.ship == false );
    REQUIRE( desc.visibility == 1 );
    REQUIRE( desc.movement_points == 1 );
    REQUIRE( desc.attack_points == 0 );
    REQUIRE( desc.defense_points == 1 );
    REQUIRE( desc.cargo_slots == 0 );
    REQUIRE( desc.cargo_slots_occupies == 1 );
    REQUIRE( desc.on_death.holds<UnitDeathAction::capture>() );
    REQUIRE( desc.canonical_base == nothing );
    REQUIRE( desc.expertise == nothing );
    REQUIRE( desc.promotion ==
             UnitPromotion_t{ UnitPromotion::fixed{
                 .type = e_unit_type::indentured_servant } } );
    unordered_map<e_unit_type,
                  unordered_set<e_unit_type_modifier>>
        expected_modifiers{
            { e_unit_type::missionary,
              { e_unit_type_modifier::blessing } },
            { e_unit_type::scout,
              { e_unit_type_modifier::horses } },
            { e_unit_type::pioneer,
              { e_unit_type_modifier::tools } },
            { e_unit_type::soldier,
              { e_unit_type_modifier::muskets } },
            { e_unit_type::dragoon,
              { e_unit_type_modifier::horses,
                e_unit_type_modifier::muskets } },
        };
    REQUIRE( desc.modifiers == expected_modifiers );
    // Derived fields.
    REQUIRE( desc.type == e_unit_type::petty_criminal );
    REQUIRE( desc.is_derived == false );
    REQUIRE( desc.can_attack() == false );
    REQUIRE( desc.is_military_unit() == false );
  }
  SECTION( "free_colonist" ) {
    UnitTypeAttributes const& desc =
        unit_attr( e_unit_type::free_colonist );
    REQUIRE( desc.name == "Free Colonist" );
    REQUIRE( desc.tile == e_tile::free_colonist );
    REQUIRE( desc.nat_icon_front == false );
    REQUIRE( desc.nat_icon_position == e_direction::sw );
    REQUIRE( desc.ship == false );
    REQUIRE( desc.visibility == 1 );
    REQUIRE( desc.movement_points == 1 );
    REQUIRE( desc.attack_points == 0 );
    REQUIRE( desc.defense_points == 1 );
    REQUIRE( desc.cargo_slots == 0 );
    REQUIRE( desc.cargo_slots_occupies == 1 );
    REQUIRE( desc.on_death.holds<UnitDeathAction::capture>() );
    REQUIRE( desc.canonical_base == nothing );
    REQUIRE( desc.expertise == nothing );
    REQUIRE( desc.promotion ==
             UnitPromotion_t{ UnitPromotion::occupation{} } );
    unordered_map<e_unit_type,
                  unordered_set<e_unit_type_modifier>>
        expected_modifiers{
            { e_unit_type::missionary,
              { e_unit_type_modifier::blessing } },
            { e_unit_type::scout,
              { e_unit_type_modifier::horses } },
            { e_unit_type::pioneer,
              { e_unit_type_modifier::tools } },
            { e_unit_type::soldier,
              { e_unit_type_modifier::muskets } },
            { e_unit_type::dragoon,
              { e_unit_type_modifier::horses,
                e_unit_type_modifier::muskets } },
        };
    REQUIRE( desc.modifiers == expected_modifiers );
    // Derived fields.
    REQUIRE( desc.type == e_unit_type::free_colonist );
    REQUIRE( desc.is_derived == false );
    REQUIRE( desc.can_attack() == false );
    REQUIRE( desc.is_military_unit() == false );
  }
  SECTION( "veteran_dragoon" ) {
    UnitTypeAttributes const& desc =
        unit_attr( e_unit_type::veteran_dragoon );
    REQUIRE( desc.name == "Veteran Dragoon" );
    REQUIRE( desc.tile == e_tile::veteran_dragoon );
    REQUIRE( desc.nat_icon_front == false );
    REQUIRE( desc.nat_icon_position == e_direction::sw );
    REQUIRE( desc.ship == false );
    REQUIRE( desc.visibility == 1 );
    REQUIRE( desc.movement_points == 4 );
    REQUIRE( desc.attack_points == 4 );
    REQUIRE( desc.defense_points == 4 );
    REQUIRE( desc.cargo_slots == 0 );
    REQUIRE( desc.cargo_slots_occupies == 1 );
    REQUIRE( desc.on_death ==
             UnitDeathAction_t{ UnitDeathAction::demote{
                 .lose = { e_unit_type_modifier::horses } } } );
    REQUIRE( desc.canonical_base ==
             e_unit_type::veteran_colonist );
    REQUIRE( desc.expertise == nothing );
    REQUIRE(
        desc.promotion ==
        UnitPromotion_t{ UnitPromotion::modifier{
            .kind = e_unit_type_modifier::independence } } );
    unordered_map<e_unit_type,
                  unordered_set<e_unit_type_modifier>>
        expected_modifiers{};
    REQUIRE( desc.modifiers == expected_modifiers );
    // Derived fields.
    REQUIRE( desc.type == e_unit_type::veteran_dragoon );
    REQUIRE( desc.is_derived == true );
    REQUIRE( desc.can_attack() == true );
    REQUIRE( desc.is_military_unit() == true );
  }
  SECTION( "scout" ) {
    UnitTypeAttributes const& desc =
        unit_attr( e_unit_type::scout );
    REQUIRE( desc.name == "Scout" );
    REQUIRE( desc.tile == e_tile::scout );
    REQUIRE( desc.nat_icon_front == false );
    REQUIRE( desc.nat_icon_position == e_direction::sw );
    REQUIRE( desc.ship == false );
    REQUIRE( desc.visibility == 2 );
    REQUIRE( desc.movement_points == 4 );
    REQUIRE( desc.attack_points == 1 );
    REQUIRE( desc.defense_points == 1 );
    REQUIRE( desc.cargo_slots == 0 );
    REQUIRE( desc.cargo_slots_occupies == 1 );
    REQUIRE( desc.on_death ==
             UnitDeathAction_t{ UnitDeathAction::destroy{} } );
    REQUIRE( desc.canonical_base == e_unit_type::free_colonist );
    REQUIRE( desc.expertise == nothing );
    REQUIRE( desc.promotion ==
             UnitPromotion_t{ UnitPromotion::expertise{
                 .kind = e_unit_activity::scouting } } );
    unordered_map<e_unit_type,
                  unordered_set<e_unit_type_modifier>>
        expected_modifiers{};
    REQUIRE( desc.modifiers == expected_modifiers );
    // Derived fields.
    REQUIRE( desc.type == e_unit_type::scout );
    REQUIRE( desc.is_derived == true );
    REQUIRE( desc.can_attack() == true );
    REQUIRE( desc.is_military_unit() == true );
  }
}

TEST_CASE( "[utype] convert_commodity_to_modifier" ) {
  auto*     f = convert_commodity_to_modifier;
  Commodity c{ .type = e_commodity::food, .quantity = 0 };

  // Zero.
  c.quantity = 0;
  c.type     = e_commodity::food;
  REQUIRE( f( c ) == nothing );
  c.type = e_commodity::trade_goods;
  REQUIRE( f( c ) == nothing );
  c.type = e_commodity::horses;
  REQUIRE( f( c ) == nothing );
  c.type = e_commodity::muskets;
  REQUIRE( f( c ) == nothing );
  c.type = e_commodity::tools;
  REQUIRE( f( c ) == nothing );

  // One.
  c.quantity = 1;
  c.type     = e_commodity::food;
  REQUIRE( f( c ) == nothing );
  c.type = e_commodity::trade_goods;
  REQUIRE( f( c ) == nothing );
  c.type = e_commodity::horses;
  REQUIRE( f( c ) == nothing );
  c.type = e_commodity::muskets;
  REQUIRE( f( c ) == nothing );
  c.type = e_commodity::tools;
  REQUIRE( f( c ) == nothing );

  // 19.
  c.quantity = 19;
  c.type     = e_commodity::food;
  REQUIRE( f( c ) == nothing );
  c.type = e_commodity::trade_goods;
  REQUIRE( f( c ) == nothing );
  c.type = e_commodity::horses;
  REQUIRE( f( c ) == nothing );
  c.type = e_commodity::muskets;
  REQUIRE( f( c ) == nothing );
  c.type = e_commodity::tools;
  REQUIRE( f( c ) == nothing );

  // 20.
  c.quantity = 20;
  c.type     = e_commodity::food;
  REQUIRE( f( c ) == nothing );
  c.type = e_commodity::trade_goods;
  REQUIRE( f( c ) == nothing );
  c.type = e_commodity::horses;
  REQUIRE( f( c ) == nothing );
  c.type = e_commodity::muskets;
  REQUIRE( f( c ) == nothing );
  c.type = e_commodity::tools;
  REQUIRE( f( c ) == UnitTypeModifierFromCommodity{
                         .modifier = e_unit_type_modifier::tools,
                         .comm_quantity_used = 20 } );

  // 39.
  c.quantity = 39;
  c.type     = e_commodity::food;
  REQUIRE( f( c ) == nothing );
  c.type = e_commodity::trade_goods;
  REQUIRE( f( c ) == nothing );
  c.type = e_commodity::horses;
  REQUIRE( f( c ) == nothing );
  c.type = e_commodity::muskets;
  REQUIRE( f( c ) == nothing );
  c.type = e_commodity::tools;
  REQUIRE( f( c ) == UnitTypeModifierFromCommodity{
                         .modifier = e_unit_type_modifier::tools,
                         .comm_quantity_used = 20 } );

  // 49.
  c.quantity = 49;
  c.type     = e_commodity::food;
  REQUIRE( f( c ) == nothing );
  c.type = e_commodity::trade_goods;
  REQUIRE( f( c ) == nothing );
  c.type = e_commodity::horses;
  REQUIRE( f( c ) == nothing );
  c.type = e_commodity::muskets;
  REQUIRE( f( c ) == nothing );
  c.type = e_commodity::tools;
  REQUIRE( f( c ) == UnitTypeModifierFromCommodity{
                         .modifier = e_unit_type_modifier::tools,
                         .comm_quantity_used = 40 } );

  // 50.
  c.quantity = 50;
  c.type     = e_commodity::food;
  REQUIRE( f( c ) == nothing );
  c.type = e_commodity::trade_goods;
  REQUIRE( f( c ) == nothing );
  c.type = e_commodity::horses;
  REQUIRE( f( c ) ==
           UnitTypeModifierFromCommodity{
               .modifier = e_unit_type_modifier::horses,
               .comm_quantity_used = 50 } );
  c.type = e_commodity::muskets;
  REQUIRE( f( c ) ==
           UnitTypeModifierFromCommodity{
               .modifier = e_unit_type_modifier::muskets,
               .comm_quantity_used = 50 } );
  c.type = e_commodity::tools;
  REQUIRE( f( c ) == UnitTypeModifierFromCommodity{
                         .modifier = e_unit_type_modifier::tools,
                         .comm_quantity_used = 40 } );

  // 99.
  c.quantity = 99;
  c.type     = e_commodity::food;
  REQUIRE( f( c ) == nothing );
  c.type = e_commodity::trade_goods;
  REQUIRE( f( c ) == nothing );
  c.type = e_commodity::horses;
  REQUIRE( f( c ) ==
           UnitTypeModifierFromCommodity{
               .modifier = e_unit_type_modifier::horses,
               .comm_quantity_used = 50 } );
  c.type = e_commodity::muskets;
  REQUIRE( f( c ) ==
           UnitTypeModifierFromCommodity{
               .modifier = e_unit_type_modifier::muskets,
               .comm_quantity_used = 50 } );
  c.type = e_commodity::tools;
  REQUIRE( f( c ) == UnitTypeModifierFromCommodity{
                         .modifier = e_unit_type_modifier::tools,
                         .comm_quantity_used = 80 } );

  // 100.
  c.quantity = 100;
  c.type     = e_commodity::food;
  REQUIRE( f( c ) == nothing );
  c.type = e_commodity::trade_goods;
  REQUIRE( f( c ) == nothing );
  c.type = e_commodity::horses;
  REQUIRE( f( c ) ==
           UnitTypeModifierFromCommodity{
               .modifier = e_unit_type_modifier::horses,
               .comm_quantity_used = 50 } );
  c.type = e_commodity::muskets;
  REQUIRE( f( c ) ==
           UnitTypeModifierFromCommodity{
               .modifier = e_unit_type_modifier::muskets,
               .comm_quantity_used = 50 } );
  c.type = e_commodity::tools;
  REQUIRE( f( c ) == UnitTypeModifierFromCommodity{
                         .modifier = e_unit_type_modifier::tools,
                         .comm_quantity_used = 100 } );

  // 150.
  c.quantity = 150;
  c.type     = e_commodity::food;
  REQUIRE( f( c ) == nothing );
  c.type = e_commodity::trade_goods;
  REQUIRE( f( c ) == nothing );
  c.type = e_commodity::horses;
  REQUIRE( f( c ) ==
           UnitTypeModifierFromCommodity{
               .modifier = e_unit_type_modifier::horses,
               .comm_quantity_used = 50 } );
  c.type = e_commodity::muskets;
  REQUIRE( f( c ) ==
           UnitTypeModifierFromCommodity{
               .modifier = e_unit_type_modifier::muskets,
               .comm_quantity_used = 50 } );
  c.type = e_commodity::tools;
  REQUIRE( f( c ) == UnitTypeModifierFromCommodity{
                         .modifier = e_unit_type_modifier::tools,
                         .comm_quantity_used = 100 } );
}

// This test case contains a random selection of cases.
TEST_CASE( "[utype] UnitType creation" ) {
  using UT = e_unit_type;
  auto f   = []( UT type, UT base_type ) {
    return UnitType::create( type, base_type );
  };
  // Same types (base);
  REQUIRE(
      f( UT::free_colonist, UT::free_colonist ).has_value() );
  REQUIRE( f( UT::free_colonist, UT::free_colonist )->type() ==
           UT::free_colonist );
  REQUIRE(
      f( UT::free_colonist, UT::free_colonist )->base_type() ==
      UT::free_colonist );
  REQUIRE( f( UT::expert_fur_trapper, UT::expert_fur_trapper )
               .has_value() );
  REQUIRE( f( UT::expert_fur_trapper, UT::expert_fur_trapper )
               ->type() == UT::expert_fur_trapper );
  REQUIRE( f( UT::expert_fur_trapper, UT::expert_fur_trapper )
               ->base_type() == UT::expert_fur_trapper );
  // Same types (derived);
  REQUIRE( !f( UT::dragoon, UT::dragoon ).has_value() );
  REQUIRE( !f( UT::artillery, UT::artillery ).has_value() );
  // Invalid.
  REQUIRE(
      !f( UT::petty_criminal, UT::free_colonist ).has_value() );
  REQUIRE( !f( UT::expert_fur_trapper, UT::veteran_dragoon )
                .has_value() );
  REQUIRE( !f( UT::artillery, UT::free_colonist ).has_value() );
  REQUIRE( !f( UT::damaged_artillery, UT::free_colonist )
                .has_value() );
  REQUIRE( !f( UT::indentured_servant, UT::petty_criminal )
                .has_value() );
  REQUIRE(
      !f( UT::veteran_dragoon, UT::expert_farmer ).has_value() );
  REQUIRE(
      !f( UT::hardy_pioneer, UT::expert_farmer ).has_value() );
  REQUIRE(
      !f( UT::seasoned_scout, UT::free_colonist ).has_value() );
  REQUIRE( !f( UT::jesuit_missionary, UT::hardy_colonist )
                .has_value() );
  REQUIRE( !f( UT::continental_cavalry, UT::free_colonist )
                .has_value() );
  REQUIRE( !f( UT::continental_cavalry, UT::indentured_servant )
                .has_value() );
  REQUIRE(
      !f( UT::continental_cavalry, UT::dragoon ).has_value() );
  REQUIRE( !f( UT::continental_cavalry, UT::veteran_dragoon )
                .has_value() );
  // Valid.
  REQUIRE( f( UT::dragoon, UT::expert_farmer ).has_value() );
  REQUIRE( f( UT::dragoon, UT::expert_farmer )->type() ==
           UT::dragoon );
  REQUIRE( f( UT::dragoon, UT::expert_farmer )->base_type() ==
           UT::expert_farmer );
  REQUIRE( f( UT::soldier, UT::expert_farmer ).has_value() );
  REQUIRE( f( UT::soldier, UT::expert_farmer )->type() ==
           UT::soldier );
  REQUIRE( f( UT::soldier, UT::expert_farmer )->base_type() ==
           UT::expert_farmer );
  REQUIRE( f( UT::pioneer, UT::expert_farmer ).has_value() );
  REQUIRE( f( UT::pioneer, UT::expert_farmer )->type() ==
           UT::pioneer );
  REQUIRE( f( UT::pioneer, UT::expert_farmer )->base_type() ==
           UT::expert_farmer );
  REQUIRE( f( UT::pioneer, UT::free_colonist ).has_value() );
  REQUIRE( f( UT::pioneer, UT::free_colonist )->type() ==
           UT::pioneer );
  REQUIRE( f( UT::pioneer, UT::free_colonist )->base_type() ==
           UT::free_colonist );
  REQUIRE(
      f( UT::pioneer, UT::indentured_servant ).has_value() );
  REQUIRE( f( UT::pioneer, UT::indentured_servant )->type() ==
           UT::pioneer );
  REQUIRE(
      f( UT::pioneer, UT::indentured_servant )->base_type() ==
      UT::indentured_servant );
  REQUIRE( f( UT::scout, UT::free_colonist ).has_value() );
  REQUIRE( f( UT::scout, UT::free_colonist )->type() ==
           UT::scout );
  REQUIRE( f( UT::scout, UT::free_colonist )->base_type() ==
           UT::free_colonist );
  REQUIRE( f( UT::scout, UT::hardy_colonist ).has_value() );
  REQUIRE( f( UT::scout, UT::hardy_colonist )->type() ==
           UT::scout );
  REQUIRE( f( UT::scout, UT::hardy_colonist )->base_type() ==
           UT::hardy_colonist );
  REQUIRE(
      f( UT::hardy_pioneer, UT::hardy_colonist ).has_value() );
  REQUIRE( f( UT::hardy_pioneer, UT::hardy_colonist )->type() ==
           UT::hardy_pioneer );
  REQUIRE(
      f( UT::hardy_pioneer, UT::hardy_colonist )->base_type() ==
      UT::hardy_colonist );
  REQUIRE( f( UT::missionary, UT::hardy_colonist ).has_value() );
  REQUIRE( f( UT::missionary, UT::hardy_colonist )->type() ==
           UT::missionary );
  REQUIRE(
      f( UT::missionary, UT::hardy_colonist )->base_type() ==
      UT::hardy_colonist );
  REQUIRE( f( UT::jesuit_missionary, UT::jesuit_colonist )
               .has_value() );
  REQUIRE(
      f( UT::jesuit_missionary, UT::jesuit_colonist )->type() ==
      UT::jesuit_missionary );
  REQUIRE( f( UT::jesuit_missionary, UT::jesuit_colonist )
               ->base_type() == UT::jesuit_colonist );
  REQUIRE( f( UT::continental_cavalry, UT::veteran_colonist )
               .has_value() );
  REQUIRE( f( UT::continental_cavalry, UT::veteran_colonist )
               ->type() == UT::continental_cavalry );
  REQUIRE( f( UT::continental_cavalry, UT::veteran_colonist )
               ->base_type() == UT::veteran_colonist );
}

// This test case contains a random selection of cases.
TEST_CASE( "[utype] unit_type_modifiers" ) {
  using UT  = e_unit_type;
  using Mod = e_unit_type_modifier;
  using US  = unordered_set<Mod>;
  auto f    = []( UT base_type, UT type ) -> maybe<US const&> {
    UNWRAP_RETURN( ut, UnitType::create( type, base_type ) );
    return ut.unit_type_modifiers();
  };
  // Same types (base);
  REQUIRE( f( UT::free_colonist, UT::free_colonist ) == US{} );
  REQUIRE( f( UT::expert_fur_trapper, UT::expert_fur_trapper ) ==
           US{} );
  // Same types (derived);
  REQUIRE( f( UT::dragoon, UT::dragoon ) == nothing );
  REQUIRE( f( UT::artillery, UT::artillery ) == nothing );
  // Invalid.
  REQUIRE( f( UT::free_colonist, UT::petty_criminal ) ==
           nothing );
  REQUIRE( f( UT::veteran_dragoon, UT::expert_fur_trapper ) ==
           nothing );
  REQUIRE( f( UT::free_colonist, UT::artillery ) == nothing );
  REQUIRE( f( UT::free_colonist, UT::damaged_artillery ) ==
           nothing );
  REQUIRE( f( UT::petty_criminal, UT::indentured_servant ) ==
           nothing );
  REQUIRE( f( UT::expert_farmer, UT::veteran_dragoon ) ==
           nothing );
  REQUIRE( f( UT::expert_farmer, UT::hardy_pioneer ) ==
           nothing );
  REQUIRE( f( UT::free_colonist, UT::seasoned_scout ) ==
           nothing );
  REQUIRE( f( UT::hardy_colonist, UT::jesuit_missionary ) ==
           nothing );
  REQUIRE( f( UT::free_colonist, UT::continental_cavalry ) ==
           nothing );
  REQUIRE( f( UT::indentured_servant,
              UT::continental_cavalry ) == nothing );
  REQUIRE( f( UT::dragoon, UT::continental_cavalry ) ==
           nothing );
  REQUIRE( f( UT::veteran_dragoon, UT::continental_cavalry ) ==
           nothing );
  // Valid.
  REQUIRE( f( UT::expert_farmer, UT::dragoon ) ==
           US{ Mod::horses, Mod::muskets } );
  REQUIRE( f( UT::expert_farmer, UT::soldier ) ==
           US{ Mod::muskets } );
  REQUIRE( f( UT::expert_farmer, UT::pioneer ) ==
           US{ Mod::tools } );
  REQUIRE( f( UT::free_colonist, UT::pioneer ) ==
           US{ Mod::tools } );
  REQUIRE( f( UT::indentured_servant, UT::pioneer ) ==
           US{ Mod::tools } );
  REQUIRE( f( UT::free_colonist, UT::scout ) ==
           US{ Mod::horses } );
  REQUIRE( f( UT::hardy_colonist, UT::scout ) ==
           US{ Mod::horses } );
  REQUIRE( f( UT::hardy_colonist, UT::hardy_pioneer ) ==
           US{ Mod::tools } );
  REQUIRE( f( UT::hardy_colonist, UT::missionary ) ==
           US{ Mod::blessing } );
  REQUIRE( f( UT::jesuit_colonist, UT::jesuit_missionary ) ==
           US{ Mod::blessing } );
  REQUIRE( f( UT::veteran_colonist, UT::continental_cavalry ) ==
           US{ Mod::muskets, Mod::horses, Mod::independence } );
}

TEST_CASE( "[utype] on_death_demoted_type" ) {
  auto* f  = on_death_demoted_type;
  using UT = e_unit_type;
  // No demoting.
  REQUIRE( f( UnitType::create( UT::free_colonist ) ) ==
           nothing );
  REQUIRE( f( UnitType::create( UT::indentured_servant ) ) ==
           nothing );
  REQUIRE( f( UnitType::create( UT::expert_sugar_planter ) ) ==
           nothing );
  REQUIRE( f( UnitType::create( UT::damaged_artillery ) ) ==
           nothing );
  REQUIRE( f( UnitType::create( UT::caravel ) ) == nothing );
  REQUIRE( f( UnitType::create( UT::cavalry ) ) == nothing );
  // Demoting.
  REQUIRE( f( UnitType::create( UT::soldier ) ) ==
           UnitType::create( UT::free_colonist ) );
  REQUIRE(
      f( UnitType::create( UT::soldier, UT::indentured_servant )
             .value() ) ==
      UnitType::create( UT::indentured_servant ) );
  REQUIRE( f( UnitType::create( UT::dragoon ) ) ==
           UnitType::create( UT::soldier ) );
  REQUIRE(
      f( UnitType::create( UT::dragoon, UT::indentured_servant )
             .value() ) ==
      UnitType::create( UT::soldier, UT::indentured_servant )
          .value() );
  REQUIRE( f( UnitType::create( UT::veteran_soldier ) ) ==
           UnitType::create( UT::veteran_colonist ) );
  REQUIRE( f( UnitType::create( UT::veteran_dragoon ) ) ==
           UnitType::create( UT::veteran_soldier ) );
  REQUIRE( f( UnitType::create( UT::continental_army ) ) ==
           UnitType::create( UT::veteran_colonist ) );
  REQUIRE( f( UnitType::create( UT::continental_cavalry ) ) ==
           UnitType::create( UT::continental_army ) );
  REQUIRE( f( UnitType::create( UT::artillery ) ) ==
           UnitType::create( UT::damaged_artillery ) );
}

TEST_CASE( "[utype] add_unit_type_modifiers" ) {
  auto* f   = add_unit_type_modifiers;
  using UT  = e_unit_type;
  using Mod = e_unit_type_modifier;

  // Empty modifiers.
  REQUIRE( f( UnitType::create( UT::petty_criminal ), {} ) ==
           UnitType::create( UT::petty_criminal ) );
  REQUIRE( f( UnitType::create( UT::dragoon ), {} ) ==
           UnitType::create( UT::dragoon ) );
  // Invalid.
  REQUIRE( f( UnitType::create( UT::veteran_soldier ),
              { Mod::muskets } ) == nothing );
  REQUIRE( f( UnitType::create( UT::veteran_soldier,
                                UT::veteran_colonist )
                  .value(),
              { Mod::tools } ) == nothing );
  REQUIRE( f( UnitType::create( UT::veteran_soldier ),
              { Mod::strength } ) == nothing );
  REQUIRE( f( UnitType::create( UT::veteran_dragoon ),
              { Mod::horses } ) == nothing );
  REQUIRE( f( UnitType::create( UT::veteran_soldier ),
              { Mod::horses, Mod::independence, Mod::tools } ) ==
           nothing );
  REQUIRE( f( UnitType::create( UT::artillery ),
              { Mod::strength } ) == nothing );
  // Valid.
  REQUIRE(
      f( UnitType::create( UT::soldier, UT::indentured_servant )
             .value(),
         { Mod::horses } ) ==
      UnitType::create( UT::dragoon, UT::indentured_servant ) );
  REQUIRE( f( UnitType::create( UT::veteran_soldier ),
              { Mod::horses, Mod::independence } ) ==
           UnitType::create( UT::continental_cavalry ) );
  REQUIRE( f( UnitType::create( UT::damaged_artillery ),
              { Mod::strength } ) ==
           UnitType::create( UT::artillery ) );
}

TEST_CASE( "[utype] lua bindings" ) {
  lua::state& st = lua_global_state();

  auto script = R"(
    local ut
    -- free_colonist
    ut = utype.UnitType.create( e.unit_type.free_colonist )
    assert( ut )
    assert( ut:type() == e.unit_type.free_colonist )
    assert( ut:base_type() == e.unit_type.free_colonist )
    -- dragoon
    ut = utype.UnitType.create( e.unit_type.dragoon )
    assert( ut )
    assert( ut:type() == e.unit_type.dragoon )
    assert( ut:base_type() == e.unit_type.free_colonist )
    -- veteran_soldier
    ut = utype.UnitType.create( e.unit_type.veteran_soldier )
    assert( ut )
    assert( ut:type() == e.unit_type.veteran_soldier )
    assert( ut:base_type() == e.unit_type.veteran_colonist )
    -- pioneer
    ut = utype.UnitType.create_with_base(
        e.unit_type.pioneer, e.unit_type.expert_farmer )
    assert( ut )
    assert( ut:type() == e.unit_type.pioneer )
    assert( ut:base_type() == e.unit_type.expert_farmer )
  )";
  REQUIRE( st.script.run_safe( script ) == valid );

  script  = R"(
    local ut
    utype.UnitType.create_with_base(
        e.unit_type.hardy_pioneer, e.unit_type.expert_farmer )
  )";
  auto xp = st.script.run_safe( script );
  REQUIRE( !xp.valid() );
  REQUIRE_THAT(
      xp.error(),
      Contains(
          "failed to create UnitType with type=hardy_pioneer "
          "and base_type=expert_farmer." ) );
}

} // namespace
} // namespace rn
