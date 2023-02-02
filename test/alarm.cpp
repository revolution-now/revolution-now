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
#include "ss/player.rds.hpp"
#include "ss/ref.hpp"
#include "ss/settings.rds.hpp"
#include "ss/terrain.hpp"

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
    add_player( e_nation::english );
    set_default_player( e_nation::english );
    create_default_map();
  }

  void create_default_map() {
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
    // clang-format off
    vector<MapSquare> tiles{
      _, L, _, L, L, L,
      L, L, L, L, L, L,
      _, L, L, L, L, L,
      _, L, L, L, L, L,
      _, L, L, L, L, L,
      _, L, L, L, L, L,
    };
    // clang-format on
    build_map( std::move( tiles ), 6 );
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

TEST_CASE( "[alarm] increase_tribal_alarm_from_land_grab" ) {
  World           W;
  Dwelling const& dwelling =
      W.add_dwelling( { .x = 2, .y = 2 }, e_tribe::inca );
  Tribe& tribe = W.natives().tribe_for( e_tribe::inca );
  TribeRelationship& relationship =
      tribe.relationship[W.default_nation()].emplace();
  Coord tile;

  auto f = [&] {
    increase_tribal_alarm_from_land_grab(
        W.ss(), W.default_player(), relationship, tile );
  };

  REQUIRE( relationship.tribal_alarm == 0 );

  int so_far = 0, expected = 0;

  SECTION( "governor" ) {
    W.settings().difficulty = e_difficulty::governor;

    tile = { .x = 2, .y = 2 };
    W.natives().mark_land_owned( dwelling.id, tile );
    f();
    expected = 11;
    REQUIRE( relationship.tribal_alarm == so_far + expected );
    so_far += expected;

    tile = { .x = 1, .y = 2 };
    W.natives().mark_land_owned( dwelling.id, tile );
    f();
    expected = 11;
    REQUIRE( relationship.tribal_alarm == so_far + expected );
    so_far += expected;

    W.terrain().mutable_square_at( tile ).ground_resource =
        e_natural_resource::tobacco;
    f();
    expected = 22;
    REQUIRE( relationship.tribal_alarm == so_far + expected );
    so_far += expected;

    tile = { .x = 1, .y = 1 };
    W.natives().mark_land_owned( dwelling.id, tile );
    f();
    expected = 11;
    REQUIRE( relationship.tribal_alarm == so_far + expected );
    so_far += expected;

    tile = { .x = 1, .y = 0 };
    W.natives().mark_land_owned( dwelling.id, tile );
    f();
    expected = 6; // lround( floor( 11 * .6 ) );
    REQUIRE( relationship.tribal_alarm == so_far + expected );
    so_far += expected;

    W.terrain().mutable_square_at( tile ).ground_resource =
        e_natural_resource::tobacco;
    f();
    expected = 13; // lround( floor( 11*.6 ) ) * 2.0.
    REQUIRE( relationship.tribal_alarm == so_far + expected );
    so_far += expected;

    tile = { .x = 5, .y = 5 };
    W.natives().mark_land_owned( dwelling.id, tile );
    f();
    expected = 3; // lround( floor( 11*.6*.6 ) );
    REQUIRE( relationship.tribal_alarm == so_far + expected );
    so_far += expected;

    W.terrain().mutable_square_at( tile ).ground_resource =
        e_natural_resource::tobacco;
    f();
    expected = 7; // lround( floor( 11*.6*.6 ) ) * 2.0.
    REQUIRE( relationship.tribal_alarm == so_far + expected );
    so_far += expected;

    // Ceiling.
    tile = { .x = 1, .y = 2 };
    f();
    expected = 22;
    REQUIRE( relationship.tribal_alarm == 99 );
    so_far += expected;
  }

  SECTION( "discoverer" ) {
    W.settings().difficulty = e_difficulty::discoverer;

    tile = { .x = 2, .y = 2 };
    W.natives().mark_land_owned( dwelling.id, tile );
    f();
    expected = 7;
    REQUIRE( relationship.tribal_alarm == so_far + expected );
    so_far += expected;

    tile = { .x = 1, .y = 2 };
    W.natives().mark_land_owned( dwelling.id, tile );
    f();
    expected = 7;
    REQUIRE( relationship.tribal_alarm == so_far + expected );
    so_far += expected;

    W.terrain().mutable_square_at( tile ).ground_resource =
        e_natural_resource::tobacco;
    f();
    expected = 14;
    REQUIRE( relationship.tribal_alarm == so_far + expected );
    so_far += expected;

    tile = { .x = 1, .y = 1 };
    W.natives().mark_land_owned( dwelling.id, tile );
    f();
    expected = 7;
    REQUIRE( relationship.tribal_alarm == so_far + expected );
    so_far += expected;

    tile = { .x = 1, .y = 0 };
    W.natives().mark_land_owned( dwelling.id, tile );
    f();
    expected = 4; // lround( floor( 7 * .6 ) );
    REQUIRE( relationship.tribal_alarm == so_far + expected );
    so_far += expected;

    W.terrain().mutable_square_at( tile ).ground_resource =
        e_natural_resource::tobacco;
    f();
    expected = 8; // lround( floor( 7*.6 ) ) * 2.0.
    REQUIRE( relationship.tribal_alarm == so_far + expected );
    so_far += expected;

    tile = { .x = 5, .y = 5 };
    W.natives().mark_land_owned( dwelling.id, tile );
    f();
    expected = 2; // lround( floor( 7*.6*.6 ) );
    REQUIRE( relationship.tribal_alarm == so_far + expected );
    so_far += expected;

    W.terrain().mutable_square_at( tile ).ground_resource =
        e_natural_resource::tobacco;
    f();
    expected = 5; // lround( floor( 7*.6*.6 ) ) * 2.0.
    REQUIRE( relationship.tribal_alarm == so_far + expected );
    so_far += expected;

    // Ceiling.
    tile = { .x = 1, .y = 2 };
    f();
    f();
    f();
    f();
    expected = 14;
    REQUIRE( relationship.tribal_alarm == 99 );
    so_far += expected;
  }

  SECTION( "discoverer w/ pocahontas" ) {
    W.settings().difficulty = e_difficulty::discoverer;
    W.default_player()
        .fathers.has[e_founding_father::pocahontas] = true;

    tile = { .x = 2, .y = 2 };
    W.natives().mark_land_owned( dwelling.id, tile );
    f();
    expected = 3;
    REQUIRE( relationship.tribal_alarm == so_far + expected );
    so_far += expected;

    tile = { .x = 1, .y = 2 };
    W.natives().mark_land_owned( dwelling.id, tile );
    f();
    expected = 3;
    REQUIRE( relationship.tribal_alarm == so_far + expected );
    so_far += expected;

    W.terrain().mutable_square_at( tile ).ground_resource =
        e_natural_resource::tobacco;
    f();
    expected = 7;
    REQUIRE( relationship.tribal_alarm == so_far + expected );
    so_far += expected;
  }
}

TEST_CASE(
    "[alarm] increase_tribal_alarm_from_attacking_brave" ) {
  World W;

  Tribe&             tribe = W.add_tribe( e_tribe::inca );
  TribeRelationship& relationship =
      tribe.relationship[W.default_nation()].emplace();
  Dwelling& dwelling =
      W.add_dwelling( { .x = 1, .y = 1 }, tribe.type );

  auto f = [&] {
    increase_tribal_alarm_from_attacking_brave(
        W.default_player(), dwelling, relationship );
  };

  SECTION( "non-capital" ) {
    REQUIRE( relationship.tribal_alarm == 0 );
    f();
    REQUIRE( relationship.tribal_alarm == 10 );
    f();
    REQUIRE( relationship.tribal_alarm == 20 );

    relationship.tribal_alarm = 95;
    f();
    REQUIRE( relationship.tribal_alarm == 99 );
  }

  SECTION( "capital" ) {
    dwelling.is_capital = true;
    REQUIRE( relationship.tribal_alarm == 0 );
    f();
    REQUIRE( relationship.tribal_alarm == 20 );
    f();
    REQUIRE( relationship.tribal_alarm == 40 );

    relationship.tribal_alarm = 95;
    f();
    REQUIRE( relationship.tribal_alarm == 99 );
  }

  SECTION( "pocahontas" ) {
    W.default_player()
        .fathers.has[e_founding_father::pocahontas] = true;
    REQUIRE( relationship.tribal_alarm == 0 );
    f();
    REQUIRE( relationship.tribal_alarm == 5 );
    f();
    REQUIRE( relationship.tribal_alarm == 10 );

    relationship.tribal_alarm = 95;
    f();
    REQUIRE( relationship.tribal_alarm == 99 );
  }
}

TEST_CASE(
    "[alarm] increase_tribal_alarm_from_attacking_dwelling" ) {
  World W;

  Tribe&             tribe = W.add_tribe( e_tribe::inca );
  TribeRelationship& relationship =
      tribe.relationship[W.default_nation()].emplace();
  Dwelling& dwelling =
      W.add_dwelling( { .x = 1, .y = 1 }, tribe.type );

  auto f = [&] {
    increase_tribal_alarm_from_attacking_dwelling(
        W.default_player(), dwelling, relationship );
  };

  SECTION( "non-capital" ) {
    REQUIRE( relationship.tribal_alarm == 0 );
    f();
    REQUIRE( relationship.tribal_alarm == 10 );
    f();
    REQUIRE( relationship.tribal_alarm == 20 );

    relationship.tribal_alarm = 95;
    f();
    REQUIRE( relationship.tribal_alarm == 99 );
  }

  SECTION( "capital" ) {
    dwelling.is_capital = true;
    REQUIRE( relationship.tribal_alarm == 0 );
    f();
    REQUIRE( relationship.tribal_alarm == 20 );
    f();
    REQUIRE( relationship.tribal_alarm == 40 );

    relationship.tribal_alarm = 95;
    f();
    REQUIRE( relationship.tribal_alarm == 99 );
  }

  SECTION( "pocahontas" ) {
    W.default_player()
        .fathers.has[e_founding_father::pocahontas] = true;
    REQUIRE( relationship.tribal_alarm == 0 );
    f();
    REQUIRE( relationship.tribal_alarm == 5 );
    f();
    REQUIRE( relationship.tribal_alarm == 10 );

    relationship.tribal_alarm = 95;
    f();
    REQUIRE( relationship.tribal_alarm == 99 );
  }
}

TEST_CASE( "[alarm] max_tribal_alarm_after_pocahontas" ) {
  REQUIRE( max_tribal_alarm_after_pocahontas() == 17 );
  // Whatever value we choose to set the tribal alarm to, it
  // should fall in the "content" category, since that is what
  // the OG documentation says.
  REQUIRE( tribe_alarm_category(
               max_tribal_alarm_after_pocahontas() ) ==
           e_alarm_category::content );
}

TEST_CASE( "[alarm] max_tribal_alarm_after_burning_capital" ) {
  REQUIRE( max_tribal_alarm_after_burning_capital() == 17 );
  // Whatever value we choose to set the tribal alarm to, it
  // should fall in the "content" category, since that is what
  // the OG documentation says.
  REQUIRE( tribe_alarm_category(
               max_tribal_alarm_after_burning_capital() ) ==
           e_alarm_category::content );
}

TEST_CASE( "[alarm] tribe_alarm_category" ) {
  REQUIRE( tribe_alarm_category( 99 ) ==
           e_alarm_category::angry );
  REQUIRE( tribe_alarm_category( 85 ) ==
           e_alarm_category::angry );
  REQUIRE( tribe_alarm_category( 84 ) ==
           e_alarm_category::wary );
  REQUIRE( tribe_alarm_category( 68 ) ==
           e_alarm_category::wary );
  REQUIRE( tribe_alarm_category( 67 ) ==
           e_alarm_category::uneasy );
  REQUIRE( tribe_alarm_category( 51 ) ==
           e_alarm_category::uneasy );
  REQUIRE( tribe_alarm_category( 50 ) ==
           e_alarm_category::restless );
  REQUIRE( tribe_alarm_category( 34 ) ==
           e_alarm_category::restless );
  REQUIRE( tribe_alarm_category( 33 ) ==
           e_alarm_category::content );
  REQUIRE( tribe_alarm_category( 17 ) ==
           e_alarm_category::content );
  REQUIRE( tribe_alarm_category( 16 ) ==
           e_alarm_category::happy );
  REQUIRE( tribe_alarm_category( 0 ) ==
           e_alarm_category::happy );
}

} // namespace
} // namespace rn
