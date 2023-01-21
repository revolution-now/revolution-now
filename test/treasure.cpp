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
#include "ss/player.rds.hpp"
#include "ss/ref.hpp"
#include "ss/settings.hpp"
#include "ss/unit-composer.hpp"
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
  World   W;
  Player& player = W.default_player();
  Unit&   unit   = W.add_unit_on_map(
      UnitComposition::create(
          UnitType::create( e_unit_type::treasure ),
          { { e_unit_inventory::gold, 100 } } )
          .value(),
      { .x = 1, .y = 1 } );
  TreasureReceipt expected;
  int&            tax_rate = player.old_world.taxes.tax_rate;

  auto f = [&] {
    return treasure_in_harbor_receipt( W.default_player(),
                                       unit );
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
  World   W;
  Player& player = W.default_player();
  W.add_unit_on_map(
      UnitComposition::create(
          UnitType::create( e_unit_type::treasure ),
          { { e_unit_inventory::gold, 100 } } )
          .value(),
      { .x = 1, .y = 1 } );

  REQUIRE( W.units().all().size() == 1 );
  REQUIRE( player.money == 0 );

  TreasureReceipt const receipt{
      .treasure_id       = UnitId{ 1 },
      .transport_mode    = e_treasure_transport_mode::player,
      .original_worth    = 100,
      .kings_cut_percent = 10,
      .net_received      = 90 };
  apply_treasure_reimbursement( W.ss(), player, receipt );

  REQUIRE( W.units().all().size() == 0 );
  REQUIRE( player.money == 90 );
}

TEST_CASE( "[treasure] show_treasure_receipt" ) {
  World           W;
  Player const&   player = W.default_player();
  TreasureReceipt receipt;
  string          msg;

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
      "Treasure worth 100 reimbursed in Amsterdam yielding "
      "@[H]90@[] after 10% taxes witheld.";
  EXPECT_CALL( W.gui(), message_box( msg ) )
      .returns( monostate{} );
  f();

  receipt = { .treasure_id = UnitId{ 1 },
              .transport_mode =
                  e_treasure_transport_mode::king_with_charge,
              .original_worth    = 100,
              .kings_cut_percent = 10,
              .net_received      = 90 };
  msg =
      "Treasure worth 100 arrives in Amsterdam!  The crown has "
      "provided a reimbursement of @[H]90@[] after a @[H]10%@[] "
      "witholding.";
  EXPECT_CALL( W.gui(), message_box( msg ) )
      .returns( monostate{} );
  f();

  receipt = {
      .treasure_id = UnitId{ 1 },
      .transport_mode =
          e_treasure_transport_mode::king_no_extra_charge,
      .original_worth    = 100,
      .kings_cut_percent = 10,
      .net_received      = 90 };
  msg =
      "Treasure worth 100 arrives in Amsterdam!  The crown has "
      "provided a reimbursement of @[H]90@[] after a @[H]10%@[] "
      "tax witholding.";
  EXPECT_CALL( W.gui(), message_box( msg ) )
      .returns( monostate{} );
  f();
}

#ifndef COMPILER_GCC
TEST_CASE( "[treasure] treasure_enter_colony" ) {
  World   W;
  Player& player = W.default_player();
  Unit&   unit   = W.add_unit_on_map(
      UnitComposition::create(
          UnitType::create( e_unit_type::treasure ),
          { { e_unit_inventory::gold, 100 } } )
          .value(),
      { .x = 1, .y = 1 } );
  player.old_world.taxes.tax_rate = 7;
  maybe<TreasureReceipt> expected;
  ChoiceConfig           config;
  string                 msg;

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
      .options = {
          ChoiceConfigOption{ .key          = "yes",
                              .display_name = "Accept." },
          ChoiceConfigOption{ .key          = "no",
                              .display_name = "Decline." } } };
  EXPECT_CALL( W.gui(), choice( config, e_input_required::no ) )
      .returns<maybe<string>>( "yes" );
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
      "@[H]no extra charge@[], only withholding an amount "
      "determined by the current tax rate.";
  config = ChoiceConfig{
      .msg     = msg,
      .options = {
          ChoiceConfigOption{ .key          = "yes",
                              .display_name = "Accept." },
          ChoiceConfigOption{ .key          = "no",
                              .display_name = "Decline." } } };
  EXPECT_CALL( W.gui(), choice( config, e_input_required::no ) )
      .returns<maybe<string>>( "yes" );
  REQUIRE( f() == expected );

  // Chooses no.
  EXPECT_CALL( W.gui(), choice( _, e_input_required::no ) )
      .returns<maybe<string>>( "no" );
  REQUIRE( f() == nothing );

  W.settings().difficulty = e_difficulty::viceroy;
  W.add_unit_on_map( e_unit_type::galleon, { .x = 0, .y = 0 } );
  player.fathers.has[e_founding_father::hernan_cortes] = false;
  expected = TreasureReceipt{
      .treasure_id = UnitId{ 1 },
      .transport_mode =
          e_treasure_transport_mode::king_with_charge,
      .original_worth    = 100,
      .kings_cut_percent = 70,
      .net_received      = 30 };
  msg =
      "The crown is happy to see the bounty that you've "
      "acquired. Because we don't want you to be burdened by "
      "having to transport this treasure to Amsterdam we will "
      "happily transport it for you, after which we will make "
      "an assessment of the appropriate percentage to withhold "
      "as compensation.";
  config = ChoiceConfig{
      .msg     = msg,
      .options = {
          ChoiceConfigOption{ .key          = "yes",
                              .display_name = "Accept." },
          ChoiceConfigOption{ .key          = "no",
                              .display_name = "Decline." } } };
  EXPECT_CALL( W.gui(), choice( config, e_input_required::no ) )
      .returns<maybe<string>>( "yes" );
  REQUIRE( f() == expected );
}
#endif

TEST_CASE( "[treasure] treasure_from_dwelling" ) {
  World     W;
  e_tribe   tribe = {};
  Dwelling& dwelling =
      W.add_dwelling( { .x = 1, .y = 1 }, tribe );
  Player&    player     = W.default_player();
  bool       has_cortes = false;
  bool       capital    = false;
  maybe<int> expected;

  auto f = [&] {
    dwelling.is_capital = capital;
    dwelling.tribe      = tribe;
    player.fathers.has[e_founding_father::hernan_cortes] =
        has_cortes;
    return treasure_from_dwelling( W.ts(), player, dwelling );
  };

  // Semi-nomadic, no capital, no cortes, no treasure.
  tribe      = e_tribe::tupi;
  has_cortes = false;
  capital    = false;
  EXPECT_CALL( W.rand(), bernoulli( .25 ) ).returns( false );
  expected = nothing;
  REQUIRE( f() == expected );

  // Semi-nomadic, no capital, no cortes, yes treasure.
  tribe      = e_tribe::tupi;
  has_cortes = false;
  capital    = false;
  EXPECT_CALL( W.rand(), bernoulli( .25 ) ).returns( true );
  EXPECT_CALL( W.rand(),
               between_ints( 200, 400, e_interval::closed ) )
      .returns( 315 );
  expected = 300;
  REQUIRE( f() == expected );

  // Semi-nomadic, no capital, yes cortes.
  tribe      = e_tribe::tupi;
  has_cortes = true;
  capital    = false;
  EXPECT_CALL( W.rand(),
               between_ints( 200, 400, e_interval::closed ) )
      .returns( 395 );
  expected = 500;
  REQUIRE( f() == expected );

  // Semi-nomadic, yes capital, no cortes.
  tribe      = e_tribe::tupi;
  has_cortes = false;
  capital    = true;
  EXPECT_CALL( W.rand(),
               between_ints( 200, 400, e_interval::closed ) )
      .returns( 395 );
  expected = 700;
  REQUIRE( f() == expected );

  // Agrarian, no capital, no cortes, yes treasure.
  tribe      = e_tribe::cherokee;
  has_cortes = false;
  capital    = false;
  EXPECT_CALL( W.rand(), bernoulli( .33 ) ).returns( true );
  EXPECT_CALL( W.rand(),
               between_ints( 300, 800, e_interval::closed ) )
      .returns( 675 );
  expected = 600;
  REQUIRE( f() == expected );

  // Advanced, no capital, no cortes, yes treasure.
  tribe      = e_tribe::aztec;
  has_cortes = false;
  capital    = false;
  EXPECT_CALL( W.rand(), bernoulli( 1.0 ) ).returns( true );
  EXPECT_CALL( W.rand(),
               between_ints( 2000, 6000, e_interval::closed ) )
      .returns( 5123 );
  expected = 5100;
  REQUIRE( f() == expected );

  // Civilized, no capital, no cortes, yes treasure.
  tribe      = e_tribe::inca;
  has_cortes = false;
  capital    = false;
  EXPECT_CALL( W.rand(), bernoulli( 1.0 ) ).returns( true );
  EXPECT_CALL( W.rand(),
               between_ints( 3000, 10000, e_interval::closed ) )
      .returns( 8123 );
  expected = 8100;
  REQUIRE( f() == expected );

  // Civilized, no capital, with cortes. (viceroy).
  tribe                   = e_tribe::inca;
  has_cortes              = true;
  capital                 = false;
  W.settings().difficulty = e_difficulty::viceroy;
  EXPECT_CALL( W.rand(),
               between_ints( 3000, 10000, e_interval::closed ) )
      .returns( 8123 );
  expected = 12100;
  REQUIRE( f() == expected );

  // Civilized, capital, with cortes. (governor).
  tribe                   = e_tribe::inca;
  has_cortes              = true;
  capital                 = true;
  W.settings().difficulty = e_difficulty::governor;
  EXPECT_CALL( W.rand(),
               between_ints( 3000, 10000, e_interval::closed ) )
      .returns( 8123 );
  expected = 24300;
  REQUIRE( f() == expected );
}

} // namespace
} // namespace rn
