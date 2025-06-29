/****************************************************************
**treasure.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-17.
*
* Description: Unit tests for the src/treasure.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/treasure.hpp"

// Testing
#include "test/fake/world.hpp"
#include "test/mocks/igui.hpp"
#include "test/mocks/irand.hpp"

// Revolution Now
#include "src/mock/matchers.hpp"

// ss
#include "ss/dwelling.rds.hpp"
#include "ss/natives.hpp"
#include "ss/old-world-state.rds.hpp"
#include "ss/player.rds.hpp"
#include "ss/ref.hpp"
#include "ss/settings.hpp"
#include "ss/units.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

using ::mock::matchers::_;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() {
    add_default_player();
    create_default_map();
  }

  Coord const kColonySquare = Coord{ .x = 1, .y = 1 };

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
TEST_CASE( "[treasure] treasure_in_harbor_receipt" ) {
  World W;
  Player& player = W.default_player();
  Unit& unit     = W.add_unit_on_map(
      UnitComposition::create(
          e_unit_type::treasure,
          { { e_unit_inventory::gold, 100 } } )
          .value(),
      { .x = 1, .y = 1 } );
  TreasureReceipt expected;
  int& tax_rate = W.old_world( player ).taxes.tax_rate;

  auto f = [&] {
    return treasure_in_harbor_receipt(
        W.ss(), W.default_player(), unit );
  };

  tax_rate = 0;
  expected = {
    .treasure_id       = UnitId{ 1 },
    .transport_mode    = e_treasure_transport_mode::player,
    .original_worth    = 100,
    .kings_cut_percent = 0,
    .net_received      = 100 };
  REQUIRE( f() == expected );

  tax_rate = 10;
  expected = {
    .treasure_id       = UnitId{ 1 },
    .transport_mode    = e_treasure_transport_mode::player,
    .original_worth    = 100,
    .kings_cut_percent = 10,
    .net_received      = 90 };
  REQUIRE( f() == expected );

  tax_rate = 99;
  expected = {
    .treasure_id       = UnitId{ 1 },
    .transport_mode    = e_treasure_transport_mode::player,
    .original_worth    = 100,
    .kings_cut_percent = 99,
    .net_received      = 1 };
  REQUIRE( f() == expected );

  tax_rate = 100;
  expected = {
    .treasure_id       = UnitId{ 1 },
    .transport_mode    = e_treasure_transport_mode::player,
    .original_worth    = 100,
    .kings_cut_percent = 100,
    .net_received      = 0 };
  REQUIRE( f() == expected );

  tax_rate = 110;
  expected = {
    .treasure_id       = UnitId{ 1 },
    .transport_mode    = e_treasure_transport_mode::player,
    .original_worth    = 100,
    .kings_cut_percent = 110,
    .net_received      = 0 };
  REQUIRE( f() == expected );
}

TEST_CASE( "[treasure] apply_treasure_reimbursement" ) {
  World W;
  Player& player = W.default_player();
  W.add_unit_on_map( UnitComposition::create(
                         e_unit_type::treasure,
                         { { e_unit_inventory::gold, 100 } } )
                         .value(),
                     { .x = 1, .y = 1 } );
  W.add_unit_on_map( UnitComposition::create(
                         e_unit_type::treasure,
                         { { e_unit_inventory::gold, 200 } } )
                         .value(),
                     { .x = 1, .y = 1 } );

  player.royal_money = 100;

  REQUIRE( W.units().all().size() == 2 );
  REQUIRE( player.money == 0 );
  REQUIRE( player.royal_money == 100 );

  {
    TreasureReceipt const receipt{
      .treasure_id       = UnitId{ 1 },
      .transport_mode    = e_treasure_transport_mode::player,
      .original_worth    = 100,
      .kings_cut_percent = 10,
      .net_received      = 90 };
    apply_treasure_reimbursement( W.ss(), player, receipt );
    REQUIRE( W.units().all().size() == 1 );
    REQUIRE( player.money == 90 );
    REQUIRE( player.total_after_tax_revenue == 90 );
    REQUIRE( player.royal_money == 110 );
  }

  player.money = 30;

  {
    TreasureReceipt const receipt{
      .treasure_id       = UnitId{ 2 },
      .transport_mode    = e_treasure_transport_mode::player,
      .original_worth    = 200,
      .kings_cut_percent = 10,
      .net_received      = 180 };
    apply_treasure_reimbursement( W.ss(), player, receipt );
    REQUIRE( W.units().all().size() == 0 );
    REQUIRE( player.money == 30 + 180 );
    REQUIRE( player.total_after_tax_revenue == 90 + 180 );
    REQUIRE( player.royal_money == 130 );
  }
}

TEST_CASE( "[treasure] show_treasure_receipt" ) {
  World W;
  Player const& player = W.default_player();
  TreasureReceipt receipt;
  string msg;

  auto f = [&] {
    wait<> w = show_treasure_receipt( W.ts(), player, receipt );
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
  };

  receipt = {
    .treasure_id       = UnitId{ 1 },
    .transport_mode    = e_treasure_transport_mode::player,
    .original_worth    = 100,
    .kings_cut_percent = 10,
    .net_received      = 90 };
  msg =
      "Treasure worth 100\x7f reimbursed in Amsterdam yielding "
      "[90\x7f] after 10% taxes witheld.";
  W.gui().EXPECT__message_box( msg ).returns( monostate{} );
  f();

  receipt = { .treasure_id = UnitId{ 1 },
              .transport_mode =
                  e_treasure_transport_mode::king_with_charge,
              .original_worth    = 100,
              .kings_cut_percent = 10,
              .net_received      = 90 };
  msg =
      "Treasure worth 100\x7f arrives in Amsterdam!  The crown "
      "has provided a reimbursement of [90\x7f] after a [10%] "
      "witholding.";
  W.gui().EXPECT__message_box( msg ).returns( monostate{} );
  f();

  receipt = {
    .treasure_id = UnitId{ 1 },
    .transport_mode =
        e_treasure_transport_mode::king_no_extra_charge,
    .original_worth    = 100,
    .kings_cut_percent = 10,
    .net_received      = 90 };
  msg =
      "Treasure worth 100\x7f arrives in Amsterdam!  The crown "
      "has provided a reimbursement of [90\x7f] after a [10%] "
      "tax witholding.";
  W.gui().EXPECT__message_box( msg ).returns( monostate{} );
  f();
}

TEST_CASE( "[treasure] treasure_enter_colony" ) {
  World W;
  Player& player = W.default_player();
  Unit& unit     = W.add_unit_on_map(
      UnitComposition::create(
          e_unit_type::treasure,
          { { e_unit_inventory::gold, 100 } } )
          .value(),
      { .x = 1, .y = 1 } );
  W.old_world( player ).taxes.tax_rate = 7;
  maybe<TreasureReceipt> expected;
  ChoiceConfig config;
  string msg;

  auto f = [&] {
    wait<maybe<TreasureReceipt>> w = treasure_enter_colony(
        W.ss(), W.ts(), W.default_player(), unit );
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    return *w;
  };

  // No galleons.
  expected = TreasureReceipt{
    .treasure_id = UnitId{ 1 },
    .transport_mode =
        e_treasure_transport_mode::king_with_charge,
    .original_worth    = 100,
    .kings_cut_percent = 50,
    .net_received      = 50 };
  msg =
      "The crown is happy to see the bounty that you've "
      "acquired. Seeing that you don't have a fleet of Galleons "
      "with which to transport this treasure to Amsterdam we "
      "will happily transport it for you, after which we will "
      "make an assessment of the appropriate percentage to "
      "withhold as compensation.";
  config = ChoiceConfig{
    .msg     = msg,
    .options = { ChoiceConfigOption{ .key          = "yes",
                                     .display_name = "Accept." },
                 ChoiceConfigOption{
                   .key = "no", .display_name = "Decline." } } };
  W.gui().EXPECT__choice( config ).returns<maybe<string>>(
      "yes" );
  REQUIRE( f() == expected );

  // No galleons, with hernan cortes.
  player.fathers.has[e_founding_father::hernan_cortes] = true;
  expected = TreasureReceipt{
    .treasure_id = UnitId{ 1 },
    .transport_mode =
        e_treasure_transport_mode::king_no_extra_charge,
    .original_worth    = 100,
    .kings_cut_percent = 7,
    .net_received      = 93 };
  msg =
      "The crown is happy to see the bounty that you've "
      "acquired. Seeing that you don't have a fleet of Galleons "
      "with which to transport this treasure to Amsterdam we "
      "will happily transport it for you, and we will do so for "
      "[no extra charge], only withholding an amount "
      "determined by the current tax rate.";
  config = ChoiceConfig{
    .msg     = msg,
    .options = { ChoiceConfigOption{ .key          = "yes",
                                     .display_name = "Accept." },
                 ChoiceConfigOption{
                   .key = "no", .display_name = "Decline." } } };
  W.gui().EXPECT__choice( config ).returns<maybe<string>>(
      "yes" );
  REQUIRE( f() == expected );

  // Chooses no.
  W.gui().EXPECT__choice( _ ).returns<maybe<string>>( "no" );
  REQUIRE( f() == nothing );

  W.settings().game_setup_options.difficulty =
      e_difficulty::viceroy;
  W.add_unit_on_map( e_unit_type::galleon, { .x = 0, .y = 0 } );
  player.fathers.has[e_founding_father::hernan_cortes] = false;

  // With Galleons, without Cortes.
  REQUIRE( f() == nothing );

  // With Galleons, with Cortes.
  player.fathers.has[e_founding_father::hernan_cortes] = true;
  expected = TreasureReceipt{
    .treasure_id = UnitId{ 1 },
    .transport_mode =
        e_treasure_transport_mode::king_no_extra_charge,
    .original_worth    = 100,
    .kings_cut_percent = 7,
    .net_received      = 93 };
  msg =
      "The crown is happy to see the bounty that you've "
      "acquired. Because we don't want you to be burdened by "
      "having to transport this treasure to Amsterdam we will "
      "happily transport it for you, and we will do so for [no "
      "extra charge], only withholding an amount determined by "
      "the current tax rate.";
  config = ChoiceConfig{
    .msg     = msg,
    .options = { ChoiceConfigOption{ .key          = "yes",
                                     .display_name = "Accept." },
                 ChoiceConfigOption{
                   .key = "no", .display_name = "Decline." } } };
  W.gui().EXPECT__choice( config ).returns<maybe<string>>(
      "yes" );
  REQUIRE( f() == expected );

  // After declaration.
  player.revolution.status = e_revolution_status::declared;

  expected = TreasureReceipt{
    .treasure_id = UnitId{ 1 },
    .transport_mode =
        e_treasure_transport_mode::traveling_merchants,
    .original_worth    = 100,
    .kings_cut_percent = 0,
    .net_received      = 100 };
  REQUIRE( f() == expected );
}

TEST_CASE( "[treasure] treasure_from_dwelling" ) {
  World W;
  e_tribe tribe   = {};
  Player& player  = W.default_player();
  bool has_cortes = false;
  bool capital    = false;
  maybe<int> expected;

  auto f = [&] {
    Dwelling& dwelling =
        W.add_dwelling( { .x = 1, .y = 1 }, tribe );
    dwelling.is_capital = capital;
    player.fathers.has[e_founding_father::hernan_cortes] =
        has_cortes;
    maybe<int> const res = treasure_from_dwelling(
        W.ss(), W.rand(), player, dwelling );
    W.natives().destroy_dwelling( dwelling.id );
    return res;
  };

  // Semi-nomadic, no capital, no cortes, no treasure.
  tribe      = e_tribe::tupi;
  has_cortes = false;
  capital    = false;
  W.rand().EXPECT__bernoulli( .25 ).returns( false );
  expected = nothing;
  REQUIRE( f() == expected );

  // Semi-nomadic, no capital, no cortes, yes treasure.
  tribe      = e_tribe::tupi;
  has_cortes = false;
  capital    = false;
  W.rand().EXPECT__bernoulli( .25 ).returns( true );
  W.rand().EXPECT__between_ints( 200, 400 ).returns( 315 );
  expected = 300;
  REQUIRE( f() == expected );

  // Semi-nomadic, no capital, yes cortes.
  tribe      = e_tribe::tupi;
  has_cortes = true;
  capital    = false;
  W.rand().EXPECT__between_ints( 200, 400 ).returns( 395 );
  expected = 500;
  REQUIRE( f() == expected );

  // Semi-nomadic, yes capital, no cortes.
  tribe      = e_tribe::tupi;
  has_cortes = false;
  capital    = true;
  W.rand().EXPECT__between_ints( 200, 400 ).returns( 395 );
  expected = 700;
  REQUIRE( f() == expected );

  // Agrarian, no capital, no cortes, yes treasure.
  tribe      = e_tribe::cherokee;
  has_cortes = false;
  capital    = false;
  W.rand().EXPECT__bernoulli( .33 ).returns( true );
  W.rand().EXPECT__between_ints( 300, 800 ).returns( 675 );
  expected = 600;
  REQUIRE( f() == expected );

  // Advanced, no capital, no cortes, yes treasure.
  tribe      = e_tribe::aztec;
  has_cortes = false;
  capital    = false;
  W.rand().EXPECT__bernoulli( 1.0 ).returns( true );
  W.rand().EXPECT__between_ints( 2000, 6000 ).returns( 5123 );
  expected = 5100;
  REQUIRE( f() == expected );

  // Civilized, no capital, no cortes, yes treasure.
  tribe      = e_tribe::inca;
  has_cortes = false;
  capital    = false;
  W.rand().EXPECT__bernoulli( 1.0 ).returns( true );
  W.rand().EXPECT__between_ints( 3000, 10000 ).returns( 8123 );
  expected = 8100;
  REQUIRE( f() == expected );

  // Civilized, no capital, with cortes. (viceroy).
  tribe      = e_tribe::inca;
  has_cortes = true;
  capital    = false;
  W.settings().game_setup_options.difficulty =
      e_difficulty::viceroy;
  W.rand().EXPECT__between_ints( 3000, 10000 ).returns( 8123 );
  expected = 12100;
  REQUIRE( f() == expected );

  // Civilized, capital, with cortes. (governor).
  tribe      = e_tribe::inca;
  has_cortes = true;
  capital    = true;
  W.settings().game_setup_options.difficulty =
      e_difficulty::governor;
  W.rand().EXPECT__between_ints( 3000, 10000 ).returns( 8123 );
  expected = 24300;
  REQUIRE( f() == expected );
}

} // namespace
} // namespace rn
