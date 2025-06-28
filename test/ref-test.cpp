/****************************************************************
**ref-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-05-18.
*
* Description: Unit tests for the ref module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/ref.hpp"

// Testing.
#include "test/fake/world.hpp"
#include "test/mocks/ieuro-agent.hpp"
#include "test/util/coro.hpp"

// ss
#include "src/ss/player.rds.hpp"
#include "src/ss/ref.hpp"
#include "src/ss/revolution.rds.hpp"
#include "src/ss/settings.rds.hpp"

// refl
#include "src/refl/to-str.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct world : testing::World {
  world() {
    add_default_player();
    create_default_map();
  }

  void create_default_map() {
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
    vector<MapSquare> tiles{
      L, L, L, //
      L, _, L, //
      L, L, L, //
    };
    build_map( std::move( tiles ), 3 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[ref] evolved_royal_money" ) {
  RoyalMoneyChange expected;

  int royal_money         = 0;
  e_difficulty difficulty = {};

  auto const f = [&] {
    return evolved_royal_money( difficulty, royal_money );
  };

  using enum e_difficulty;

  difficulty  = discoverer;
  royal_money = 0;
  expected    = {
       .old_value         = 0,
       .per_turn_increase = 10,
       .new_value         = 10,
       .new_unit_produced = false,
       .amount_subtracted = 0,
  };
  REQUIRE( f() == expected );

  difficulty  = explorer;
  royal_money = 1781;
  expected    = {
       .old_value         = 1781,
       .per_turn_increase = 18,
       .new_value         = 1799,
       .new_unit_produced = false,
       .amount_subtracted = 0,
  };
  REQUIRE( f() == expected );

  difficulty  = conquistador;
  royal_money = 30;
  expected    = {
       .old_value         = 30,
       .per_turn_increase = 26,
       .new_value         = 56,
       .new_unit_produced = false,
       .amount_subtracted = 0,
  };
  REQUIRE( f() == expected );

  difficulty  = governor;
  royal_money = 1770;
  expected    = {
       .old_value         = 1770,
       .per_turn_increase = 34,
       .new_value         = 4,
       .new_unit_produced = true,
       .amount_subtracted = 1800,
  };
  REQUIRE( f() == expected );

  difficulty  = viceroy;
  royal_money = 1758;
  expected    = {
       .old_value         = 1758,
       .per_turn_increase = 42,
       .new_value         = 0,
       .new_unit_produced = true,
       .amount_subtracted = 1800,
  };
  REQUIRE( f() == expected );
}

TEST_CASE( "[ref] apply_royal_money_change" ) {
  Player player;
  RoyalMoneyChange change = {};

  auto const f = [&] {
    apply_royal_money_change( player, change );
  };

  REQUIRE( player.royal_money == 0 );

  change = {};
  f();
  REQUIRE( player.royal_money == 0 );

  change = {
    .old_value         = 9999, // Should not be used.
    .per_turn_increase = 9999, // Should not be used.
    .new_value         = 123,
    .new_unit_produced = true, // Should not be used.
    .amount_subtracted = 9999, // Should not be used.
  };
  f();
  REQUIRE( player.royal_money == 123 );
  f();
  REQUIRE( player.royal_money == 123 );

  change = {
    .old_value         = 9999, // Should not be used.
    .per_turn_increase = 9999, // Should not be used.
    .new_value         = 234,
    .new_unit_produced = true, // Should not be used.
    .amount_subtracted = 9999, // Should not be used.
  };
  f();
  REQUIRE( player.royal_money == 234 );
  f();
  REQUIRE( player.royal_money == 234 );
}

TEST_CASE( "[ref] select_next_ref_type" ) {
  ExpeditionaryForce force;
  e_expeditionary_force_type expected = {};

  auto const f = [&] { return select_next_ref_type( force ); };

  using enum e_expeditionary_force_type;

  force = {
    .regular = 0, .cavalry = 0, .artillery = 0, .man_o_war = 0 };
  expected = man_o_war;
  REQUIRE( f() == expected );

  force = {
    .regular = 0, .cavalry = 0, .artillery = 0, .man_o_war = 1 };
  expected = regular;
  REQUIRE( f() == expected );

  force = {
    .regular = 1, .cavalry = 0, .artillery = 0, .man_o_war = 1 };
  expected = cavalry;
  REQUIRE( f() == expected );

  force = {
    .regular = 1, .cavalry = 1, .artillery = 0, .man_o_war = 1 };
  expected = artillery;
  REQUIRE( f() == expected );

  force = {
    .regular = 1, .cavalry = 1, .artillery = 1, .man_o_war = 1 };
  expected = regular;
  REQUIRE( f() == expected );

  force = {
    .regular = 2, .cavalry = 1, .artillery = 1, .man_o_war = 1 };
  expected = regular;
  REQUIRE( f() == expected );

  force = {
    .regular = 3, .cavalry = 1, .artillery = 1, .man_o_war = 1 };
  expected = regular;
  REQUIRE( f() == expected );

  force = {
    .regular = 4, .cavalry = 1, .artillery = 1, .man_o_war = 1 };
  expected = cavalry;
  REQUIRE( f() == expected );

  force = {
    .regular = 4, .cavalry = 2, .artillery = 1, .man_o_war = 1 };
  expected = regular;
  REQUIRE( f() == expected );

  force = {
    .regular = 5, .cavalry = 2, .artillery = 1, .man_o_war = 1 };
  expected = artillery;
  REQUIRE( f() == expected );

  force = {
    .regular = 5, .cavalry = 2, .artillery = 2, .man_o_war = 1 };
  expected = regular;
  REQUIRE( f() == expected );

  force = {
    .regular = 6, .cavalry = 2, .artillery = 2, .man_o_war = 1 };
  expected = regular;
  REQUIRE( f() == expected );

  force = {
    .regular = 7, .cavalry = 2, .artillery = 2, .man_o_war = 1 };
  expected = cavalry;
  REQUIRE( f() == expected );

  force = {
    .regular = 7, .cavalry = 3, .artillery = 2, .man_o_war = 1 };
  expected = man_o_war;
  REQUIRE( f() == expected );

  force    = { .regular   = 580,
               .cavalry   = 190,
               .artillery = 140,
               .man_o_war = 90 };
  expected = regular;
  REQUIRE( f() == expected );

  force    = { .regular   = 579,
               .cavalry   = 190,
               .artillery = 140,
               .man_o_war = 90 };
  expected = regular;
  REQUIRE( f() == expected );

  force    = { .regular   = 580,
               .cavalry   = 189,
               .artillery = 140,
               .man_o_war = 90 };
  expected = cavalry;
  REQUIRE( f() == expected );

  force    = { .regular   = 580,
               .cavalry   = 190,
               .artillery = 139,
               .man_o_war = 90 };
  expected = artillery;
  REQUIRE( f() == expected );

  force    = { .regular   = 580,
               .cavalry   = 190,
               .artillery = 140,
               .man_o_war = 89 };
  expected = man_o_war;
  REQUIRE( f() == expected );

  force    = { .regular   = 6,
               .cavalry   = 200,
               .artillery = 2,
               .man_o_war = 1 };
  expected = regular;
  REQUIRE( f() == expected );
}

TEST_CASE( "[ref] add_ref_unit" ) {
  world w;
  ExpeditionaryForce force, expected;
  e_expeditionary_force_type type = {};

  auto const f = [&] { add_ref_unit( force, type ); };

  using enum e_expeditionary_force_type;

  force    = { .regular   = 6,
               .cavalry   = 200,
               .artillery = 2,
               .man_o_war = 1 };
  type     = artillery;
  expected = { .regular   = 6,
               .cavalry   = 200,
               .artillery = 3,
               .man_o_war = 1 };
  f();
  REQUIRE( force == expected );

  force    = { .regular   = 6,
               .cavalry   = 200,
               .artillery = 2,
               .man_o_war = 1 };
  type     = regular;
  expected = { .regular   = 7,
               .cavalry   = 200,
               .artillery = 2,
               .man_o_war = 1 };
  f();
  REQUIRE( force == expected );

  force    = { .regular   = 6,
               .cavalry   = 200,
               .artillery = 2,
               .man_o_war = 1 };
  type     = cavalry;
  expected = { .regular   = 6,
               .cavalry   = 201,
               .artillery = 2,
               .man_o_war = 1 };
  f();
  REQUIRE( force == expected );

  force    = { .regular   = 6,
               .cavalry   = 200,
               .artillery = 2,
               .man_o_war = 1 };
  type     = man_o_war;
  expected = { .regular   = 6,
               .cavalry   = 200,
               .artillery = 2,
               .man_o_war = 2 };
  f();
  REQUIRE( force == expected );
}

TEST_CASE( "[ref] ref_unit_to_unit_type" ) {
  world w;

  auto const f = []( e_expeditionary_force_type const type ) {
    return ref_unit_to_unit_type( type );
  };

  using enum e_expeditionary_force_type;

  REQUIRE( f( regular ) == e_unit_type::regular );
  REQUIRE( f( cavalry ) == e_unit_type::cavalry );
  REQUIRE( f( artillery ) == e_unit_type::artillery );
  REQUIRE( f( man_o_war ) == e_unit_type::man_o_war );
}

TEST_CASE( "[ref] add_ref_unit_ui_seq" ) {
  world w;

  MockIEuroAgent& mock_agent =
      w.euro_agent( w.default_player_type() );

  auto const f = [&]( e_expeditionary_force_type const type ) {
    co_await_test( add_ref_unit_ui_seq( mock_agent, type ) );
  };

  using enum e_expeditionary_force_type;

  mock_agent.EXPECT__message_box(
      "The King has announced an increase to the Royal military "
      "budget. [Regulars] have been added to the Royal "
      "Expeditionary Force, causing alarm among colonists." );
  f( regular );

  mock_agent.EXPECT__message_box(
      "The King has announced an increase to the Royal military "
      "budget. [Men-O-War] have been added to the Royal "
      "Expeditionary Force, causing alarm among colonists." );
  f( man_o_war );
}

TEST_CASE( "[ref] add_ref_unit (loop)" ) {
  ExpeditionaryForce force, expected;

  auto const one_round = [&] {
    e_expeditionary_force_type const type =
        select_next_ref_type( force );
    add_ref_unit( force, type );
  };

  expected = {
    .regular   = 0,
    .cavalry   = 0,
    .artillery = 0,
    .man_o_war = 0,
  };
  REQUIRE( force == expected );

  for( int i = 0; i < 1000; ++i ) one_round();

  expected = {
    .regular   = 580,
    .cavalry   = 190,
    .artillery = 140,
    .man_o_war = 90,
  };
  REQUIRE( force == expected );
}

} // namespace
} // namespace rn
