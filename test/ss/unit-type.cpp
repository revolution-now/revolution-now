/****************************************************************
**unit-type.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-09-27.
*
* Description: Unit tests for the src/unit-type.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/ss/unit-type.hpp"

// Testing
#include "test/fake/world.hpp"

// Revolution Now
#include "src/lua.hpp"

// config
#include "src/config/tile-enum.rds.hpp"
#include "src/config/unit-type.hpp"

// ss
#include "src/ss/player.rds.hpp"

// luapp
#include "src/luapp/state.hpp"

// refl
#include "src/refl/to-str.hpp"

// base
#include "src/base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

using Catch::Contains;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() { add_default_player(); }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[unit-type] inventory_traits" ) {
  auto& traits = config_unit_type.composition
                     .inventory_traits[e_unit_inventory::tools];
  REQUIRE( traits.commodity == e_commodity::tools );
  REQUIRE( traits.min_quantity == 20 );
  REQUIRE( traits.max_quantity == 100 );
  REQUIRE( traits.multiple == 20 );
}

TEST_CASE( "[unit-type] inventory_to_modifier" ) {
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

TEST_CASE( "[unit-type] commodity_to_inventory" ) {
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

TEST_CASE( "[unit-type] inventory_to_commodity" ) {
  auto f = inventory_to_commodity;
  REQUIRE( f( e_unit_inventory::gold ) == nothing );
  REQUIRE( f( e_unit_inventory::tools ) == e_commodity::tools );
}

TEST_CASE( "[unit-type] unit type attributes deserialization" ) {
  SECTION( "expert_cotton_planter" ) {
    UnitTypeAttributes const& desc =
        unit_attr( e_unit_type::expert_cotton_planter );
    REQUIRE( desc.name == "Expert Cotton Planter" );
    REQUIRE( desc.name_plural == "Expert Cotton Planters" );
    REQUIRE( desc.tile == e_tile::expert_cotton_planter );
    REQUIRE( desc.nat_icon_front == false );
    REQUIRE( desc.nat_icon_position == e_direction::sw );
    REQUIRE( desc.ship == false );
    REQUIRE( desc.human == e_unit_human::yes );
    REQUIRE( desc.can_found == e_unit_can_found_colony::yes );
    REQUIRE( desc.visibility == 1 );
    REQUIRE( desc.base_movement_points == 1 );
    REQUIRE( desc.can_attack == false );
    REQUIRE( desc.combat == 1 );
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
    REQUIRE( can_attack( desc.type ) == false );
    REQUIRE( is_military_unit( desc.type ) == false );
  }
  SECTION( "petty_criminal" ) {
    UnitTypeAttributes const& desc =
        unit_attr( e_unit_type::petty_criminal );
    REQUIRE( desc.name == "Petty Criminal" );
    REQUIRE( desc.name_plural == "Petty Criminals" );
    REQUIRE( desc.tile == e_tile::petty_criminal );
    REQUIRE( desc.nat_icon_front == false );
    REQUIRE( desc.nat_icon_position == e_direction::sw );
    REQUIRE( desc.ship == false );
    REQUIRE( desc.human == e_unit_human::yes );
    REQUIRE( desc.can_found == e_unit_can_found_colony::yes );
    REQUIRE( desc.visibility == 1 );
    REQUIRE( desc.base_movement_points == 1 );
    REQUIRE( desc.can_attack == false );
    REQUIRE( desc.combat == 1 );
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
    REQUIRE( can_attack( desc.type ) == false );
    REQUIRE( is_military_unit( desc.type ) == false );
  }
  SECTION( "free_colonist" ) {
    UnitTypeAttributes const& desc =
        unit_attr( e_unit_type::free_colonist );
    REQUIRE( desc.name == "Free Colonist" );
    REQUIRE( desc.name_plural == "Free Colonists" );
    REQUIRE( desc.tile == e_tile::free_colonist );
    REQUIRE( desc.nat_icon_front == false );
    REQUIRE( desc.nat_icon_position == e_direction::sw );
    REQUIRE( desc.ship == false );
    REQUIRE( desc.human == e_unit_human::yes );
    REQUIRE( desc.can_found == e_unit_can_found_colony::yes );
    REQUIRE( desc.visibility == 1 );
    REQUIRE( desc.base_movement_points == 1 );
    REQUIRE( desc.can_attack == false );
    REQUIRE( desc.combat == 1 );
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
    REQUIRE( can_attack( desc.type ) == false );
    REQUIRE( is_military_unit( desc.type ) == false );
  }
  SECTION( "veteran_dragoon" ) {
    UnitTypeAttributes const& desc =
        unit_attr( e_unit_type::veteran_dragoon );
    REQUIRE( desc.name == "Veteran Dragoon" );
    REQUIRE( desc.name_plural == "Veteran Dragoons" );
    REQUIRE( desc.tile == e_tile::veteran_dragoon );
    REQUIRE( desc.nat_icon_front == false );
    REQUIRE( desc.nat_icon_position == e_direction::sw );
    REQUIRE( desc.ship == false );
    REQUIRE( desc.human == e_unit_human::from_base );
    REQUIRE( desc.can_found ==
             e_unit_can_found_colony::from_base );
    REQUIRE( desc.visibility == 1 );
    REQUIRE( desc.base_movement_points == 4 );
    REQUIRE( desc.can_attack == true );
    REQUIRE( desc.combat == 4 );
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
    REQUIRE( can_attack( desc.type ) == true );
    REQUIRE( is_military_unit( desc.type ) == true );
  }
  SECTION( "scout" ) {
    UnitTypeAttributes const& desc =
        unit_attr( e_unit_type::scout );
    REQUIRE( desc.name == "Scout" );
    REQUIRE( desc.name_plural == "Scouts" );
    REQUIRE( desc.tile == e_tile::scout );
    REQUIRE( desc.nat_icon_front == false );
    REQUIRE( desc.nat_icon_position == e_direction::sw );
    REQUIRE( desc.ship == false );
    REQUIRE( desc.human == e_unit_human::from_base );
    REQUIRE( desc.can_found ==
             e_unit_can_found_colony::from_base );
    REQUIRE( desc.visibility == 2 );
    REQUIRE( desc.base_movement_points == 4 );
    REQUIRE( desc.can_attack == true );
    REQUIRE( desc.combat == 1 );
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
    REQUIRE( can_attack( desc.type ) == true );
    REQUIRE( is_military_unit( desc.type ) == true );
  }
  SECTION( "pioneer" ) {
    UnitTypeAttributes const& desc =
        unit_attr( e_unit_type::pioneer );
    REQUIRE( desc.name == "Pioneer" );
    REQUIRE( desc.name_plural == "Pioneers" );
    REQUIRE( desc.tile == e_tile::pioneer );
    REQUIRE( desc.nat_icon_front == false );
    REQUIRE( desc.nat_icon_position == e_direction::sw );
    REQUIRE( desc.ship == false );
    REQUIRE( desc.human == e_unit_human::from_base );
    REQUIRE( desc.can_found ==
             e_unit_can_found_colony::from_base );
    REQUIRE( desc.visibility == 1 );
    REQUIRE( desc.base_movement_points == 1 );
    REQUIRE( desc.can_attack == false );
    REQUIRE( desc.combat == 1 );
    REQUIRE( desc.cargo_slots == 0 );
    REQUIRE( desc.cargo_slots_occupies == 1 );
    REQUIRE( desc.on_death ==
             UnitDeathAction_t{ UnitDeathAction::destroy{} } );
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
    REQUIRE( can_attack( desc.type ) == false );
    REQUIRE( is_military_unit( desc.type ) == false );
  }
  SECTION( "missionary" ) {
    UnitTypeAttributes const& desc =
        unit_attr( e_unit_type::missionary );
    REQUIRE( desc.name == "Missionary" );
    REQUIRE( desc.name_plural == "Missionaries" );
    REQUIRE( desc.tile == e_tile::missionary );
    REQUIRE( desc.nat_icon_front == false );
    REQUIRE( desc.nat_icon_position == e_direction::sw );
    REQUIRE( desc.ship == false );
    REQUIRE( desc.human == e_unit_human::from_base );
    REQUIRE( desc.can_found ==
             e_unit_can_found_colony::from_base );
    REQUIRE( desc.visibility == 1 );
    REQUIRE( desc.base_movement_points == 2 );
    REQUIRE( desc.can_attack == false );
    REQUIRE( desc.combat == 1 );
    REQUIRE( desc.cargo_slots == 0 );
    REQUIRE( desc.cargo_slots_occupies == 1 );
    REQUIRE( desc.on_death ==
             UnitDeathAction_t{ UnitDeathAction::destroy{} } );
    REQUIRE( desc.canonical_base == e_unit_type::free_colonist );
    REQUIRE( desc.expertise == nothing );
    REQUIRE( desc.promotion ==
             UnitPromotion_t{ UnitPromotion::expertise{
                 .kind = e_unit_activity::missioning } } );
    unordered_map<e_unit_type,
                  unordered_set<e_unit_type_modifier>>
        expected_modifiers{};
    REQUIRE( desc.modifiers == expected_modifiers );
    REQUIRE( desc.inventory_types ==
             unordered_set<e_unit_inventory>{} );
    // Derived fields.
    REQUIRE( desc.type == e_unit_type::missionary );
    REQUIRE( desc.is_derived == true );
    REQUIRE( can_attack( desc.type ) == false );
    REQUIRE( is_military_unit( desc.type ) == false );
  }
  SECTION( "treasure" ) {
    UnitTypeAttributes const& desc =
        unit_attr( e_unit_type::treasure );
    REQUIRE( desc.name == "Treasure" );
    REQUIRE( desc.name_plural == "Treasures" );
    REQUIRE( desc.tile == e_tile::treasure );
    REQUIRE( desc.nat_icon_front == false );
    REQUIRE( desc.nat_icon_position == e_direction::n );
    REQUIRE( desc.ship == false );
    REQUIRE( desc.human == e_unit_human::no );
    REQUIRE( desc.can_found == e_unit_can_found_colony::no );
    REQUIRE( desc.visibility == 1 );
    REQUIRE( desc.base_movement_points == 1 );
    REQUIRE( desc.can_attack == false );
    REQUIRE( desc.combat == 0 );
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
    REQUIRE( desc.type == e_unit_type::treasure );
    REQUIRE( desc.is_derived == false );
    REQUIRE( can_attack( desc.type ) == false );
    REQUIRE( is_military_unit( desc.type ) == false );
  }
}

// This test case contains a random selection of cases.
TEST_CASE( "[unit-type] UnitType creation" ) {
  using UT = e_unit_type;
  auto f   = []( UT type, UT base_type ) {
    return UnitType::create( type, base_type );
  };
  // Same types (base);
  REQUIRE(
      f( UT::free_colonist, UT::free_colonist ).has_value() );
  REQUIRE(
      f( UT::native_convert, UT::native_convert ).has_value() );
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
  REQUIRE(
      !f( UT::native_convert, UT::free_colonist ).has_value() );
  REQUIRE(
      !f( UT::free_colonist, UT::native_convert ).has_value() );
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
TEST_CASE( "[unit-type] unit_type_modifiers" ) {
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

TEST_CASE( "[unit-type] add_unit_type_modifiers" ) {
  auto f = []( UnitType ut,
               unordered_set<e_unit_type_modifier> const&
                   modifiers ) {
    return add_unit_type_modifiers( ut, modifiers );
  };
  using UT  = e_unit_type;
  using Mod = e_unit_type_modifier;

  // Empty modifiers.
  REQUIRE( f( UT::petty_criminal, {} ) == UT::petty_criminal );
  REQUIRE( f( UT::native_convert, {} ) == UT::native_convert );
  REQUIRE( f( UT::dragoon, {} ) == UT::dragoon );
  // Invalid.
  REQUIRE( f( UT::veteran_soldier, { Mod::muskets } ) ==
           nothing );
  REQUIRE( f( UT::native_convert, { Mod::muskets } ) ==
           nothing );
  REQUIRE( f( UnitType::create( UT::veteran_soldier,
                                UT::veteran_colonist )
                  .value(),
              { Mod::tools } ) == nothing );
  REQUIRE( f( UT::veteran_soldier, { Mod::strength } ) ==
           nothing );
  REQUIRE( f( UT::veteran_dragoon, { Mod::horses } ) ==
           nothing );
  REQUIRE( f( UT::veteran_soldier,
              { Mod::horses, Mod::independence, Mod::tools } ) ==
           nothing );
  REQUIRE( f( UT::artillery, { Mod::strength } ) == nothing );
  // Valid.
  REQUIRE(
      f( UnitType::create( UT::soldier, UT::indentured_servant )
             .value(),
         { Mod::horses } ) ==
      UnitType::create( UT::dragoon, UT::indentured_servant ) );
  REQUIRE( f( UT::veteran_soldier,
              { Mod::horses, Mod::independence } ) ==
           UT::continental_cavalry );
  REQUIRE( f( UT::damaged_artillery, { Mod::strength } ) ==
           UT::artillery );
}

TEST_CASE( "[unit-type] rm_unit_type_modifiers" ) {
  auto f = []( UnitType ut,
               unordered_set<e_unit_type_modifier> const&
                   modifiers ) {
    return rm_unit_type_modifiers( ut, modifiers );
  };
  using UT  = e_unit_type;
  using Mod = e_unit_type_modifier;

  // Empty modifiers.
  REQUIRE( f( UT::petty_criminal, {} ) == UT::petty_criminal );
  REQUIRE( f( UT::dragoon, {} ) == UT::dragoon );
  // Invalid.
  REQUIRE( f( UT::scout, { Mod::muskets } ) == nothing );
  REQUIRE( f( UT::native_convert, { Mod::horses } ) == nothing );
  REQUIRE( f( UnitType::create( UT::veteran_soldier,
                                UT::veteran_colonist )
                  .value(),
              { Mod::tools } ) == nothing );
  REQUIRE( f( UT::veteran_soldier, { Mod::strength } ) ==
           nothing );
  REQUIRE( f( UT::veteran_colonist, { Mod::horses } ) ==
           nothing );
  REQUIRE( f( UT::veteran_soldier,
              { Mod::muskets, Mod::tools } ) == nothing );
  REQUIRE( f( UT::damaged_artillery, { Mod::strength } ) ==
           nothing );
  REQUIRE( f( UT::continental_cavalry, { Mod::muskets } ) ==
           nothing );
  // Valid.
  REQUIRE(
      f( UnitType::create( UT::dragoon, UT::indentured_servant )
             .value(),
         { Mod::horses } ) ==
      UnitType::create( UT::soldier, UT::indentured_servant )
          .value() );
  REQUIRE( f( UT::continental_cavalry,
              { Mod::horses, Mod::independence } ) ==
           UT::veteran_soldier );
  REQUIRE( f( UT::artillery, { Mod::strength } ) ==
           UT::damaged_artillery );
  REQUIRE( f( UT::dragoon, { Mod::muskets } ) == UT::scout );
}

TEST_CASE( "[unit-type] lua bindings" ) {
  lua::state st;
  st.lib.open_all();
  run_lua_startup_routines( st );

  auto script = R"(
    local ut
    -- free_colonist
    ut = unit_type.UnitType.create( "free_colonist" )
    assert( ut )
    assert( ut:type() == "free_colonist" )
    assert( ut:base_type() == "free_colonist" )
    -- dragoon
    ut = unit_type.UnitType.create( "dragoon" )
    assert( ut )
    assert( ut:type() == "dragoon" )
    assert( ut:base_type() == "free_colonist" )
    -- veteran_soldier
    ut = unit_type.UnitType.create( "veteran_soldier" )
    assert( ut )
    assert( ut:type() == "veteran_soldier" )
    assert( ut:base_type() == "veteran_colonist" )
    -- pioneer
    ut = unit_type.UnitType.create_with_base(
        "pioneer", "expert_farmer" )
    assert( ut )
    assert( ut:type() == "pioneer" )
    assert( ut:base_type() == "expert_farmer" )
  )";
  REQUIRE( st.script.run_safe( script ) == valid );

  script  = R"(
    local ut
    unit_type.UnitType.create_with_base(
        "hardy_pioneer", "expert_farmer" )
  )";
  auto xp = st.script.run_safe( script );
  REQUIRE( !xp.valid() );
  REQUIRE_THAT(
      xp.error(),
      Contains(
          "failed to create UnitType with type=hardy_pioneer "
          "and base_type=expert_farmer." ) );
}

TEST_CASE( "[unit-type] unit human status" ) {
  bool     expected;
  UnitType ut;

  ut       = e_unit_type::free_colonist;
  expected = true;
  REQUIRE( is_unit_human( ut ) == expected );

  ut       = e_unit_type::native_convert;
  expected = true;
  REQUIRE( is_unit_human( ut ) == expected );

  ut       = e_unit_type::dragoon;
  expected = true;
  REQUIRE( is_unit_human( ut ) == expected );

  ut       = e_unit_type::veteran_colonist;
  expected = true;
  REQUIRE( is_unit_human( ut ) == expected );

  ut       = e_unit_type::hardy_pioneer;
  expected = true;
  REQUIRE( is_unit_human( ut ) == expected );

  ut       = e_unit_type::wagon_train;
  expected = false;
  REQUIRE( is_unit_human( ut ) == expected );

  ut       = e_unit_type::treasure;
  expected = false;
  REQUIRE( is_unit_human( ut ) == expected );
}

TEST_CASE( "[unit-type] unit can_found status" ) {
  bool     expected;
  UnitType ut;

  ut       = e_unit_type::free_colonist;
  expected = true;
  REQUIRE( can_unit_found( ut ) == expected );

  ut       = e_unit_type::native_convert;
  expected = false;
  REQUIRE( can_unit_found( ut ) == expected );

  ut       = e_unit_type::dragoon;
  expected = true;
  REQUIRE( can_unit_found( ut ) == expected );

  ut       = e_unit_type::veteran_colonist;
  expected = true;
  REQUIRE( can_unit_found( ut ) == expected );

  ut       = e_unit_type::hardy_pioneer;
  expected = true;
  REQUIRE( can_unit_found( ut ) == expected );

  ut       = e_unit_type::wagon_train;
  expected = false;
  REQUIRE( can_unit_found( ut ) == expected );

  ut       = e_unit_type::treasure;
  expected = false;
  REQUIRE( can_unit_found( ut ) == expected );
}

TEST_CASE( "[unit-type] movement_points" ) {
  World   W;
  Player& player = W.default_player();

  REQUIRE(
      movement_points( player, e_unit_type::free_colonist ) ==
      MvPoints( 1 ) );
  REQUIRE( movement_points( player, e_unit_type::dragoon ) ==
           MvPoints( 4 ) );
  REQUIRE( movement_points( player, e_unit_type::privateer ) ==
           MvPoints( 8 ) );
  REQUIRE( movement_points( player, e_unit_type::caravel ) ==
           MvPoints( 4 ) );

  player.fathers.has[e_founding_father::ferdinand_magellan] =
      true;

  REQUIRE(
      movement_points( player, e_unit_type::free_colonist ) ==
      MvPoints( 1 ) );
  REQUIRE( movement_points( player, e_unit_type::dragoon ) ==
           MvPoints( 4 ) );
  REQUIRE( movement_points( player, e_unit_type::privateer ) ==
           MvPoints( 9 ) );
  REQUIRE( movement_points( player, e_unit_type::caravel ) ==
           MvPoints( 5 ) );
}

TEST_CASE( "[unit-type] scout_type" ) {
  REQUIRE( scout_type( e_unit_type::free_colonist ) == nothing );
  REQUIRE( scout_type( e_unit_type::scout ) ==
           e_scout_type::non_seasoned );
  REQUIRE( scout_type( e_unit_type::seasoned_scout ) ==
           e_scout_type::seasoned );
  REQUIRE( scout_type( e_unit_type::dragoon ) == nothing );
}

TEST_CASE( "[unit-type] missionary_type" ) {
  UnitType                 in;
  maybe<e_missionary_type> expected = {};

  auto f = [&] { return missionary_type( in ); };

  // petty criminal.
  in = UnitType::create( e_unit_type::missionary,
                         e_unit_type::petty_criminal )
           .value();
  expected = e_missionary_type::criminal;
  REQUIRE( f() == expected );

  // indentured servant.
  in = UnitType::create( e_unit_type::missionary,
                         e_unit_type::indentured_servant )
           .value();
  expected = e_missionary_type::indentured;
  REQUIRE( f() == expected );

  // free colonist.
  in = UnitType::create( e_unit_type::missionary,
                         e_unit_type::free_colonist )
           .value();
  expected = e_missionary_type::normal;
  REQUIRE( f() == expected );

  // expert_farmer.
  in = UnitType::create( e_unit_type::missionary,
                         e_unit_type::expert_farmer )
           .value();
  expected = e_missionary_type::normal;
  REQUIRE( f() == expected );

  // jesuit_missionary.
  in       = e_unit_type::jesuit_missionary;
  expected = e_missionary_type::jesuit;
  REQUIRE( f() == expected );

  // free_colonist non-missionary.
  in       = e_unit_type::free_colonist;
  expected = nothing;
  REQUIRE( f() == expected );

  // petty_criminal non-missionary.
  in       = e_unit_type::petty_criminal;
  expected = nothing;
  REQUIRE( f() == expected );

  // jesuit_colonist non-missionary.
  in       = e_unit_type::jesuit_colonist;
  expected = nothing;
  REQUIRE( f() == expected );

  // artillery.
  in       = e_unit_type::artillery;
  expected = nothing;
  REQUIRE( f() == expected );
}

} // namespace
} // namespace rn
