/****************************************************************
**price-group.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-09-16.
*
* Description: Unit tests for the src/price-group.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/price-group.hpp"

// Testing
#include "test/fake/world.hpp"
#include "test/mocking.hpp"
#include "test/mocks/irand.hpp"

// config
#include "config/market.rds.hpp"

// refl
#include "refl/to-str.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

using PGMap = ProcessedGoodsPriceGroup::Map;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() {
    create_default_map();
    add_player( e_nation::french );
  }

  void create_default_map() {
    MapSquare const   L = make_grassland();
    vector<MapSquare> tiles{ L };
    build_map( std::move( tiles ), 1 );
  }
};

/****************************************************************
** Test scenario configuration.
*****************************************************************/
struct PGStruct {
  int rum    = 0;
  int cigars = 0;
  int cloth  = 0;
  int coats  = 0;

  int get( e_processed_good good ) const {
    switch( good ) {
      case e_processed_good::rum: return rum;
      case e_processed_good::cigars: return cigars;
      case e_processed_good::cloth: return cloth;
      case e_processed_good::coats: return coats;
    }
  }
};

void copy( PGStruct const& l, PGMap& r ) {
  r[e_processed_good::rum]    = l.rum;
  r[e_processed_good::cigars] = l.cigars;
  r[e_processed_good::cloth]  = l.cloth;
  r[e_processed_good::coats]  = l.coats;
}

struct TestCaseConfig {
  enum e_action_type { buy, sell, sell_all };

  struct Action {
    int              count = 0;
    e_action_type    type  = {};
    e_processed_good good  = {};
  };

  struct Step {
    Action   action;
    PGStruct expect_eq;
    PGStruct expect_vol;
  };

  PGStruct     starting_intrinsic_volumes;
  vector<Step> steps;
};

/****************************************************************
** Default model config.
*****************************************************************/
// 11/12  10/11  14/15  9/10
PGStruct const STARTING_11_10_14_9{ .rum    = 0x02a9,
                                    .cigars = 0x02c6,
                                    .cloth  = 0x0224,
                                    .coats  = 0x033c };

// 12/13  9/10  14/15  8/9
PGStruct const STARTING_12_9_14_8{ .rum    = 0x1f3,
                                   .cigars = 0x277,
                                   .cloth  = 0x1c6,
                                   .coats  = 0x2b5 };

#define ACTION_SELL_ALL TestCaseConfig::e_action_type::sell_all
#define ACTION_BUY      TestCaseConfig::e_action_type::buy
#define ACTION_SELL     TestCaseConfig::e_action_type::sell

/****************************************************************
** Scenarios
*****************************************************************/
TestCaseConfig const scenario_0{
    .starting_intrinsic_volumes = { .rum    = 0x0295,
                                    .cigars = 0x02b2,
                                    .cloth  = 0x0214,
                                    .coats  = 0x0324 },

    .steps = { TestCaseConfig::Step{
        .action     = { .count = 30,
                        .type  = ACTION_SELL_ALL,
                        .good  = {} },
        .expect_eq  = { .rum    = 12,
                        .cigars = 11,
                        .cloth  = 12,
                        .coats  = 11 },
        .expect_vol = { .rum    = 0xFBB2,
                        .cigars = 0xFBCB,
                        .cloth  = 0xFBA4,
                        .coats  = 0xFC09 } } } };

TestCaseConfig const scenario_1{
    // 11/12  10/11  14/15  9/10
    .starting_intrinsic_volumes = { .rum    = 0x0295,
                                    .cigars = 0x02b2,
                                    .cloth  = 0x0214,
                                    .coats  = 0x0324 },

    .steps = { { .action     = { .count = 28,
                                 .type  = ACTION_SELL,
                                 .good  = e_processed_good::cloth },
                 .expect_eq  = { .rum    = 20,
                                 .cigars = 20,
                                 .cloth  = 4,
                                 .coats  = 20 },
                 .expect_vol = { .rum    = 0x01DB,
                                 .cigars = 0x01ED,
                                 .cloth  = 0xFEEE,
                                 .coats  = 0x023D } },
               { .action     = { .count = 28,
                                 .type  = ACTION_BUY,
                                 .good  = e_processed_good::cloth },
                 .expect_eq  = { .rum    = 8,
                                 .cigars = 8,
                                 .cloth  = 20,
                                 .coats  = 7 },
                 .expect_vol = { .rum    = 0x0187,
                                 .cigars = 0x0199,
                                 .cloth  = 0xFF94,
                                 .coats  = 0x01D9 } } } };

TestCaseConfig const scenario_2{
    .starting_intrinsic_volumes = STARTING_11_10_14_9,

    .steps = { { .action     = { .count = 18,
                                 .type  = ACTION_SELL,
                                 .good  = e_processed_good::cigars },
                 .expect_eq  = { .rum    = 20,
                                 .cigars = 5,
                                 .cloth  = 20,
                                 .coats  = 16 },
                 .expect_vol = { .rum    = 0x0208,
                                 .cigars = 0x00AC,
                                 .cloth  = 0x01A8,
                                 .coats  = 0x0275 } },
               { .action     = { .count = 12,
                                 .type  = ACTION_BUY,
                                 .good  = e_processed_good::coats },
                 .expect_eq  = { .rum    = 20,
                                 .cigars = 5,
                                 .cloth  = 20,
                                 .coats  = 16 },
                 .expect_vol = { .rum    = 0x0208,
                                 .cigars = 0x00AC,
                                 .cloth  = 0x01A8,
                                 .coats  = 0x0275 } },
               { .action     = { .count = 1,
                                 .type  = ACTION_SELL,
                                 .good  = e_processed_good::rum },
                 .expect_eq  = { .rum    = 17,
                                 .cigars = 5,
                                 .cloth  = 20,
                                 .coats  = 17 },
                 .expect_vol = { .rum    = 0x01F7,
                                 .cigars = 0x009D,
                                 .cloth  = 0x01A5,
                                 .coats  = 0x0271 } },
               { .action     = { .count = 1,
                                 .type  = ACTION_BUY,
                                 .good  = e_processed_good::rum },
                 .expect_eq  = { .rum    = 20,
                                 .cigars = 5,
                                 .cloth  = 20,
                                 .coats  = 16 },
                 .expect_vol = { .rum    = 0x01FF,
                                 .cigars = 0x008E,
                                 .cloth  = 0x01A2,
                                 .coats  = 0x026D } } } };

TestCaseConfig const scenario_3{
    .starting_intrinsic_volumes = STARTING_11_10_14_9,

    .steps = { { .action     = { .count = 6,
                                 .type  = ACTION_SELL,
                                 .good  = e_processed_good::rum },
                 .expect_eq  = { .rum    = 7,
                                 .cigars = 14,
                                 .cloth  = 18,
                                 .coats  = 12 },
                 .expect_vol = { .rum    = 0x01D2,
                                 .cigars = 0x024F,
                                 .cloth  = 0x01CC,
                                 .coats  = 0x02AF } },
               { .action     = { .count = 8,
                                 .type  = ACTION_SELL,
                                 .good  = e_processed_good::cloth },
                 .expect_eq  = { .rum    = 9,
                                 .cigars = 17,
                                 .cloth  = 9,
                                 .coats  = 15 },
                 .expect_vol = { .rum    = 0x0194,
                                 .cigars = 0x022F,
                                 .cloth  = 0x0127,
                                 .coats  = 0x0287 } },
               { .action     = { .count = 3,
                                 .type  = ACTION_SELL,
                                 .good  = e_processed_good::coats },
                 .expect_eq  = { .rum    = 10,
                                 .cigars = 19,
                                 .cloth  = 9,
                                 .coats  = 11 },
                 .expect_vol = { .rum    = 0x017F,
                                 .cigars = 0x0223,
                                 .cloth  = 0x010F,
                                 .coats  = 0x024A } },
               { .action     = { .count = 8,
                                 .type  = ACTION_BUY,
                                 .good  = e_processed_good::rum },
                 .expect_eq  = { .rum    = 19,
                                 .cigars = 16,
                                 .cloth  = 8,
                                 .coats  = 10 },
                 .expect_vol = { .rum    = 0x01BD,
                                 .cigars = 0x020B,
                                 .cloth  = 0x00DF,
                                 .coats  = 0x0226 } },
               { .action     = { .count = 10,
                                 .type  = ACTION_SELL,
                                 .good  = e_processed_good::cigars },
                 .expect_eq  = { .rum    = 20,
                                 .cigars = 7,
                                 .cloth  = 10,
                                 .coats  = 13 },
                 .expect_vol = { .rum    = 0x019F,
                                 .cigars = 0x0136,
                                 .cloth  = 0x0099,
                                 .coats  = 0x01EA } },
               { .action     = { .count = 8,
                                 .type  = ACTION_BUY,
                                 .good  = e_processed_good::cloth },
                 .expect_eq  = { .rum    = 20,
                                 .cigars = 6,
                                 .cloth  = 20,
                                 .coats  = 10 },
                 .expect_vol = { .rum    = 0x0187,
                                 .cigars = 0x00EA,
                                 .cloth  = 0x00F1,
                                 .coats  = 0x01BE } } } };

TestCaseConfig const scenario_4{
    .starting_intrinsic_volumes = STARTING_11_10_14_9,

    .steps = { { .action     = { .count = 6,
                                 .type  = ACTION_SELL,
                                 .good  = e_processed_good::cloth },
                 .expect_eq  = { .rum    = 14,
                                 .cigars = 14,
                                 .cloth  = 8,
                                 .coats  = 12 },
                 .expect_vol = { .rum    = 0x0238,
                                 .cigars = 0x024F,
                                 .cloth  = 0x0166,
                                 .coats  = 0x02AF } },
               { .action     = { .count = 3,
                                 .type  = ACTION_BUY,
                                 .good  = e_processed_good::cloth },
                 .expect_eq  = { .rum    = 13,
                                 .cigars = 12,
                                 .cloth  = 10,
                                 .coats  = 11 },
                 .expect_vol = { .rum    = 0x022C,
                                 .cigars = 0x0243,
                                 .cloth  = 0x0181,
                                 .coats  = 0x02A0 } },
               { .action     = { .count = 12,
                                 .type  = ACTION_SELL,
                                 .good  = e_processed_good::cigars },
                 .expect_eq  = { .rum    = 19,
                                 .cigars = 6,
                                 .cloth  = 15,
                                 .coats  = 15 },
                 .expect_vol = { .rum    = 0x01FC,
                                 .cigars = 0x0133,
                                 .cloth  = 0x0147,
                                 .coats  = 0x0269 } },
               { .action     = { .count = 6,
                                 .type  = ACTION_SELL,
                                 .good  = e_processed_good::cigars },
                 .expect_eq  = { .rum    = 20,
                                 .cigars = 5,
                                 .cloth  = 18,
                                 .coats  = 18 },
                 .expect_vol = { .rum    = 0x01EA,
                                 .cigars = 0x008C,
                                 .cloth  = 0x012F,
                                 .coats  = 0x0251 } },
               { .action     = { .count = 6,
                                 .type  = ACTION_SELL,
                                 .good  = e_processed_good::cloth },
                 .expect_eq  = { .rum    = 20,
                                 .cigars = 6,
                                 .cloth  = 11,
                                 .coats  = 20 },
                 .expect_vol = { .rum    = 0x01D8,
                                 .cigars = 0x0036,
                                 .cloth  = 0x00B3,
                                 .coats  = 0x0239 } },
               { .action     = { .count = 6,
                                 .type  = ACTION_SELL,
                                 .good  = e_processed_good::coats },
                 .expect_eq  = { .rum    = 20,
                                 .cigars = 7,
                                 .cloth  = 12,
                                 .coats  = 12 },
                 .expect_vol = { .rum    = 0x01C6,
                                 .cigars = 0xFFE3,
                                 .cloth  = 0x0083,
                                 .coats  = 0x01BA } },
               { .action     = { .count = 6,
                                 .type  = ACTION_SELL,
                                 .good  = e_processed_good::coats },
                 .expect_eq  = { .rum    = 20,
                                 .cigars = 8,
                                 .cloth  = 14,
                                 .coats  = 9 },
                 .expect_vol = { .rum    = 0x01B4,
                                 .cigars = 0xFF95,
                                 .cloth  = 0x0058,
                                 .coats  = 0x0129 } },
               { .action     = { .count = 18,
                                 .type  = ACTION_BUY,
                                 .good  = e_processed_good::cigars },
                 .expect_eq  = { .rum    = 20,
                                 .cigars = 20,
                                 .cloth  = 9,
                                 .coats  = 5 },
                 .expect_vol = { .rum    = 0x017E,
                                 .cigars = 0x002F,
                                 .cloth  = 0xFFDE,
                                 .coats  = 0x006C } } } };

TestCaseConfig const scenario_5{
    .starting_intrinsic_volumes = STARTING_12_9_14_8,

    .steps = { { .action     = { .count = 6,
                                 .type  = ACTION_SELL,
                                 .good  = e_processed_good::cigars },
                 .expect_eq  = { .rum    = 17,
                                 .cigars = 7,
                                 .cloth  = 19,
                                 .coats  = 12 },
                 .expect_vol = { .rum    = 0x01A5,
                                 .cigars = 0x01A8,
                                 .cloth  = 0x017A,
                                 .coats  = 0x0242 } },
               { .action     = { .count = 6,
                                 .type  = ACTION_SELL,
                                 .good  = e_processed_good::cigars },
                 .expect_eq  = { .rum    = 20,
                                 .cigars = 5,
                                 .cloth  = 20,
                                 .coats  = 15 },
                 .expect_vol = { .rum    = 0x0193,
                                 .cigars = 0x0118,
                                 .cloth  = 0x016E,
                                 .coats  = 0x022A } },
               { .action     = { .count = 6,
                                 .type  = ACTION_SELL,
                                 .good  = e_processed_good::cigars },
                 .expect_eq  = { .rum    = 20,
                                 .cigars = 4,
                                 .cloth  = 20,
                                 .coats  = 18 },
                 .expect_vol = { .rum    = 0x0181,
                                 .cigars = 0x0071,
                                 .cloth  = 0x0162,
                                 .coats  = 0x0212 } },
               { .action     = { .count = 6,
                                 .type  = ACTION_SELL,
                                 .good  = e_processed_good::cigars },
                 .expect_eq  = { .rum    = 20,
                                 .cigars = 4,
                                 .cloth  = 20,
                                 .coats  = 20 },
                 .expect_vol = { .rum    = 0x0174,
                                 .cigars = 0xFFB8,
                                 .cloth  = 0x0156,
                                 .coats  = 0x01FB } } } };

/****************************************************************
** Scenario Runners.
*****************************************************************/
void assert_price( int step_idx, e_processed_good good,
                   PGMap const&    eq_prices,
                   PGStruct const& expect_eq ) {
  INFO( fmt::format(
      "equilibrium price for commodity {} at step {}. "
      "actual={}, .expect_eq={}.",
      good, step_idx, eq_prices[good], expect_eq.get( good ) ) );
  REQUIRE( eq_prices[good] == expect_eq.get( good ) );
}

void assert_volume( int step_idx, e_processed_good good,
                    PGMap           intrinsic_volumes,
                    PGStruct const& expected_volumes ) {
  int expect_vol = expected_volumes.get( good );
  // The volume will be in the form 0xNNNN, i.e. a signed 16 bit
  // hex number, since that is what is seen when looking at the
  // save files in a hex editor (i.e., easy comparison). But
  // since they could be negative, we need to transform them back
  // to what Lua understands.
  if( expect_vol >= 0x8000 ) expect_vol = expect_vol - 0x10000;
  INFO( fmt::format(
      "intrinsic volume for commodity {} at step {}. actual={}, "
      "expect_vol={}.",
      good, step_idx, intrinsic_volumes[good], expect_vol ) );
  REQUIRE( intrinsic_volumes[good] == expect_vol );
}

void run_action( ProcessedGoodsPriceGroup&   group,
                 TestCaseConfig::Step const& step ) {
  for( int i = 0; i < step.action.count; ++i ) {
    switch( step.action.type ) {
      case TestCaseConfig::e_action_type::buy:
        group.buy( step.action.good, 100 );
        break;
      case TestCaseConfig::e_action_type::sell:
        group.sell( step.action.good, 100 );
        break;
      case TestCaseConfig::e_action_type::sell_all:
        group.sell( e_processed_good::rum, 100 );
        group.sell( e_processed_good::cigars, 100 );
        group.sell( e_processed_good::cloth, 100 );
        group.sell( e_processed_good::coats, 100 );
        break;
    }
  }
}

void run_scenario( TestCaseConfig const& scenario ) {
  ProcessedGoodsPriceGroupConfig pg_config =
      default_processed_goods_price_group_config();
  copy( scenario.starting_intrinsic_volumes,
        pg_config.starting_intrinsic_volumes );

  // This is kind of slow (quadratic in the number of steps), but
  // we need to do this because the empirical data for each step
  // was found by running all of the steps up to that point and
  // then evolving for about 20 turns. So then when we move on to
  // the next step we have to start over because the 20 turns of
  // evolution would mess it up.
  for( int final_step = 0;
       final_step < int( scenario.steps.size() );
       ++final_step ) {
    ProcessedGoodsPriceGroup group( pg_config );
    for( int i = 0; i <= final_step; ++i )
      run_action( group, scenario.steps[i] );
    // We do 20 because when we did the experiements to produce
    // the data we evolved for 20 turns at the end of each subse-
    // quence of steps.
    for( int i = 0; i < 20; ++i ) group.evolve();
    PGMap const& intrinsic_volumes = group.intrinsic_volumes();
    PGMap const  eq_prices         = group.equilibrium_prices();
    auto&        step              = scenario.steps[final_step];
    assert_price( final_step, e_processed_good::rum, eq_prices,
                  step.expect_eq );
    assert_price( final_step, e_processed_good::cigars,
                  eq_prices, step.expect_eq );
    assert_price( final_step, e_processed_good::cloth, eq_prices,
                  step.expect_eq );
    assert_price( final_step, e_processed_good::coats, eq_prices,
                  step.expect_eq );
    assert_volume( final_step, e_processed_good::rum,
                   intrinsic_volumes, step.expect_vol );
    assert_volume( final_step, e_processed_good::cigars,
                   intrinsic_volumes, step.expect_vol );
    assert_volume( final_step, e_processed_good::cloth,
                   intrinsic_volumes, step.expect_vol );
    assert_volume( final_step, e_processed_good::coats,
                   intrinsic_volumes, step.expect_vol );
  }
}

/****************************************************************
** Unit test cases.
*****************************************************************/
TEST_CASE( "[price-group] evolve without buy/sell" ) {
  ProcessedGoodsPriceGroupConfig config =
      default_processed_goods_price_group_config();
  // We'll set dutch=true just to make sure that nothing special
  // happens during the evolution; the dutch should only get an
  // advantage when buying/selling in the price group model.
  config.dutch = true;
  config.starting_intrinsic_volumes[e_processed_good::rum] =
      5000;
  config.starting_intrinsic_volumes[e_processed_good::cigars] =
      4000;
  config.starting_intrinsic_volumes[e_processed_good::cloth] =
      3000;
  config.starting_intrinsic_volumes[e_processed_good::coats] =
      2000;

  // These should influence the evolution but they should not
  // change.
  config.starting_traded_volumes[e_processed_good::rum] = 1000;
  config.starting_traded_volumes[e_processed_good::cigars] =
      2000;
  config.starting_traded_volumes[e_processed_good::cloth] = 3000;
  config.starting_traded_volumes[e_processed_good::coats] = 4000;

  ProcessedGoodsPriceGroup group( config );

  // Sanity check.
  REQUIRE( group.intrinsic_volume( e_processed_good::rum ) ==
           5000 );
  REQUIRE( group.intrinsic_volume( e_processed_good::cigars ) ==
           4000 );
  REQUIRE( group.intrinsic_volume( e_processed_good::cloth ) ==
           3000 );
  REQUIRE( group.intrinsic_volume( e_processed_good::coats ) ==
           2000 );

  // Do the evolution.
  group.evolve();

  // Tests.
  REQUIRE( group.traded_volume( e_processed_good::rum ) ==
           1000 );
  REQUIRE( group.traded_volume( e_processed_good::cigars ) ==
           2000 );
  REQUIRE( group.traded_volume( e_processed_good::cloth ) ==
           3000 );
  REQUIRE( group.traded_volume( e_processed_good::coats ) ==
           4000 );

  int expected = 0;

  expected =
      lround( ( 5000.0 + 1000.0 + .5 ) * .9921875 - 1000.0 );
  REQUIRE( group.intrinsic_volume( e_processed_good::rum ) ==
           expected );

  expected =
      lround( ( 4000.0 + 2000.0 + .5 ) * .9921875 - 2000.0 );
  REQUIRE( group.intrinsic_volume( e_processed_good::cigars ) ==
           expected );

  expected =
      lround( ( 3000.0 + 3000.0 + .5 ) * .9921875 - 3000.0 );
  REQUIRE( group.intrinsic_volume( e_processed_good::cloth ) ==
           expected );

  expected =
      lround( ( 2000.0 + 4000.0 + .5 ) * .9921875 - 4000.0 );
  REQUIRE( group.intrinsic_volume( e_processed_good::coats ) ==
           expected );
}

TEST_CASE( "[price-group] generate_random_intrinsic_volume" ) {
  World W;

  int const center = 10;
  int const window = 10;

  auto f = [&] {
    return generate_random_intrinsic_volume( W.ts(), center,
                                             window );
  };

  auto expect = [&]( int ret ) {
    int const bottom = 5;
    int const top    = 15;
    CHECK( ret >= bottom );
    CHECK( ret <= top );
    EXPECT_CALL( W.rand(), between_ints( bottom, top,
                                         e_interval::closed ) )
        .returns( ret );
  };

  expect( 10 );
  REQUIRE( f() == 10 );

  expect( 12 );
  REQUIRE( f() == 12 );

  expect( 5 );
  REQUIRE( f() == 5 );

  expect( 15 );
  REQUIRE( f() == 15 );
}

TEST_CASE( "[price-group] scenario cases" ) {
  run_scenario( scenario_0 );
  run_scenario( scenario_1 );
  run_scenario( scenario_2 );
  run_scenario( scenario_3 );
  run_scenario( scenario_4 );
  run_scenario( scenario_5 );
}

TEST_CASE( "[price-group] eq prices all zero/nan" ) {
  ProcessedGoodsPriceGroupConfig pg_config =
      default_processed_goods_price_group_config();
  pg_config.starting_intrinsic_volumes = {};
  ProcessedGoodsPriceGroup group( pg_config );
  PGMap const eq_prices = group.equilibrium_prices();
  REQUIRE( eq_prices[e_processed_good::rum] == 1 );
  REQUIRE( eq_prices[e_processed_good::cigars] == 1 );
  REQUIRE( eq_prices[e_processed_good::cloth] == 1 );
  REQUIRE( eq_prices[e_processed_good::coats] == 1 );
}

} // namespace
} // namespace rn
