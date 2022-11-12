/****************************************************************
**alarm.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-11-07.
*
* Description: Unit tests for the src/alarm.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/alarm.hpp"

// Testing
#include "test/fake/world.hpp"

// ss
#include "ss/dwelling.rds.hpp"
#include "ss/natives.hpp"
#include "ss/ref.hpp"

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
    add_player( e_nation::english );
    set_default_player( e_nation::english );
    create_default_map();
  }

  void create_default_map() {
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
    // clang-format off
    vector<MapSquare> tiles{
      _, L, _,
      L, L, L,
      _, L, L,
    };
    // clang-format on
    build_map( std::move( tiles ), 3 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[alarm] effective_dwelling_alarm" ) {
  World     W;
  Dwelling& dwelling =
      W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::arawak );
  Tribe& tribe = W.natives().tribe_for( e_tribe::arawak );
  e_nation const nation = e_nation::english;

  auto f = [&] {
    return effective_dwelling_alarm( W.ss(), dwelling, nation );
  };

  // No contact.
  dwelling.relationship[nation].dwelling_only_alarm = 30;
  REQUIRE( f() == 0 );

  // Has contact with no tribal alarm.
  tribe.relationship[nation].emplace();
  REQUIRE( f() == 30 );

  // Has contact with tribal alarm.
  tribe.relationship[nation]->tribal_alarm = 40;
  REQUIRE( f() == 58 );

  // Has contact with tribal alarm but no dwelling alarm.
  tribe.relationship[nation]->tribal_alarm          = 40;
  dwelling.relationship[nation].dwelling_only_alarm = 0;
  REQUIRE( f() == 40 );

  // Rounding.
  tribe.relationship[nation]->tribal_alarm          = 33;
  dwelling.relationship[nation].dwelling_only_alarm = 34;
  REQUIRE( f() == 56 );

  // Both zero.
  tribe.relationship[nation]->tribal_alarm          = 0;
  dwelling.relationship[nation].dwelling_only_alarm = 0;
  REQUIRE( f() == 0 );

  // One max.
  tribe.relationship[nation]->tribal_alarm          = 99;
  dwelling.relationship[nation].dwelling_only_alarm = 0;
  REQUIRE( f() == 99 );

  // Both max.
  tribe.relationship[nation]->tribal_alarm          = 99;
  dwelling.relationship[nation].dwelling_only_alarm = 99;
  REQUIRE( f() == 99 );
}

TEST_CASE( "[alarm] reaction_for_dwelling" ) {
  World     W;
  Dwelling& dwelling =
      W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::arawak );
  Tribe& tribe = W.natives().tribe_for( e_tribe::arawak );
  e_nation const nation = e_nation::english;

  auto f = [&] {
    return reaction_for_dwelling( W.ss(), W.default_player(),
                                  tribe, dwelling );
  };

  // No contact no alarm.
  REQUIRE( f() == e_enter_dwelling_reaction::wave_happily );

  // No contact with dwelling alarm.
  dwelling.relationship[nation].dwelling_only_alarm = 30;
  REQUIRE( f() == e_enter_dwelling_reaction::wave_happily );

  // Has contact with no tribal alarm.
  tribe.relationship[nation].emplace();
  REQUIRE( f() ==
           e_enter_dwelling_reaction::wave_happily_with_scalps );

  // At war.
  tribe.relationship[nation]->at_war = true;
  REQUIRE( f() ==
           e_enter_dwelling_reaction::scalps_and_war_drums );
  tribe.relationship[nation]->at_war = false;

  // Has contact with tribal alarm.
  tribe.relationship[nation]->tribal_alarm = 40;
  REQUIRE( f() == e_enter_dwelling_reaction::frowning_archers );

  // Has contact with tribal alarm but no dwelling alarm.
  tribe.relationship[nation]->tribal_alarm          = 40;
  dwelling.relationship[nation].dwelling_only_alarm = 0;
  REQUIRE( f() == e_enter_dwelling_reaction::frowning_archers );

  // Rounding.
  tribe.relationship[nation]->tribal_alarm          = 33;
  dwelling.relationship[nation].dwelling_only_alarm = 34;
  REQUIRE( f() == e_enter_dwelling_reaction::frowning_archers );

  // Both zero.
  tribe.relationship[nation]->tribal_alarm          = 0;
  dwelling.relationship[nation].dwelling_only_alarm = 0;
  REQUIRE( f() == e_enter_dwelling_reaction::wave_happily );

  // One max.
  tribe.relationship[nation]->tribal_alarm          = 99;
  dwelling.relationship[nation].dwelling_only_alarm = 0;
  REQUIRE( f() ==
           e_enter_dwelling_reaction::scalps_and_war_drums );

  // Both max.
  tribe.relationship[nation]->tribal_alarm          = 99;
  dwelling.relationship[nation].dwelling_only_alarm = 99;
  REQUIRE( f() ==
           e_enter_dwelling_reaction::scalps_and_war_drums );

  // Both max.
  tribe.relationship[nation]->tribal_alarm          = 50;
  dwelling.relationship[nation].dwelling_only_alarm = 50;
  REQUIRE( f() == e_enter_dwelling_reaction::wary_warriors );
}

} // namespace
} // namespace rn
