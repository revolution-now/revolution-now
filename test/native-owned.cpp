/****************************************************************
**native-owned.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-11-19.
*
* Description: Unit tests for the src/native-owned.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/native-owned.hpp"

// Testing
#include "test/fake/world.hpp"

// ss
#include "ss/natives.hpp"
#include "ss/player.rds.hpp"
#include "ss/ref.hpp"
#include "ss/settings.rds.hpp"

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
    create_default_map();
  }

  void create_default_map() {
    MapSquare const   _ = make_ocean();
    MapSquare const   L = make_grassland();
    vector<MapSquare> tiles{
        _, L, _, L, L, L, L, L, L, //
        L, L, L, L, L, L, L, L, L, //
        _, L, L, L, L, L, L, L, L, //
        L, L, L, L, L, L, L, L, L, //
        L, L, _, L, L, L, L, L, L, //
        L, L, L, L, L, L, L, L, L, //
        L, L, L, L, L, L, L, L, L, //
        L, L, L, L, L, L, L, L, L, //
        L, L, L, L, L, L, L, L, L, //
    };
    build_map( std::move( tiles ), 9 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE(
    "[native-owned] is_land_native_owned_after_meeting" ) {
  World           W;
  Coord const     loc = { .x = 1, .y = 1 };
  Dwelling const& dwelling =
      W.add_dwelling( loc, e_tribe::cherokee );
  Player& player = W.default_player();
  Tribe&  tribe  = W.natives().tribe_for( e_tribe::cherokee );

  auto f = [&] {
    return is_land_native_owned_after_meeting( W.ss(), player,
                                               loc );
  };

  REQUIRE( f() == nothing );
  W.natives().mark_land_owned( dwelling.id, loc );
  REQUIRE( f() == dwelling.id );
  tribe.relationship[player.nation].emplace();
  REQUIRE( f() == dwelling.id );
  player.fathers.has[e_founding_father::peter_minuit] = true;
  REQUIRE( f() == nothing );
}

TEST_CASE( "[native-owned] is_land_native_owned" ) {
  World           W;
  Coord const     loc = { .x = 1, .y = 1 };
  Dwelling const& dwelling =
      W.add_dwelling( loc, e_tribe::cherokee );
  Player& player = W.default_player();
  Tribe&  tribe  = W.natives().tribe_for( e_tribe::cherokee );

  auto f = [&] {
    return is_land_native_owned( W.ss(), player, loc );
  };

  REQUIRE( f() == nothing );
  W.natives().mark_land_owned( dwelling.id, loc );
  REQUIRE( f() == nothing );
  tribe.relationship[player.nation].emplace();
  REQUIRE( f() == dwelling.id );
  player.fathers.has[e_founding_father::peter_minuit] = true;
  REQUIRE( f() == nothing );
}

TEST_CASE( "[native-owned] native_owned_land_around_square" ) {
  World           W;
  Coord const     loc = { .x = 1, .y = 1 };
  Dwelling const& dwelling =
      W.add_dwelling( loc, e_tribe::cherokee );
  Player& player = W.default_player();
  Tribe&  tribe  = W.natives().tribe_for( e_tribe::cherokee );
  refl::enum_map<e_direction, maybe<DwellingId>> expected;

  auto f = [&] {
    return native_owned_land_around_square( W.ss(), player,
                                            loc );
  };

  expected = {};
  REQUIRE( f() == expected );

  W.natives().mark_land_owned( dwelling.id, { .x = 1, .y = 0 } );
  W.natives().mark_land_owned( dwelling.id, { .x = 1, .y = 2 } );
  W.natives().mark_land_owned( dwelling.id, { .x = 0, .y = 1 } );
  W.natives().mark_land_owned( dwelling.id, { .x = 2, .y = 1 } );
  W.natives().mark_land_owned( dwelling.id, { .x = 0, .y = 0 } );

  expected = {};
  REQUIRE( f() == expected );

  tribe.relationship[player.nation].emplace();
  expected = {
      { e_direction::nw, dwelling.id },
      { e_direction::n, dwelling.id },
      { e_direction::w, dwelling.id },
      { e_direction::e, dwelling.id },
      { e_direction::s, dwelling.id },
  };
  REQUIRE( f() == expected );

  player.fathers.has[e_founding_father::peter_minuit] = true;
  expected                                            = {};
  REQUIRE( f() == expected );
}

TEST_CASE( "[native-owned] price_for_native_owned_land" ) {
  World            W;
  Player const&    player = W.default_player();
  Coord            tile;
  maybe<LandPrice> expected;
  Coord const      kDwellingLoc{ .x = 3, .y = 3 };

  SECTION( "semi-nomadic" ) {
    Dwelling& dwelling =
        W.add_dwelling( kDwellingLoc, e_tribe::tupi );

    // Mark some tiles as owned.
    for( int y = 0; y < 7; ++y )
      for( int x = 0; x < 7; ++x )
        W.natives().mark_land_owned( dwelling.id,
                                     { .x = x, .y = y } );

    auto f = [&] {
      return price_for_native_owned_land(
          W.ss(), W.default_player(), tile );
    };

    W.settings().difficulty = e_difficulty::discoverer;
    Tribe& tribe = W.natives().tribe_for( dwelling.tribe );
    maybe<TribeRelationship>& relationship =
        tribe.relationship[player.nation];

    // No relationship.
    tile     = { .x = 2, .y = 2 };
    expected = nothing;
    REQUIRE( f() == expected );

    // With relationship.
    relationship.emplace();
    expected =
        LandPrice{ .owner = e_tribe::tupi, .price = int( 65 ) };
    REQUIRE( f() == expected );

    // With relationship at war.
    relationship->at_war = true;
    expected =
        LandPrice{ .owner = e_tribe::tupi, .price = int( 65 ) };
    REQUIRE( f() == expected );

    tile = { .x = 4, .y = 2 };
    expected =
        LandPrice{ .owner = e_tribe::tupi, .price = int( 65 ) };
    REQUIRE( f() == expected );

    // Difficulty levels.
    W.settings().difficulty = e_difficulty::conquistador;
    expected                = LandPrice{ .owner = e_tribe::tupi,
                                         .price = int( 65 + 65 * 2 ) };
    REQUIRE( f() == expected );

    W.settings().difficulty = e_difficulty::governor;
    expected                = LandPrice{ .owner = e_tribe::tupi,
                                         .price = int( 65 + 65 * 3 ) };
    REQUIRE( f() == expected );

    W.settings().difficulty = e_difficulty::viceroy;
    expected                = LandPrice{ .owner = e_tribe::tupi,
                                         .price = int( 65 + 65 * 4 ) };
    REQUIRE( f() == expected );

    W.settings().difficulty = e_difficulty::explorer;
    expected                = LandPrice{ .owner = e_tribe::tupi,
                                         .price = int( 65 + 65 * 1 ) };
    REQUIRE( f() == expected );

    double so_far = 65 + 65 * 1;

    // 1 colony.
    W.add_colony( { .x = 0, .y = 9 } );
    expected = LandPrice{ .owner = e_tribe::tupi,
                          .price = int( so_far + 0 ) };
    REQUIRE( f() == expected );
    // 2 colonies.
    W.add_colony( { .x = 2, .y = 9 } );
    expected = LandPrice{ .owner = e_tribe::tupi,
                          .price = int( so_far + 32.5 * 1 ) };
    REQUIRE( f() == expected );
    // 4 colonies.
    W.add_colony( { .x = 4, .y = 9 } );
    W.add_colony( { .x = 6, .y = 9 } );
    expected = LandPrice{ .owner = e_tribe::tupi,
                          .price = int( so_far + 32.5 * 2 ) };
    REQUIRE( f() == expected );
    // 5 colonies.
    W.add_colony( { .x = 8, .y = 9 } );
    expected = LandPrice{ .owner = e_tribe::tupi,
                          .price = int( so_far + 32.5 * 2 ) };
    REQUIRE( f() == expected );
    // 6 colonies.
    W.add_colony( { .x = 8, .y = 7 } );
    expected = LandPrice{ .owner = e_tribe::tupi,
                          .price = int( so_far + 32.5 * 3 ) };
    REQUIRE( f() == expected );
    // 8 colonies.
    W.add_colony( { .x = 8, .y = 5 } );
    W.add_colony( { .x = 8, .y = 3 } );
    expected = LandPrice{ .owner = e_tribe::tupi,
                          .price = int( so_far + 32.5 * 3 ) };
    REQUIRE( f() == expected );

    so_far += 32.5 * 3;

    // Paid already = 1.
    ++relationship->land_squares_paid_for;
    expected = LandPrice{ .owner = e_tribe::tupi,
                          .price = int( so_far + 32.5 * 1 ) };
    REQUIRE( f() == expected );
    // Paid already = 3.
    relationship->land_squares_paid_for += 2;
    expected = LandPrice{ .owner = e_tribe::tupi,
                          .price = int( so_far + 32.5 * 3 ) };
    REQUIRE( f() == expected );

    so_far += 32.5 * 3;

    // Prime resource.
    W.square( tile ).ground_resource =
        e_natural_resource::tobacco;
    so_far *= 2.0;
    expected = LandPrice{ .owner = e_tribe::tupi,
                          .price = int( so_far ) };
    REQUIRE( f() == expected );

    // Capital.
    dwelling.is_capital = true;
    so_far *= 1.5;
    expected = LandPrice{ .owner = e_tribe::tupi,
                          .price = int( so_far ) };
    REQUIRE( f() == expected );

    // Tribal anger = 20.
    relationship->tribal_alarm = 20;
    expected = LandPrice{ .owner = e_tribe::tupi,
                          .price = int( so_far * 1 ) };
    REQUIRE( f() == expected );
    // Tribal anger = 40.
    relationship->tribal_alarm = 40;
    expected = LandPrice{ .owner = e_tribe::tupi,
                          .price = int( so_far * 2 ) };
    REQUIRE( f() == expected );
    // Tribal anger = 60.
    relationship->tribal_alarm = 60;
    expected = LandPrice{ .owner = e_tribe::tupi,
                          .price = int( so_far * 3 ) };
    REQUIRE( f() == expected );
    // Tribal anger = 80.
    relationship->tribal_alarm = 80;
    expected = LandPrice{ .owner = e_tribe::tupi,
                          .price = int( so_far * 4 ) };
    REQUIRE( f() == expected );
    // Tribal anger = 99.
    relationship->tribal_alarm = 99;
    expected = LandPrice{ .owner = e_tribe::tupi,
                          .price = int( so_far * 4 ) };
    REQUIRE( f() == expected );
    so_far *= 4;

    // Distance modifier, second square.
    so_far /= 2.0; // remove prime resource modifier.
    --tile.y;
    so_far *= .9;
    expected = LandPrice{ .owner = e_tribe::tupi,
                          .price = int( so_far ) };
    REQUIRE( f() == expected );
    // Distance modifier, third square.
    --tile.y;
    so_far *= .9;
    expected = LandPrice{ .owner = e_tribe::tupi,
                          .price = int( so_far ) };
    REQUIRE( f() == expected );

    // On top of dwelling.
    tile     = kDwellingLoc;
    expected = nothing;
    REQUIRE( f() == expected );
  }

  SECTION( "agrarian" ) {
    Dwelling& dwelling =
        W.add_dwelling( kDwellingLoc, e_tribe::iroquois );

    // Mark some tiles as owned.
    for( int y = 0; y < 7; ++y )
      for( int x = 0; x < 7; ++x )
        W.natives().mark_land_owned( dwelling.id,
                                     { .x = x, .y = y } );

    auto f = [&] {
      return price_for_native_owned_land(
          W.ss(), W.default_player(), tile );
    };

    W.settings().difficulty = e_difficulty::discoverer;
    Tribe& tribe = W.natives().tribe_for( dwelling.tribe );
    maybe<TribeRelationship>& relationship =
        tribe.relationship[player.nation];

    // No relationship.
    tile     = { .x = 2, .y = 2 };
    expected = nothing;
    REQUIRE( f() == expected );

    // With relationship.
    relationship.emplace();
    expected = LandPrice{ .owner = e_tribe::iroquois,
                          .price = int( 65 + 32.5 ) };
    REQUIRE( f() == expected );

    // With relationship at war.
    relationship->at_war = true;
    expected             = LandPrice{ .owner = e_tribe::iroquois,
                                      .price = int( 65 + 32.5 ) };
    REQUIRE( f() == expected );

    tile     = { .x = 4, .y = 2 };
    expected = LandPrice{ .owner = e_tribe::iroquois,
                          .price = int( 65 + 32.5 ) };
    REQUIRE( f() == expected );

    // Difficulty levels.
    W.settings().difficulty = e_difficulty::conquistador;
    expected = LandPrice{ .owner = e_tribe::iroquois,
                          .price = int( 65 + 32.5 + 65 * 2 ) };
    REQUIRE( f() == expected );

    W.settings().difficulty = e_difficulty::governor;
    expected = LandPrice{ .owner = e_tribe::iroquois,
                          .price = int( 65 + 32.5 + 65 * 3 ) };
    REQUIRE( f() == expected );

    W.settings().difficulty = e_difficulty::viceroy;
    expected = LandPrice{ .owner = e_tribe::iroquois,
                          .price = int( 65 + 32.5 + 65 * 4 ) };
    REQUIRE( f() == expected );

    W.settings().difficulty = e_difficulty::explorer;
    expected = LandPrice{ .owner = e_tribe::iroquois,
                          .price = int( 65 + 32.5 + 65 * 1 ) };
    REQUIRE( f() == expected );

    double so_far = 65 + 32.5 + 65 * 1;

    // 1 colony.
    W.add_colony( { .x = 0, .y = 9 } );
    expected = LandPrice{ .owner = e_tribe::iroquois,
                          .price = int( so_far + 0 ) };
    REQUIRE( f() == expected );
    // 2 colonies.
    W.add_colony( { .x = 2, .y = 9 } );
    expected = LandPrice{ .owner = e_tribe::iroquois,
                          .price = int( so_far + 32.5 * 1 ) };
    REQUIRE( f() == expected );
    // 4 colonies.
    W.add_colony( { .x = 4, .y = 9 } );
    W.add_colony( { .x = 6, .y = 9 } );
    expected = LandPrice{ .owner = e_tribe::iroquois,
                          .price = int( so_far + 32.5 * 2 ) };
    REQUIRE( f() == expected );
    // 5 colonies.
    W.add_colony( { .x = 8, .y = 9 } );
    expected = LandPrice{ .owner = e_tribe::iroquois,
                          .price = int( so_far + 32.5 * 2 ) };
    REQUIRE( f() == expected );
    // 6 colonies.
    W.add_colony( { .x = 8, .y = 7 } );
    expected = LandPrice{ .owner = e_tribe::iroquois,
                          .price = int( so_far + 32.5 * 3 ) };
    REQUIRE( f() == expected );
    // 8 colonies.
    W.add_colony( { .x = 8, .y = 5 } );
    W.add_colony( { .x = 8, .y = 3 } );
    expected = LandPrice{ .owner = e_tribe::iroquois,
                          .price = int( so_far + 32.5 * 3 ) };
    REQUIRE( f() == expected );

    so_far += 32.5 * 3;

    // Paid already = 1.
    ++relationship->land_squares_paid_for;
    expected = LandPrice{ .owner = e_tribe::iroquois,
                          .price = int( so_far + 32.5 * 1 ) };
    REQUIRE( f() == expected );
    // Paid already = 3.
    relationship->land_squares_paid_for += 2;
    expected = LandPrice{ .owner = e_tribe::iroquois,
                          .price = int( so_far + 32.5 * 3 ) };
    REQUIRE( f() == expected );

    so_far += 32.5 * 3;

    // Prime resource.
    W.square( tile ).ground_resource =
        e_natural_resource::tobacco;
    so_far *= 2.0;
    expected = LandPrice{ .owner = e_tribe::iroquois,
                          .price = int( so_far ) };
    REQUIRE( f() == expected );

    // Capital.
    dwelling.is_capital = true;
    so_far *= 1.5;
    expected = LandPrice{ .owner = e_tribe::iroquois,
                          .price = int( so_far ) };
    REQUIRE( f() == expected );

    // Tribal anger = 20.
    relationship->tribal_alarm = 20;
    expected = LandPrice{ .owner = e_tribe::iroquois,
                          .price = int( so_far * 1 ) };
    REQUIRE( f() == expected );
    // Tribal anger = 40.
    relationship->tribal_alarm = 40;
    expected = LandPrice{ .owner = e_tribe::iroquois,
                          .price = int( so_far * 2 ) };
    REQUIRE( f() == expected );
    // Tribal anger = 60.
    relationship->tribal_alarm = 60;
    expected = LandPrice{ .owner = e_tribe::iroquois,
                          .price = int( so_far * 3 ) };
    REQUIRE( f() == expected );
    // Tribal anger = 80.
    relationship->tribal_alarm = 80;
    expected = LandPrice{ .owner = e_tribe::iroquois,
                          .price = int( so_far * 4 ) };
    REQUIRE( f() == expected );
    // Tribal anger = 99.
    relationship->tribal_alarm = 99;
    expected = LandPrice{ .owner = e_tribe::iroquois,
                          .price = int( so_far * 4 ) };
    REQUIRE( f() == expected );
    so_far *= 4;

    // Distance modifier, second square.
    so_far /= 2.0; // remove prime resource modifier.
    --tile.y;
    so_far *= .9;
    expected = LandPrice{ .owner = e_tribe::iroquois,
                          .price = int( so_far ) };
    REQUIRE( f() == expected );
    // Distance modifier, third square.
    --tile.y;
    so_far *= .9;
    expected = LandPrice{ .owner = e_tribe::iroquois,
                          .price = int( so_far ) };
    REQUIRE( f() == expected );

    // On top of dwelling.
    tile     = kDwellingLoc;
    expected = nothing;
    REQUIRE( f() == expected );
  }

  SECTION( "aztec" ) {
    Dwelling& dwelling =
        W.add_dwelling( kDwellingLoc, e_tribe::aztec );

    // Mark some tiles as owned.
    for( int y = 0; y < 7; ++y )
      for( int x = 0; x < 7; ++x )
        W.natives().mark_land_owned( dwelling.id,
                                     { .x = x, .y = y } );

    auto f = [&] {
      return price_for_native_owned_land(
          W.ss(), W.default_player(), tile );
    };

    W.settings().difficulty = e_difficulty::discoverer;
    Tribe& tribe = W.natives().tribe_for( dwelling.tribe );
    maybe<TribeRelationship>& relationship =
        tribe.relationship[player.nation];

    // No relationship.
    tile     = { .x = 2, .y = 2 };
    expected = nothing;
    REQUIRE( f() == expected );

    // With relationship.
    relationship.emplace();
    expected = LandPrice{ .owner = e_tribe::aztec,
                          .price = int( 65 + 32.5 * 2 ) };
    REQUIRE( f() == expected );

    // With relationship at war.
    relationship->at_war = true;
    expected             = LandPrice{ .owner = e_tribe::aztec,
                                      .price = int( 65 + 32.5 * 2 ) };
    REQUIRE( f() == expected );

    tile     = { .x = 4, .y = 2 };
    expected = LandPrice{ .owner = e_tribe::aztec,
                          .price = int( 65 + 32.5 * 2 ) };
    REQUIRE( f() == expected );

    double so_far = 65 + 32.5 * 2;

    // Difficulty levels.
    W.settings().difficulty = e_difficulty::conquistador;
    expected                = LandPrice{ .owner = e_tribe::aztec,
                                         .price = int( so_far + 65 * 2 ) };
    REQUIRE( f() == expected );

    W.settings().difficulty = e_difficulty::governor;
    expected                = LandPrice{ .owner = e_tribe::aztec,
                                         .price = int( so_far + 65 * 3 ) };
    REQUIRE( f() == expected );

    W.settings().difficulty = e_difficulty::viceroy;
    expected                = LandPrice{ .owner = e_tribe::aztec,
                                         .price = int( so_far + 65 * 4 ) };
    REQUIRE( f() == expected );

    W.settings().difficulty = e_difficulty::explorer;
    expected                = LandPrice{ .owner = e_tribe::aztec,
                                         .price = int( so_far + 65 * 1 ) };
    REQUIRE( f() == expected );

    so_far += 65 * 1;

    // 1 colony.
    W.add_colony( { .x = 0, .y = 9 } );
    expected = LandPrice{ .owner = e_tribe::aztec,
                          .price = int( so_far + 0 ) };
    REQUIRE( f() == expected );
    // 2 colonies.
    W.add_colony( { .x = 2, .y = 9 } );
    expected = LandPrice{ .owner = e_tribe::aztec,
                          .price = int( so_far + 32.5 * 1 ) };
    REQUIRE( f() == expected );
    // 4 colonies.
    W.add_colony( { .x = 4, .y = 9 } );
    W.add_colony( { .x = 6, .y = 9 } );
    expected = LandPrice{ .owner = e_tribe::aztec,
                          .price = int( so_far + 32.5 * 2 ) };
    REQUIRE( f() == expected );
    // 5 colonies.
    W.add_colony( { .x = 8, .y = 9 } );
    expected = LandPrice{ .owner = e_tribe::aztec,
                          .price = int( so_far + 32.5 * 2 ) };
    REQUIRE( f() == expected );
    // 6 colonies.
    W.add_colony( { .x = 8, .y = 7 } );
    expected = LandPrice{ .owner = e_tribe::aztec,
                          .price = int( so_far + 32.5 * 3 ) };
    REQUIRE( f() == expected );
    // 8 colonies.
    W.add_colony( { .x = 8, .y = 5 } );
    W.add_colony( { .x = 8, .y = 3 } );
    expected = LandPrice{ .owner = e_tribe::aztec,
                          .price = int( so_far + 32.5 * 3 ) };
    REQUIRE( f() == expected );

    so_far += 32.5 * 3;

    // Paid already = 1.
    ++relationship->land_squares_paid_for;
    expected = LandPrice{ .owner = e_tribe::aztec,
                          .price = int( so_far + 32.5 * 1 ) };
    REQUIRE( f() == expected );
    // Paid already = 3.
    relationship->land_squares_paid_for += 2;
    expected = LandPrice{ .owner = e_tribe::aztec,
                          .price = int( so_far + 32.5 * 3 ) };
    REQUIRE( f() == expected );

    so_far += 32.5 * 3;

    // Prime resource.
    W.square( tile ).ground_resource =
        e_natural_resource::tobacco;
    so_far *= 2.0;
    expected = LandPrice{ .owner = e_tribe::aztec,
                          .price = int( so_far ) };
    REQUIRE( f() == expected );

    // Capital.
    dwelling.is_capital = true;
    so_far *= 1.5;
    expected = LandPrice{ .owner = e_tribe::aztec,
                          .price = int( so_far ) };
    REQUIRE( f() == expected );

    // Tribal anger = 20.
    relationship->tribal_alarm = 20;
    expected = LandPrice{ .owner = e_tribe::aztec,
                          .price = int( so_far * 1 ) };
    REQUIRE( f() == expected );
    // Tribal anger = 40.
    relationship->tribal_alarm = 40;
    expected = LandPrice{ .owner = e_tribe::aztec,
                          .price = int( so_far * 2 ) };
    REQUIRE( f() == expected );
    // Tribal anger = 60.
    relationship->tribal_alarm = 60;
    expected = LandPrice{ .owner = e_tribe::aztec,
                          .price = int( so_far * 3 ) };
    REQUIRE( f() == expected );
    // Tribal anger = 80.
    relationship->tribal_alarm = 80;
    expected = LandPrice{ .owner = e_tribe::aztec,
                          .price = int( so_far * 4 ) };
    REQUIRE( f() == expected );
    // Tribal anger = 99.
    relationship->tribal_alarm = 99;
    expected = LandPrice{ .owner = e_tribe::aztec,
                          .price = int( so_far * 4 ) };
    REQUIRE( f() == expected );
    so_far *= 4;

    // Distance modifier, second square.
    so_far /= 2.0; // remove prime resource modifier.
    --tile.y;
    so_far *= .9;
    expected = LandPrice{ .owner = e_tribe::aztec,
                          .price = int( so_far ) };
    REQUIRE( f() == expected );
    // Distance modifier, third square.
    --tile.y;
    so_far *= .9;
    expected = LandPrice{ .owner = e_tribe::aztec,
                          .price = int( so_far ) };
    REQUIRE( f() == expected );

    // On top of dwelling.
    tile     = kDwellingLoc;
    expected = nothing;
    REQUIRE( f() == expected );
  }

  SECTION( "inca" ) {
    Dwelling& dwelling =
        W.add_dwelling( kDwellingLoc, e_tribe::inca );

    // Mark some tiles as owned.
    for( int y = 0; y < 7; ++y )
      for( int x = 0; x < 7; ++x )
        W.natives().mark_land_owned( dwelling.id,
                                     { .x = x, .y = y } );

    auto f = [&] {
      return price_for_native_owned_land(
          W.ss(), W.default_player(), tile );
    };

    W.settings().difficulty = e_difficulty::discoverer;
    Tribe& tribe = W.natives().tribe_for( dwelling.tribe );
    maybe<TribeRelationship>& relationship =
        tribe.relationship[player.nation];

    // No relationship.
    tile     = { .x = 2, .y = 2 };
    expected = nothing;
    REQUIRE( f() == expected );

    // With relationship.
    relationship.emplace();
    expected = LandPrice{ .owner = e_tribe::inca,
                          .price = int( 65 + 32.5 * 3 ) };
    REQUIRE( f() == expected );

    // With relationship at war.
    relationship->at_war = true;
    expected             = LandPrice{ .owner = e_tribe::inca,
                                      .price = int( 65 + 32.5 * 3 ) };
    REQUIRE( f() == expected );

    tile     = { .x = 4, .y = 2 };
    expected = LandPrice{ .owner = e_tribe::inca,
                          .price = int( 65 + 32.5 * 3 ) };
    REQUIRE( f() == expected );

    double so_far = 65 + 32.5 * 3;

    // Difficulty levels.
    W.settings().difficulty = e_difficulty::conquistador;
    expected                = LandPrice{ .owner = e_tribe::inca,
                                         .price = int( so_far + 65 * 2 ) };
    REQUIRE( f() == expected );

    W.settings().difficulty = e_difficulty::governor;
    expected                = LandPrice{ .owner = e_tribe::inca,
                                         .price = int( so_far + 65 * 3 ) };
    REQUIRE( f() == expected );

    W.settings().difficulty = e_difficulty::viceroy;
    expected                = LandPrice{ .owner = e_tribe::inca,
                                         .price = int( so_far + 65 * 4 ) };
    REQUIRE( f() == expected );

    W.settings().difficulty = e_difficulty::explorer;
    expected                = LandPrice{ .owner = e_tribe::inca,
                                         .price = int( so_far + 65 * 1 ) };
    REQUIRE( f() == expected );

    so_far += 65 * 1;

    // 1 colony.
    W.add_colony( { .x = 0, .y = 9 } );
    expected = LandPrice{ .owner = e_tribe::inca,
                          .price = int( so_far + 0 ) };
    REQUIRE( f() == expected );
    // 2 colonies.
    W.add_colony( { .x = 2, .y = 9 } );
    expected = LandPrice{ .owner = e_tribe::inca,
                          .price = int( so_far + 32.5 * 1 ) };
    REQUIRE( f() == expected );
    // 4 colonies.
    W.add_colony( { .x = 4, .y = 9 } );
    W.add_colony( { .x = 6, .y = 9 } );
    expected = LandPrice{ .owner = e_tribe::inca,
                          .price = int( so_far + 32.5 * 2 ) };
    REQUIRE( f() == expected );
    // 5 colonies.
    W.add_colony( { .x = 8, .y = 9 } );
    expected = LandPrice{ .owner = e_tribe::inca,
                          .price = int( so_far + 32.5 * 2 ) };
    REQUIRE( f() == expected );
    // 6 colonies.
    W.add_colony( { .x = 8, .y = 7 } );
    expected = LandPrice{ .owner = e_tribe::inca,
                          .price = int( so_far + 32.5 * 3 ) };
    REQUIRE( f() == expected );
    // 8 colonies.
    W.add_colony( { .x = 8, .y = 5 } );
    W.add_colony( { .x = 8, .y = 3 } );
    expected = LandPrice{ .owner = e_tribe::inca,
                          .price = int( so_far + 32.5 * 3 ) };
    REQUIRE( f() == expected );

    so_far += 32.5 * 3;

    // Paid already = 1.
    ++relationship->land_squares_paid_for;
    expected = LandPrice{ .owner = e_tribe::inca,
                          .price = int( so_far + 32.5 * 1 ) };
    REQUIRE( f() == expected );
    // Paid already = 3.
    relationship->land_squares_paid_for += 2;
    expected = LandPrice{ .owner = e_tribe::inca,
                          .price = int( so_far + 32.5 * 3 ) };
    REQUIRE( f() == expected );

    so_far += 32.5 * 3;

    // Prime resource.
    W.square( tile ).ground_resource =
        e_natural_resource::tobacco;
    so_far *= 2.0;
    expected = LandPrice{ .owner = e_tribe::inca,
                          .price = int( so_far ) };
    REQUIRE( f() == expected );

    // Capital.
    dwelling.is_capital = true;
    so_far *= 1.5;
    expected = LandPrice{ .owner = e_tribe::inca,
                          .price = int( so_far ) };
    REQUIRE( f() == expected );

    // Tribal anger = 20.
    relationship->tribal_alarm = 20;
    expected = LandPrice{ .owner = e_tribe::inca,
                          .price = int( so_far * 1 ) };
    REQUIRE( f() == expected );
    // Tribal anger = 40.
    relationship->tribal_alarm = 40;
    expected = LandPrice{ .owner = e_tribe::inca,
                          .price = int( so_far * 2 ) };
    REQUIRE( f() == expected );
    // Tribal anger = 60.
    relationship->tribal_alarm = 60;
    expected = LandPrice{ .owner = e_tribe::inca,
                          .price = int( so_far * 3 ) };
    REQUIRE( f() == expected );
    // Tribal anger = 80.
    relationship->tribal_alarm = 80;
    expected = LandPrice{ .owner = e_tribe::inca,
                          .price = int( so_far * 4 ) };
    REQUIRE( f() == expected );
    // Tribal anger = 99.
    relationship->tribal_alarm = 99;
    expected = LandPrice{ .owner = e_tribe::inca,
                          .price = int( so_far * 4 ) };
    REQUIRE( f() == expected );
    so_far *= 4;

    // Distance modifier, second square.
    so_far /= 2.0; // remove prime resource modifier.
    --tile.y;
    so_far *= .9;
    expected = LandPrice{ .owner = e_tribe::inca,
                          .price = int( so_far ) };
    REQUIRE( f() == expected );
    // Distance modifier, third square.
    --tile.y;
    so_far *= .9;
    expected = LandPrice{ .owner = e_tribe::inca,
                          .price = int( so_far ) };
    REQUIRE( f() == expected );

    // On top of dwelling.
    tile     = kDwellingLoc;
    expected = nothing;
    REQUIRE( f() == expected );
  }
}

} // namespace
} // namespace rn
