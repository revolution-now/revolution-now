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

namespace rn {
namespace {

using namespace std;

using Catch::Contains;

TEST_CASE( "[utype] unit type attributes deserialization" ) {
  SECTION( "expert_cotton_planter" ) {
    UnitTypeAttributes const& desc =
        unit_desc( e_unit_type::expert_cotton_planter );
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
        unit_desc( e_unit_type::petty_criminal );
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
        unit_desc( e_unit_type::free_colonist );
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
        unit_desc( e_unit_type::veteran_dragoon );
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
        unit_desc( e_unit_type::scout );
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

TEST_CASE( "[utype] lua bindings" ) {
  lua::state& st     = lua_global_state();
  auto        script = R"(
    local x = 5+6
  )";
  REQUIRE( st.script.run_safe( script ).valid() );

  script = R"(
    local x =
  )";

  auto xp = st.script.run_safe( script );
  REQUIRE( !xp.valid() );
  REQUIRE_THAT( xp.error(), Contains( "unexpected symbol" ) );

  script = R"(
    return 5+8.5
  )";
  REQUIRE( st.script.run_safe<double>( script ) == 13.5 );
}

} // namespace
} // namespace rn
