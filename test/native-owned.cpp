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
        _, L, _, //
        L, L, L, //
        _, L, L, //
    };
    build_map( std::move( tiles ), 3 );
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

} // namespace
} // namespace rn
