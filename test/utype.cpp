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
#include "src/config-files.hpp"
#include "src/lua.hpp"
#include "src/luapp/state.hpp"

// Revolution Now (config)
#include "../config/rcl/units.inl"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

using Catch::Contains;

TEST_CASE( "[utype] inventory_traits" ) {
  auto& traits = config_units.composition
                     .inventory_traits[e_unit_inventory::tools];
  REQUIRE( traits.commodity == e_commodity::tools );
  REQUIRE( traits.min_quantity == 20 );
  REQUIRE( traits.max_quantity == 100 );
  REQUIRE( traits.multiple == 20 );
}

TEST_CASE( "[utype] inventory_to_modifier" ) {
  SECTION( "gold" ) {
    auto mod_info =
        inventory_to_modifier( e_unit_inventory::gold );
    REQUIRE( !mod_info.has_value() );
  }
  SECTION( "tools" ) {
    auto mod = inventory_to_modifier( e_unit_inventory::tools );
    REQUIRE( mod.has_value() );
    REQUIRE( mod == e_unit_type_modifier::tools );
  }
}

TEST_CASE( "[utype] commodity_to_inventory" ) {
  auto f = commodity_to_inventory;
  REQUIRE( f( e_commodity::cigars ) == nothing );
  REQUIRE( f( e_commodity::cloth ) == nothing );
  REQUIRE( f( e_commodity::coats ) == nothing );
  REQUIRE( f( e_commodity::cotton ) == nothing );
  REQUIRE( f( e_commodity::food ) == nothing );
  REQUIRE( f( e_commodity::fur ) == nothing );
  REQUIRE( f( e_commodity::horses ) == nothing );
  REQUIRE( f( e_commodity::lumber ) == nothing );
  REQUIRE( f( e_commodity::muskets ) == nothing );
  REQUIRE( f( e_commodity::ore ) == nothing );
  REQUIRE( f( e_commodity::rum ) == nothing );
  REQUIRE( f( e_commodity::silver ) == nothing );
  REQUIRE( f( e_commodity::sugar ) == nothing );
  REQUIRE( f( e_commodity::tobacco ) == nothing );
  REQUIRE( f( e_commodity::tools ) == e_unit_inventory::tools );
  REQUIRE( f( e_commodity::trade_goods ) == nothing );
}

TEST_CASE( "[utype] inventory_to_commodity" ) {
  auto f = inventory_to_commodity;
  REQUIRE( f( e_unit_inventory::gold ) == nothing );
  REQUIRE( f( e_unit_inventory::tools ) == e_commodity::tools );
}

TEST_CASE( "[utype] unit type attributes deserialization" ) {
  SECTION( "expert_cotton_planter" ) {
    UnitTypeAttributes const& desc =
        unit_attr( e_unit_type::expert_cotton_planter );
    REQUIRE( desc.name == "Expert Cotton Planter" );
    REQUIRE( desc.tile == e_tile::expert_cotton_planter );
    REQUIRE( desc.nat_icon_front == false );
    REQUIRE( desc.nat_icon_position == e_direction::sw );
    REQUIRE( desc.ship == false );
    REQUIRE( desc.human == e_unit_human::yes );
    REQUIRE( desc.visibility == 1 );
    REQUIRE( desc.movement_points == 1 );
    REQUIRE( desc.attack_points == 0 );
    REQUIRE( desc.defense_points == 1 );
    REQUIRE( desc.road_turns == nothing );
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
    REQUIRE( desc.inventory_types ==
             unordered_set<e_unit_inventory>{} );
    // Derived fields.
    REQUIRE( desc.type == e_unit_type::expert_cotton_planter );
    REQUIRE( desc.is_derived == false );
    REQUIRE( can_attack( desc ) == false );
    REQUIRE( is_military_unit( desc ) == false );
  }
  SECTION( "petty_criminal" ) {
    UnitTypeAttributes const& desc =
        unit_attr( e_unit_type::petty_criminal );
    REQUIRE( desc.name == "Petty Criminal" );
    REQUIRE( desc.tile == e_tile::petty_criminal );
    REQUIRE( desc.nat_icon_front == false );
    REQUIRE( desc.nat_icon_position == e_direction::sw );
    REQUIRE( desc.ship == false );
    REQUIRE( desc.human == e_unit_human::yes );
    REQUIRE( desc.visibility == 1 );
    REQUIRE( desc.movement_points == 1 );
    REQUIRE( desc.attack_points == 0 );
    REQUIRE( desc.defense_points == 1 );
    REQUIRE( desc.road_turns == nothing );
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
    REQUIRE( desc.inventory_types ==
             unordered_set<e_unit_inventory>{} );
    // Derived fields.
    REQUIRE( desc.type == e_unit_type::petty_criminal );
    REQUIRE( desc.is_derived == false );
    REQUIRE( can_attack( desc ) == false );
    REQUIRE( is_military_unit( desc ) == false );
  }
  SECTION( "free_colonist" ) {
    UnitTypeAttributes const& desc =
        unit_attr( e_unit_type::free_colonist );
    REQUIRE( desc.name == "Free Colonist" );
    REQUIRE( desc.tile == e_tile::free_colonist );
    REQUIRE( desc.nat_icon_front == false );
    REQUIRE( desc.nat_icon_position == e_direction::sw );
    REQUIRE( desc.ship == false );
    REQUIRE( desc.human == e_unit_human::yes );
    REQUIRE( desc.visibility == 1 );
    REQUIRE( desc.movement_points == 1 );
    REQUIRE( desc.attack_points == 0 );
    REQUIRE( desc.defense_points == 1 );
    REQUIRE( desc.road_turns == nothing );
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
    REQUIRE( desc.inventory_types ==
             unordered_set<e_unit_inventory>{} );
    // Derived fields.
    REQUIRE( desc.type == e_unit_type::free_colonist );
    REQUIRE( desc.is_derived == false );
    REQUIRE( can_attack( desc ) == false );
    REQUIRE( is_military_unit( desc ) == false );
  }
  SECTION( "veteran_dragoon" ) {
    UnitTypeAttributes const& desc =
        unit_attr( e_unit_type::veteran_dragoon );
    REQUIRE( desc.name == "Veteran Dragoon" );
    REQUIRE( desc.tile == e_tile::veteran_dragoon );
    REQUIRE( desc.nat_icon_front == false );
    REQUIRE( desc.nat_icon_position == e_direction::sw );
    REQUIRE( desc.ship == false );
    REQUIRE( desc.human == e_unit_human::from_base );
    REQUIRE( desc.visibility == 1 );
    REQUIRE( desc.movement_points == 4 );
    REQUIRE( desc.attack_points == 4 );
    REQUIRE( desc.defense_points == 4 );
    REQUIRE( desc.road_turns == nothing );
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
    REQUIRE( desc.inventory_types ==
             unordered_set<e_unit_inventory>{} );
    // Derived fields.
    REQUIRE( desc.type == e_unit_type::veteran_dragoon );
    REQUIRE( desc.is_derived == true );
    REQUIRE( can_attack( desc ) == true );
    REQUIRE( is_military_unit( desc ) == true );
  }
  SECTION( "scout" ) {
    UnitTypeAttributes const& desc =
        unit_attr( e_unit_type::scout );
    REQUIRE( desc.name == "Scout" );
    REQUIRE( desc.tile == e_tile::scout );
    REQUIRE( desc.nat_icon_front == false );
    REQUIRE( desc.nat_icon_position == e_direction::sw );
    REQUIRE( desc.ship == false );
    REQUIRE( desc.human == e_unit_human::from_base );
    REQUIRE( desc.visibility == 2 );
    REQUIRE( desc.movement_points == 4 );
    REQUIRE( desc.attack_points == 1 );
    REQUIRE( desc.defense_points == 1 );
    REQUIRE( desc.road_turns == nothing );
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
    REQUIRE( desc.inventory_types ==
             unordered_set<e_unit_inventory>{} );
    // Derived fields.
    REQUIRE( desc.type == e_unit_type::scout );
    REQUIRE( desc.is_derived == true );
    REQUIRE( can_attack( desc ) == true );
    REQUIRE( is_military_unit( desc ) == true );
  }
  SECTION( "pioneer" ) {
    UnitTypeAttributes const& desc =
        unit_attr( e_unit_type::pioneer );
    REQUIRE( desc.name == "Pioneer" );
    REQUIRE( desc.tile == e_tile::pioneer );
    REQUIRE( desc.nat_icon_front == false );
    REQUIRE( desc.nat_icon_position == e_direction::sw );
    REQUIRE( desc.ship == false );
    REQUIRE( desc.human == e_unit_human::from_base );
    REQUIRE( desc.visibility == 1 );
    REQUIRE( desc.movement_points == 1 );
    REQUIRE( desc.attack_points == 0 );
    REQUIRE( desc.defense_points == 1 );
    REQUIRE( desc.road_turns == 4 );
    REQUIRE( desc.cargo_slots == 0 );
    REQUIRE( desc.cargo_slots_occupies == 1 );
    REQUIRE( desc.on_death ==
             UnitDeathAction_t{ UnitDeathAction::capture{} } );
    REQUIRE( desc.canonical_base == e_unit_type::free_colonist );
    REQUIRE( desc.expertise == nothing );
    REQUIRE( desc.promotion ==
             UnitPromotion_t{ UnitPromotion::expertise{
                 .kind = e_unit_activity::pioneering } } );
    unordered_map<e_unit_type,
                  unordered_set<e_unit_type_modifier>>
        expected_modifiers{};
    REQUIRE( desc.modifiers == expected_modifiers );
    REQUIRE( desc.inventory_types ==
             unordered_set<e_unit_inventory>{
                 e_unit_inventory::tools } );
    // Derived fields.
    REQUIRE( desc.type == e_unit_type::pioneer );
    REQUIRE( desc.is_derived == true );
    REQUIRE( can_attack( desc ) == false );
    REQUIRE( is_military_unit( desc ) == false );
  }
  SECTION( "large_treasure" ) {
    UnitTypeAttributes const& desc =
        unit_attr( e_unit_type::large_treasure );
    REQUIRE( desc.name == "Large Treasure" );
    REQUIRE( desc.tile == e_tile::large_treasure );
    REQUIRE( desc.nat_icon_front == false );
    REQUIRE( desc.nat_icon_position == e_direction::n );
    REQUIRE( desc.ship == false );
    REQUIRE( desc.human == e_unit_human::no );
    REQUIRE( desc.visibility == 1 );
    REQUIRE( desc.movement_points == 1 );
    REQUIRE( desc.attack_points == 0 );
    REQUIRE( desc.defense_points == 1 );
    REQUIRE( desc.road_turns == nothing );
    REQUIRE( desc.cargo_slots == 0 );
    REQUIRE( desc.cargo_slots_occupies == 6 );
    REQUIRE( desc.on_death ==
             UnitDeathAction_t{ UnitDeathAction::capture{} } );
    REQUIRE( desc.canonical_base == nothing );
    REQUIRE( desc.expertise == nothing );
    REQUIRE( desc.promotion == nothing );
    unordered_map<e_unit_type,
                  unordered_set<e_unit_type_modifier>>
        expected_modifiers{};
    REQUIRE( desc.modifiers == expected_modifiers );
    REQUIRE( desc.inventory_types ==
             unordered_set<e_unit_inventory>{
                 e_unit_inventory::gold } );
    // Derived fields.
    REQUIRE( desc.type == e_unit_type::large_treasure );
    REQUIRE( desc.is_derived == false );
    REQUIRE( can_attack( desc ) == false );
    REQUIRE( is_military_unit( desc ) == false );
  }
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
      !f( UT::veteran_dragoon, UT::free_colonist ).has_value() );
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
  auto f = []( UnitType ut,
               unordered_set<e_unit_type_modifier> const&
                   modifiers ) {
    return add_unit_type_modifiers( ut, modifiers );
  };
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

TEST_CASE( "[utype] rm_unit_type_modifiers" ) {
  auto f = []( UnitType ut,
               unordered_set<e_unit_type_modifier> const&
                   modifiers ) {
    return rm_unit_type_modifiers( ut, modifiers );
  };
  using UT  = e_unit_type;
  using Mod = e_unit_type_modifier;

  // Empty modifiers.
  REQUIRE( f( UnitType::create( UT::petty_criminal ), {} ) ==
           UnitType::create( UT::petty_criminal ) );
  REQUIRE( f( UnitType::create( UT::dragoon ), {} ) ==
           UnitType::create( UT::dragoon ) );
  // Invalid.
  REQUIRE( f( UnitType::create( UT::scout ),
              { Mod::muskets } ) == nothing );
  REQUIRE( f( UnitType::create( UT::veteran_soldier,
                                UT::veteran_colonist )
                  .value(),
              { Mod::tools } ) == nothing );
  REQUIRE( f( UnitType::create( UT::veteran_soldier ),
              { Mod::strength } ) == nothing );
  REQUIRE( f( UnitType::create( UT::veteran_colonist ),
              { Mod::horses } ) == nothing );
  REQUIRE( f( UnitType::create( UT::veteran_soldier ),
              { Mod::muskets, Mod::tools } ) == nothing );
  REQUIRE( f( UnitType::create( UT::damaged_artillery ),
              { Mod::strength } ) == nothing );
  REQUIRE( f( UnitType::create( UT::continental_cavalry ),
              { Mod::muskets } ) == nothing );
  // Valid.
  REQUIRE(
      f( UnitType::create( UT::dragoon, UT::indentured_servant )
             .value(),
         { Mod::horses } ) ==
      UnitType::create( UT::soldier, UT::indentured_servant )
          .value() );
  REQUIRE( f( UnitType::create( UT::continental_cavalry ),
              { Mod::horses, Mod::independence } ) ==
           UnitType::create( UT::veteran_soldier ) );
  REQUIRE( f( UnitType::create( UT::artillery ),
              { Mod::strength } ) ==
           UnitType::create( UT::damaged_artillery ) );
  REQUIRE(
      f( UnitType::create( UT::dragoon ), { Mod::muskets } ) ==
      UnitType::create( UT::scout ) );
}

TEST_CASE( "[utype] promoted_unit_type" ) {
  auto f = []( UnitType ut, e_unit_activity activity ) {
    return promoted_unit_type( ut, activity );
  };
  using UT  = e_unit_type;
  using Act = e_unit_activity;
  UnitType        ut;
  UnitType        expected;
  e_unit_activity act;

  SECTION( "base types" ) {
    // petty_criminal.
    ut       = UnitType::create( UT::petty_criminal );
    act      = Act::farming;
    expected = UnitType::create( UT::indentured_servant );
    REQUIRE( f( ut, act ) == expected );
    act = Act::bell_ringing;
    REQUIRE( f( ut, act ) == expected );
    act = Act::fighting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::scouting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::cotton_planting;
    REQUIRE( f( ut, act ) == expected );
    // indentured_servant.
    ut       = UnitType::create( UT::indentured_servant );
    act      = Act::farming;
    expected = UnitType::create( UT::free_colonist );
    REQUIRE( f( ut, act ) == expected );
    act = Act::bell_ringing;
    REQUIRE( f( ut, act ) == expected );
    act = Act::fighting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::scouting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::cotton_planting;
    REQUIRE( f( ut, act ) == expected );
    // free_colonist.
    ut       = UnitType::create( UT::free_colonist );
    act      = Act::farming;
    expected = UnitType::create( UT::expert_farmer );
    REQUIRE( f( ut, act ) == expected );
    act      = Act::bell_ringing;
    expected = UnitType::create( UT::elder_statesman );
    REQUIRE( f( ut, act ) == expected );
    act      = Act::fighting;
    expected = UnitType::create( UT::veteran_colonist );
    REQUIRE( f( ut, act ) == expected );
    act      = Act::scouting;
    expected = UnitType::create( UT::seasoned_colonist );
    REQUIRE( f( ut, act ) == expected );
    act      = Act::cotton_planting;
    expected = UnitType::create( UT::expert_cotton_planter );
    REQUIRE( f( ut, act ) == expected );
    // expert_cotton_planer.
    ut  = UnitType::create( UT::expert_cotton_planter );
    act = Act::farming;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::bell_ringing;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::fighting;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::scouting;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::cotton_planting;
    REQUIRE( f( ut, act ) == nothing );
    // veteran_colonist.
    ut  = UnitType::create( UT::veteran_colonist );
    act = Act::farming;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::bell_ringing;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::fighting;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::scouting;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::cotton_planting;
    REQUIRE( f( ut, act ) == nothing );
    // cavalry.
    ut  = UnitType::create( UT::cavalry );
    act = Act::farming;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::bell_ringing;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::fighting;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::scouting;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::cotton_planting;
    REQUIRE( f( ut, act ) == nothing );
    // damaged_artillery.
    ut  = UnitType::create( UT::damaged_artillery );
    act = Act::farming;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::bell_ringing;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::fighting;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::scouting;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::cotton_planting;
    REQUIRE( f( ut, act ) == nothing );
    // privateer.
    ut  = UnitType::create( UT::privateer );
    act = Act::farming;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::bell_ringing;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::fighting;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::scouting;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::cotton_planting;
    REQUIRE( f( ut, act ) == nothing );
    // wagon_train.
    ut  = UnitType::create( UT::wagon_train );
    act = Act::farming;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::bell_ringing;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::fighting;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::scouting;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::cotton_planting;
    REQUIRE( f( ut, act ) == nothing );
    // large_treasure.
    ut  = UnitType::create( UT::large_treasure );
    act = Act::farming;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::bell_ringing;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::fighting;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::scouting;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::cotton_planting;
    REQUIRE( f( ut, act ) == nothing );
  }
  SECTION( "non-expert modified colonists" ) {
    // pioneer/petty_criminal.
    ut = UnitType::create( UT::pioneer, UT::petty_criminal )
             .value();
    expected =
        UnitType::create( UT::pioneer, UT::indentured_servant )
            .value();
    act = Act::farming;
    REQUIRE( f( ut, act ) == expected );
    act = Act::bell_ringing;
    REQUIRE( f( ut, act ) == expected );
    act = Act::fighting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::scouting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::cotton_planting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::pioneering;
    REQUIRE( f( ut, act ) == expected );
    // pioneer/indentured_servant.
    ut = UnitType::create( UT::pioneer, UT::indentured_servant )
             .value();
    expected = UnitType::create( UT::pioneer, UT::free_colonist )
                   .value();
    act = Act::farming;
    REQUIRE( f( ut, act ) == expected );
    act = Act::bell_ringing;
    REQUIRE( f( ut, act ) == expected );
    act = Act::fighting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::scouting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::cotton_planting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::pioneering;
    REQUIRE( f( ut, act ) == expected );
    // pioneer/free_colonist.
    ut = UnitType::create( UT::pioneer, UT::free_colonist )
             .value();
    expected =
        UnitType::create( UT::hardy_pioneer, UT::hardy_colonist )
            .value();
    act = Act::farming;
    REQUIRE( f( ut, act ) == expected );
    act = Act::bell_ringing;
    REQUIRE( f( ut, act ) == expected );
    act = Act::fighting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::scouting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::cotton_planting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::pioneering;
    REQUIRE( f( ut, act ) == expected );
    // pioneer/expert_farmer.
    ut = UnitType::create( UT::pioneer, UT::expert_farmer )
             .value();
    act = Act::farming;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::bell_ringing;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::fighting;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::scouting;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::cotton_planting;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::pioneering;
    REQUIRE( f( ut, act ) == nothing );
    // dragoon/petty_criminal.
    ut = UnitType::create( UT::dragoon, UT::petty_criminal )
             .value();
    expected =
        UnitType::create( UT::dragoon, UT::indentured_servant )
            .value();
    act = Act::farming;
    REQUIRE( f( ut, act ) == expected );
    act = Act::bell_ringing;
    REQUIRE( f( ut, act ) == expected );
    act = Act::fighting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::scouting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::cotton_planting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::pioneering;
    REQUIRE( f( ut, act ) == expected );
    // dragoon/free_colonist.
    ut = UnitType::create( UT::dragoon, UT::free_colonist )
             .value();
    expected = UnitType::create( UT::veteran_dragoon,
                                 UT::veteran_colonist )
                   .value();
    act = Act::farming;
    REQUIRE( f( ut, act ) == expected );
    act = Act::bell_ringing;
    REQUIRE( f( ut, act ) == expected );
    act = Act::fighting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::scouting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::cotton_planting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::pioneering;
    REQUIRE( f( ut, act ) == expected );
    // dragoon/expert_farmer.
    ut = UnitType::create( UT::dragoon, UT::expert_farmer )
             .value();
    act = Act::farming;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::bell_ringing;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::fighting;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::scouting;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::cotton_planting;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::pioneering;
    REQUIRE( f( ut, act ) == nothing );
    // scout/free_colonist.
    ut =
        UnitType::create( UT::scout, UT::free_colonist ).value();
    expected = UnitType::create( UT::seasoned_scout,
                                 UT::seasoned_colonist )
                   .value();
    act = Act::farming;
    REQUIRE( f( ut, act ) == expected );
    act = Act::bell_ringing;
    REQUIRE( f( ut, act ) == expected );
    act = Act::fighting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::scouting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::cotton_planting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::pioneering;
    REQUIRE( f( ut, act ) == expected );
  }
  SECTION( "expert modified colonists" ) {
    // veteran_soldier
    ut       = UnitType::create( UT::veteran_soldier );
    expected = UnitType::create( UT::continental_army );
    act      = Act::farming;
    REQUIRE( f( ut, act ) == expected );
    act = Act::bell_ringing;
    REQUIRE( f( ut, act ) == expected );
    act = Act::fighting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::scouting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::cotton_planting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::pioneering;
    REQUIRE( f( ut, act ) == expected );
    // veteran_dragoon
    ut       = UnitType::create( UT::veteran_dragoon );
    expected = UnitType::create( UT::continental_cavalry );
    act      = Act::farming;
    REQUIRE( f( ut, act ) == expected );
    act = Act::bell_ringing;
    REQUIRE( f( ut, act ) == expected );
    act = Act::fighting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::scouting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::cotton_planting;
    REQUIRE( f( ut, act ) == expected );
    act = Act::pioneering;
    REQUIRE( f( ut, act ) == expected );
    // continental_army
    ut  = UnitType::create( UT::continental_army );
    act = Act::farming;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::bell_ringing;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::fighting;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::scouting;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::cotton_planting;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::pioneering;
    REQUIRE( f( ut, act ) == nothing );
    // continental_cavalry
    ut  = UnitType::create( UT::continental_cavalry );
    act = Act::farming;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::bell_ringing;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::fighting;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::scouting;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::cotton_planting;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::pioneering;
    REQUIRE( f( ut, act ) == nothing );
    // hardy_pioneer
    ut  = UnitType::create( UT::hardy_pioneer );
    act = Act::farming;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::bell_ringing;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::fighting;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::scouting;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::cotton_planting;
    REQUIRE( f( ut, act ) == nothing );
    act = Act::pioneering;
    REQUIRE( f( ut, act ) == nothing );
  }
}

TEST_CASE( "[utype] cleared_expertise" ) {
  auto* f  = cleared_expertise;
  using UT = e_unit_type;
  UnitType ut;
  UnitType expected;

  SECTION( "base types" ) {
    // petty_criminal.
    ut = UnitType::create( UT::petty_criminal );
    REQUIRE( f( ut ) == nothing );
    // indentured_servant.
    ut = UnitType::create( UT::indentured_servant );
    REQUIRE( f( ut ) == nothing );
    // free_colonist.
    ut = UnitType::create( UT::free_colonist );
    REQUIRE( f( ut ) == nothing );
    // expert_farmer.
    ut       = UnitType::create( UT::expert_farmer );
    expected = UnitType::create( UT::free_colonist );
    REQUIRE( f( ut ) == expected );
    // veteran_colonist.
    ut       = UnitType::create( UT::veteran_colonist );
    expected = UnitType::create( UT::free_colonist );
    REQUIRE( f( ut ) == expected );
    // jesuit_missionary.
    ut       = UnitType::create( UT::jesuit_colonist );
    expected = UnitType::create( UT::free_colonist );
    REQUIRE( f( ut ) == expected );
    // caravel.
    ut = UnitType::create( UT::caravel );
    REQUIRE( f( ut ) == nothing );
    // artillery.
    ut = UnitType::create( UT::artillery );
    REQUIRE( f( ut ) == nothing );
  }
  SECTION( "non-expert modified colonists" ) {
    // dragoon/petty_criminal.
    ut = UnitType::create( UT::dragoon, UT::petty_criminal )
             .value();
    REQUIRE( f( ut ) == nothing );
    // dragoon/indentured_servant.
    ut = UnitType::create( UT::dragoon, UT::indentured_servant )
             .value();
    REQUIRE( f( ut ) == nothing );
    // dragoon/free_colonist.
    ut = UnitType::create( UT::dragoon, UT::free_colonist )
             .value();
    REQUIRE( f( ut ) == nothing );
    // scout/expert_farmer.
    ut =
        UnitType::create( UT::scout, UT::expert_farmer ).value();
    expected =
        UnitType::create( UT::scout, UT::free_colonist ).value();
    REQUIRE( f( ut ) == expected );
    // scout/veteran_colonist.
    ut = UnitType::create( UT::scout, UT::veteran_colonist )
             .value();
    expected =
        UnitType::create( UT::scout, UT::free_colonist ).value();
    REQUIRE( f( ut ) == expected );
    // pioneer/jesuit_colonist.
    ut = UnitType::create( UT::pioneer, UT::jesuit_colonist )
             .value();
    expected = UnitType::create( UT::pioneer, UT::free_colonist )
                   .value();
    REQUIRE( f( ut ) == expected );
  }
  SECTION( "expert modified colonists" ) {
    // veteran_dragoon.
    ut       = UnitType::create( UT::veteran_dragoon );
    expected = UnitType::create( UT::dragoon );
    REQUIRE( f( ut ) == expected );
    // continental_cavalry.
    ut       = UnitType::create( UT::continental_cavalry );
    expected = UnitType::create( UT::dragoon );
    REQUIRE( f( ut ) == expected );
    // continental_army.
    ut       = UnitType::create( UT::continental_army );
    expected = UnitType::create( UT::soldier );
    REQUIRE( f( ut ) == expected );
    // seasoned_scout.
    ut       = UnitType::create( UT::seasoned_scout );
    expected = UnitType::create( UT::scout );
    REQUIRE( f( ut ) == expected );
    // hardy_pioneer.
    ut       = UnitType::create( UT::hardy_pioneer );
    expected = UnitType::create( UT::pioneer );
    REQUIRE( f( ut ) == expected );
    // veteran_soldier.
    ut       = UnitType::create( UT::veteran_soldier );
    expected = UnitType::create( UT::soldier );
    REQUIRE( f( ut ) == expected );
    // jesuit_missionary..
    ut       = UnitType::create( UT::jesuit_missionary );
    expected = UnitType::create( UT::missionary );
    REQUIRE( f( ut ) == expected );
  }
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

TEST_CASE( "[utype] unit human status" ) {
  bool     expected;
  UnitType ut;

  ut       = UnitType::create( e_unit_type::free_colonist );
  expected = true;
  REQUIRE( is_unit_human( ut ) == expected );

  ut       = UnitType::create( e_unit_type::dragoon );
  expected = true;
  REQUIRE( is_unit_human( ut ) == expected );

  ut       = UnitType::create( e_unit_type::veteran_colonist );
  expected = true;
  REQUIRE( is_unit_human( ut ) == expected );

  ut       = UnitType::create( e_unit_type::hardy_pioneer );
  expected = true;
  REQUIRE( is_unit_human( ut ) == expected );

  ut       = UnitType::create( e_unit_type::wagon_train );
  expected = false;
  REQUIRE( is_unit_human( ut ) == expected );

  ut       = UnitType::create( e_unit_type::small_treasure );
  expected = false;
  REQUIRE( is_unit_human( ut ) == expected );
}

} // namespace
} // namespace rn
