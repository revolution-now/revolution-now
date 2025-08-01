/****************************************************************
**declare-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-04-13.
*
* Description: Unit tests for the declare module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/declare.hpp"

// Testing.
#include "test/fake/world.hpp"
#include "test/mocking.hpp"
#include "test/mocks/iengine.hpp"
#include "test/mocks/igui.hpp"
#include "test/util/coro.hpp"

// Revolution Now
#include "src/harbor-units.hpp"

// ss
#include "src/ss/colonies.hpp"
#include "src/ss/colony.rds.hpp"
#include "src/ss/nation.hpp"
#include "src/ss/natives.hpp"
#include "src/ss/player.rds.hpp"
#include "src/ss/players.rds.hpp"
#include "src/ss/ref.hpp"
#include "src/ss/revolution.rds.hpp"
#include "src/ss/settings.rds.hpp"
#include "src/ss/terrain.hpp"
#include "src/ss/unit-composition.hpp"
#include "src/ss/unit.hpp"
#include "src/ss/units.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

using ::base::valid;
using ::mock::matchers::_;
using ::mock::matchers::StrContains;
using ::refl::enum_values;

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
TEST_CASE( "[declare] human_player_that_declared" ) {
  world w;

  auto const f = [&] {
    return human_player_that_declared( w.ss() );
  };

  using enum e_player;

  w.add_player( english );
  w.add_player( french );
  w.add_player( spanish );

  REQUIRE( f() == nothing );

  w.spanish().control = e_player_control::human;
  REQUIRE( f() == nothing );

  w.french().control = e_player_control::human;
  REQUIRE( f() == nothing );

  w.spanish().revolution.status = e_revolution_status::declared;
  REQUIRE( f() == spanish );

  w.french().revolution.status = e_revolution_status::declared;
  w.spanish().revolution.status =
      e_revolution_status::not_declared;
  REQUIRE( f() == french );

  w.french().revolution.status = e_revolution_status::won;
  REQUIRE( f() == french );

  w.french().control = e_player_control::ai;
  REQUIRE( f() == nothing );
}

TEST_CASE( "[declare] can_declare_independence" ) {
  world w;

  Player& player       = w.add_player( e_player::english );
  Player& other_player = w.add_player( e_player::french );
  Player& ref_player   = w.add_player( e_player::ref_english );

  auto const f = [&]( Player const& player ) {
    return can_declare_independence( w.ss().as_const, player );
  };

  w.settings().game_setup_options.difficulty =
      e_difficulty::discoverer;

  using enum e_declare_rejection;
  using enum e_revolution_status;

  player.revolution.status          = not_declared;
  player.revolution.rebel_sentiment = 50;
  other_player.revolution.status    = not_declared;
  REQUIRE( f( player ) == valid );

  SECTION( "default: yes" ) { REQUIRE( f( player ) == valid ); }

  SECTION( "rebel_sentiment_too_low" ) {
    player.revolution.rebel_sentiment = 0;
    REQUIRE( f( player ) == rebel_sentiment_too_low );

    player.revolution.rebel_sentiment = 10;
    REQUIRE( f( player ) == rebel_sentiment_too_low );

    player.revolution.rebel_sentiment = 30;
    REQUIRE( f( player ) == rebel_sentiment_too_low );

    player.revolution.rebel_sentiment = 49;
    REQUIRE( f( player ) == rebel_sentiment_too_low );

    player.revolution.rebel_sentiment = 50;
    REQUIRE( f( player ) == valid );

    player.revolution.rebel_sentiment = 60;
    REQUIRE( f( player ) == valid );

    player.revolution.rebel_sentiment = 99;
    REQUIRE( f( player ) == valid );

    player.revolution.rebel_sentiment = 100;
    REQUIRE( f( player ) == valid );

    w.settings().game_setup_options.difficulty =
        e_difficulty::viceroy;

    player.revolution.rebel_sentiment = 0;
    REQUIRE( f( player ) == rebel_sentiment_too_low );

    player.revolution.rebel_sentiment = 10;
    REQUIRE( f( player ) == rebel_sentiment_too_low );

    player.revolution.rebel_sentiment = 30;
    REQUIRE( f( player ) == rebel_sentiment_too_low );

    player.revolution.rebel_sentiment = 49;
    REQUIRE( f( player ) == rebel_sentiment_too_low );

    player.revolution.rebel_sentiment = 50;
    REQUIRE( f( player ) == valid );

    player.revolution.rebel_sentiment = 60;
    REQUIRE( f( player ) == valid );

    player.revolution.rebel_sentiment = 99;
    REQUIRE( f( player ) == valid );

    player.revolution.rebel_sentiment = 100;
    REQUIRE( f( player ) == valid );
  }

  SECTION( "already_declared" ) {
    player.revolution.status = declared;
    REQUIRE( f( player ) == already_declared );
  }

  SECTION( "other_human_already_declared" ) {
    other_player.revolution.status = declared;
    REQUIRE( f( player ) == other_human_already_declared );
  }

  SECTION( "ref_cannot_declare" ) {
    REQUIRE( f( ref_player ) == ref_cannot_declare );
  }

  SECTION( "already_one" ) {
    player.revolution.status = won;
    REQUIRE( f( player ) == already_won );
  }
}

TEST_CASE( "[declare] show_declare_rejection_msg" ) {
  world w;
  Player& player = w.default_player();

  using enum e_declare_rejection;

  auto const f = [&]( e_declare_rejection const reason ) {
    co_await_test( show_declare_rejection_msg(
        w.ss(), player, w.gui(), reason ) );
  };

  auto const expect_msg = [&]( string const& msg ) {
    w.gui().EXPECT__message_box( StrContains( msg ) );
  };

  expect_msg( "[Rebel Sentiment] in the New World is too low" );
  f( rebel_sentiment_too_low );

  expect_msg( "already fighting" );
  f( already_declared );

  expect_msg( "another human-controlled player has already" );
  f( other_human_already_declared );

  expect_msg( "already won" );
  f( already_won );

  expect_msg( "Royal Expeditionary Force cannot declare" );
  f( ref_cannot_declare );
}

TEST_CASE( "[declare] ask_declare" ) {
  world w;

  w.add_player( e_player::french );

  auto const f = [&] {
    return co_await_test( ask_declare( w.gui(), w.french() ) );
  };

  w.gui().EXPECT__choice( _ ).returns( nothing );
  REQUIRE( f() == ui::e_confirm::no );

  w.gui().EXPECT__choice( _ ).returns( "no" );
  REQUIRE( f() == ui::e_confirm::no );

  w.gui().EXPECT__choice( _ ).returns( "yes" );
  REQUIRE( f() == ui::e_confirm::yes );
}

TEST_CASE( "[declare] declare_independence_ui_sequence_pre" ) {
  world w;
  Player const& player = w.default_player();

  auto const f = [&] {
    co_await_test( declare_independence_ui_sequence_pre(
        w.ss(), w.ts(), player ) );
  };

  auto const expect_msg = [&]( string const& msg ) {
    w.gui().EXPECT__message_box( StrContains( msg ) );
  };

  expect_msg( "(signing of signature on declaration)" );

  f();
}

TEST_CASE( "[declare] declare_independence_ui_sequence_post" ) {
  world w;
  DeclarationResult decl_res;

  Player const& player = w.default_player();

  auto const f = [&] {
    co_await_test( declare_independence_ui_sequence_post(
        w.ss(), w.ts(), player, decl_res ) );
  };

  auto const expect_msg = [&]( string const& msg ) {
    w.gui().EXPECT__message_box( StrContains( msg ) );
  };

  using enum e_unit_type;

  decl_res = { .seized_ships     = { { privateer, 2 },
                                     { merchantman, 3 },
                                     { galleon, 1 } },
               .offboarded_units = true };

  expect_msg( "signs [Declaration of Independence]" );
  expect_msg( "seized [three Merchantmen]" );
  expect_msg( "seized [one Galleon]" );
  expect_msg( "seized [two Privateers]" );
  expect_msg( "ships in our colonies have offboarded" );

  f();
}

TEST_CASE( "[declare] post_declaration_turn" ) {
  world w;

  Player& player = w.default_player();

  auto const f = [&] { return post_declaration_turn( player ); };

  using enum e_turn_after_declaration;
  using enum e_revolution_status;

  REQUIRE( f() == zero );
  REQUIRE( f() == zero );

  player.revolution.status = declared;
  REQUIRE( f() == one );
  REQUIRE( f() == one );

  player.revolution.continental_army_mobilized = true;
  REQUIRE( f() == two );
  REQUIRE( f() == two );

  player.revolution.gave_independence_war_hints = true;
  REQUIRE( f() == done );
  REQUIRE( f() == done );

  player.revolution.continental_army_mobilized = false;
  REQUIRE( f() == one );
  REQUIRE( f() == one );

  player.revolution.status = not_declared;
  REQUIRE( f() == zero );
  REQUIRE( f() == zero );
}

TEST_CASE( "[declare] declare_independence" ) {
  world w;
  DeclarationResult expected;

  Player& player = w.add_player( e_player::english );
  e_player const ref_player_type =
      ref_player_for( nation_for( player.type ) );
  // This is so that it will add one man-o-war.
  player.control = e_player_control::human;
  player.revolution.expeditionary_force.regular = 1;
  player.bells                                  = 5;
  player.crosses                                = 6;
  player.fathers.in_progress =
      e_founding_father::benjamin_franklin;
  player.fathers.pool[e_founding_father_type::military] =
      e_founding_father::hernan_cortes;
  UnitId const unit_1 =
      w.add_unit_on_map( e_unit_type::free_colonist,
                         { .x = 2, .y = 0 }, player.type )
          .id();
  UnitId const unit_2 =
      w.add_unit_on_map( e_unit_type::free_colonist,
                         { .x = 1, .y = 1 }, player.type )
          .id();
  UnitId const unit_3 =
      w.add_unit_in_port( e_unit_type::free_colonist,
                          player.type )
          .id();
  UnitId const unit_4 =
      w.add_unit_in_port( e_unit_type::caravel, player.type )
          .id();
  UnitId const unit_5 =
      w.add_unit_in_port( e_unit_type::galleon, player.type )
          .id();
  UnitId const unit_6 =
      w.add_unit_in_port( e_unit_type::galleon, player.type )
          .id();
  unit_sail_to_new_world( w.ss(), unit_6 );
  BASE_CHECK( as_const( w.units() )
                  .ownership_of( unit_6 )
                  .get_if<UnitOwnership::harbor>()
                  .member( &UnitOwnership::harbor::port_status )
                  .holds<PortStatus::outbound>(),
              "wrong unit ownership: {}",
              as_const( w.units() ).ownership_of( unit_6 ) );
  Colony const& colony =
      w.add_colony( { .x = 2, .y = 0 }, player.type );
  UnitId const unit_7 =
      w.add_unit_on_map( e_unit_type::merchantman,
                         colony.location, player.type )
          .id();
  UnitId const unit_8 =
      w.add_unit_in_cargo( e_unit_type::soldier, unit_7 ).id();
  BASE_CHECK(
      as_const( w.units() )
              .ownership_of( unit_8 )
              .inner_if<UnitOwnership::cargo>() == unit_7,
      "wrong unit ownership: {}",
      as_const( w.units() ).ownership_of( unit_8 ) );
  ColonyId const colony_id = colony.id;
  UnitId const unit_in_colony =
      w.add_unit_indoors( colony_id, e_indoor_job::bells ).id();

  Player& other_player = w.add_player( e_player::french );
  other_player.control = e_player_control::ai;
  UnitId const foreign_unit_1 =
      w.add_unit_on_map( e_unit_type::free_colonist,
                         { .x = 2, .y = 0 }, other_player.type )
          .id();
  UnitId const foreign_unit_2 =
      w.add_unit_on_map( e_unit_type::free_colonist,
                         { .x = 1, .y = 1 }, other_player.type )
          .id();
  UnitId const foreign_unit_3 =
      w.add_unit_in_port( e_unit_type::free_colonist,
                          other_player.type )
          .id();
  Colony const& foreign_colony =
      w.add_colony( { .x = 2, .y = 2 }, other_player.type );
  ColonyId const foreign_colony_id = foreign_colony.id;
  UnitId const foreign_unit_in_colony =
      w.add_unit_indoors( foreign_colony_id,
                          e_indoor_job::bells )
          .id();

  e_tribe const tribe_type = e_tribe::aztec;
  w.add_tribe( tribe_type );

  auto const f = [&] {
    return declare_independence( w.engine(), w.ss(), w.ts(),
                                 player );
  };

  // BEFORE: sanity check.

  w.settings().game_setup_options.difficulty =
      e_difficulty::conquistador;

  REQUIRE( player.revolution.status ==
           e_revolution_status::not_declared );
  REQUIRE( player.revolution.intervention_force ==
           InterventionForce{} );
  REQUIRE( !w.players().players[ref_player_type].has_value() );
  REQUIRE( !w.terrain()
                .player_terrain( ref_player_type )
                .has_value() );
  REQUIRE_FALSE( w.ss()
                     .natives.tribe_for( tribe_type )
                     .relationship[ref_player_type]
                     .encountered );
  REQUIRE( player.revolution.expeditionary_force ==
           ExpeditionaryForce{ .regular = 1 } );
  REQUIRE( w.units().exists( unit_1 ) );
  REQUIRE( w.units().exists( unit_2 ) );
  REQUIRE( w.units().exists( unit_3 ) );
  REQUIRE( w.units().exists( unit_4 ) );
  REQUIRE( w.units().exists( unit_5 ) );
  REQUIRE( w.units().exists( unit_6 ) );
  REQUIRE( w.units().exists( unit_7 ) );
  REQUIRE( as_const( w.units() )
               .ownership_of( unit_8 )
               .inner_if<UnitOwnership::cargo>() == unit_7 );
  REQUIRE_FALSE(
      w.units().unit_for( unit_1 ).mv_pts_exhausted() );
  REQUIRE_FALSE(
      w.units().unit_for( unit_2 ).mv_pts_exhausted() );
  REQUIRE_FALSE(
      w.units().unit_for( unit_7 ).mv_pts_exhausted() );
  REQUIRE_FALSE(
      w.units().unit_for( unit_8 ).mv_pts_exhausted() );
  REQUIRE( w.units().exists( unit_in_colony ) );
  REQUIRE( w.colonies().exists( colony_id ) );
  REQUIRE( w.units().exists( foreign_unit_1 ) );
  REQUIRE( w.units().exists( foreign_unit_2 ) );
  REQUIRE( w.units().exists( foreign_unit_3 ) );
  REQUIRE( w.units().exists( foreign_unit_in_colony ) );
  REQUIRE( w.colonies().exists( foreign_colony_id ) );
  REQUIRE( player.control == e_player_control::human );
  REQUIRE( other_player.control == e_player_control::ai );
  REQUIRE( player.revolution.expeditionary_force.regular == 1 );
  REQUIRE( player.bells == 5 );
  REQUIRE( player.crosses == 6 );
  REQUIRE( player.fathers.in_progress ==
           e_founding_father::benjamin_franklin );
  REQUIRE(
      player.fathers.pool[e_founding_father_type::military] ==
      e_founding_father::hernan_cortes );

  // ***********
  expected = DeclarationResult{
    .seized_ships =
        {
          { e_unit_type::caravel, 1 },
          { e_unit_type::galleon, 2 },
        },
    .offboarded_units = true,
  };
  REQUIRE( f() == expected );
  // ***********

  // AFTER.

  REQUIRE( player.revolution.status ==
           e_revolution_status::declared );
  REQUIRE( player.revolution.intervention_force ==
           InterventionForce{ .continental_army    = 3,
                              .continental_cavalry = 2,
                              .artillery           = 2,
                              .man_o_war           = 2 } );
  REQUIRE( w.players().players[ref_player_type].has_value() );
  Player const& ref_player = w.player( ref_player_type );
  REQUIRE( ref_player.type == ref_player_type );
  REQUIRE( ref_player.nation == nation_for( ref_player_type ) );
  REQUIRE( w.terrain()
               .player_terrain( ref_player_type )
               .has_value() );
  REQUIRE( ref_player.new_world_name == "The New World" );
  for( auto& [woodcut, done] : ref_player.woodcuts ) {
    INFO( format( "woodcut: {}", woodcut ) );
    REQUIRE( done );
  }
  REQUIRE( w.ss()
               .natives.tribe_for( tribe_type )
               .relationship[ref_player_type]
               .encountered );
  REQUIRE( player.revolution.expeditionary_force ==
           ExpeditionaryForce{ .regular = 1, .man_o_war = 1 } );
  REQUIRE( w.units().exists( unit_1 ) );
  REQUIRE( w.units().exists( unit_2 ) );
  REQUIRE_FALSE( w.units().exists( unit_3 ) );
  REQUIRE_FALSE( w.units().exists( unit_4 ) );
  REQUIRE_FALSE( w.units().exists( unit_5 ) );
  REQUIRE_FALSE( w.units().exists( unit_6 ) );
  REQUIRE( w.units().exists( unit_7 ) );
  REQUIRE( as_const( w.units() )
               .ownership_of( unit_8 )
               .inner_if<UnitOwnership::world>() ==
           colony.location ); // offboarded unit.
  REQUIRE( w.units().unit_for( unit_1 ).mv_pts_exhausted() );
  REQUIRE( w.units().unit_for( unit_2 ).mv_pts_exhausted() );
  REQUIRE( w.units().unit_for( unit_7 ).mv_pts_exhausted() );
  REQUIRE( w.units().unit_for( unit_8 ).mv_pts_exhausted() );
  REQUIRE( w.units().exists( unit_in_colony ) );
  REQUIRE( w.colonies().exists( colony_id ) );
  REQUIRE_FALSE( w.units().exists( foreign_unit_1 ) );
  REQUIRE_FALSE( w.units().exists( foreign_unit_2 ) );
  REQUIRE_FALSE( w.units().exists( foreign_unit_3 ) );
  REQUIRE( w.units().exists( foreign_unit_in_colony ) );
  REQUIRE( w.colonies().exists( foreign_colony_id ) );
  REQUIRE( player.control == e_player_control::human );
  REQUIRE( ref_player.control == e_player_control::ai );
  REQUIRE( other_player.control == e_player_control::inactive );
  REQUIRE( player.bells == 0 );
  REQUIRE( player.crosses == 0 );
  REQUIRE( player.fathers.in_progress == nothing );
  REQUIRE(
      player.fathers.pool[e_founding_father_type::military] ==
      nothing );
}

} // namespace
} // namespace rn
