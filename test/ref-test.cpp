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
#include "test/mocks/iagent.hpp"
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
using namespace ::rn::signal;
using namespace ::rn::detail;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct world : testing::World {
  world() {
    add_default_player();
    // NOTE: map not created by default since many test casess
    // don't need it.
  }

  void create_default_map() {
    MapSquare const _ = make_ocean();
    MapSquare const X = make_grassland();
    // clang-format off
    vector<MapSquare> tiles{ /*
          0 1 2 3 4 5 6 7
      0*/ _,X,X,X,X,X,X,_, /*0
      1*/ _,X,X,X,X,X,X,_, /*1
      2*/ _,X,X,X,X,X,_,_, /*2
      3*/ _,X,X,X,X,_,_,_, /*3
      4*/ _,X,X,X,X,X,_,_, /*4
      5*/ _,X,X,X,X,X,X,_, /*5
      6*/ _,_,X,X,X,X,X,_, /*6
      7*/ X,_,_,X,X,X,X,_, /*7
          0 1 2 3 4 5 6 7
    */};
    // clang-format on
    build_map( std::move( tiles ), 8 );
  }

  void create_ref_landing_bug_map() {
    MapSquare const _ = make_ocean();
    MapSquare const X = make_grassland();
    // clang-format off
    vector<MapSquare> tiles{ /*
          0 1 2 3 4 5 6 7
      0*/ X,X,_,X,X,X,X,X, /*0
      1*/ X,X,_,X,X,X,X,X, /*1
      2*/ _,_,_,_,_,_,X,X, /*2
      3*/ X,X,X,X,X,_,X,X, /*3
      4*/ X,X,X,X,X,_,_,_, /*4
      5*/ X,X,X,X,X,_,X,X, /*5
      6*/ X,X,X,X,X,_,X,X, /*6
      7*/ X,X,X,X,X,_,X,X, /*7
          0 1 2 3 4 5 6 7
    */};
    // clang-format on
    build_map( std::move( tiles ), 8 );
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

  MockIAgent& mock_agent = w.agent( w.default_player_type() );

  auto const f = [&] [[clang::noinline]] (
                     e_expeditionary_force_type const type ) {
    co_await_test( add_ref_unit_ui_seq( mock_agent, type ) );
  };

  using enum e_expeditionary_force_type;

  mock_agent.EXPECT__message_box(
      "The King has announced an increase to the Royal military "
      "budget. [Regulars] have been added to the Royal "
      "Expeditionary Force, causing alarm among colonists." );
  mock_agent.EXPECT__handle( RefUnitAdded{} );
  f( regular );

  mock_agent.EXPECT__message_box(
      "The King has announced an increase to the Royal military "
      "budget. [Men-O-War] have been added to the Royal "
      "Expeditionary Force, causing alarm among colonists." );
  mock_agent.EXPECT__handle( RefUnitAdded{} );
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

TEST_CASE( "[ref] ref_colony_selection_metrics" ) {
  world w;
  w.create_default_map();
  w.update_terrain_connectivity();
  RefColonySelectionMetrics expected;

  auto const f =
      [&] [[clang::noinline]] ( Colony const& colony ) {
        return ref_colony_selection_metrics(
            w.ss(), w.connectivity(), colony.id );
      };

  using B = e_colony_barricade_type;

  SECTION( "inland" ) {
    Colony& colony = w.add_colony( { .x = 2, .y = 1 } );

    expected = { .colony_id        = colony.id,
                 .defense_strength = 1,
                 .barricade        = B::none,
                 .population       = 0,
                 .valid_landings   = {} };
    REQUIRE( f( colony ) == expected );
  }

  SECTION( "map edge and island" ) {
    Colony& colony = w.add_colony( { .x = 0, .y = 7 } );

    expected = { .colony_id        = colony.id,
                 .defense_strength = 1,
                 .barricade        = B::none,
                 .population       = 0,
                 .valid_landings   = {} };
    REQUIRE( f( colony ) == expected );
  }

  SECTION( "coastal with two ship tiles" ) {
    Colony& colony = w.add_colony( { .x = 5, .y = 4 } );

    expected = {
      .colony_id        = colony.id,
      .defense_strength = 1,
      .barricade        = B::none,
      .population       = 0,
      .valid_landings =
          {
            {
              .ship_tile =
                  {
                    .tile           = { .x = 5, .y = 3 },
                    .captured_units = {},
                  },
              .landings =
                  {
                    { .tile           = { .x = 4, .y = 3 },
                      .captured_units = {} },
                    { .tile           = { .x = 4, .y = 4 },
                      .captured_units = {} },
                  },
            },
            {
              .ship_tile =
                  {
                    .tile           = { .x = 6, .y = 4 },
                    .captured_units = {},
                  },
              .landings =
                  {
                    { .tile           = { .x = 5, .y = 5 },
                      .captured_units = {} },
                    { .tile           = { .x = 6, .y = 5 },
                      .captured_units = {} },
                  },
            },
          },
    };
    REQUIRE( f( colony ) == expected );
  }

  SECTION( "coastal colony with one ship tile" ) {
    Colony& colony = w.add_colony( { .x = 4, .y = 3 } );

    expected = {
      .colony_id        = colony.id,
      .defense_strength = 1,
      .barricade        = B::none,
      .population       = 0,
      .valid_landings   = {
        { .ship_tile = { .tile           = { .x = 5, .y = 3 },
                           .captured_units = {} },
            .landings  = {
            { .tile = { .x = 4, .y = 2 }, .captured_units = {} },
            { .tile = { .x = 5, .y = 2 }, .captured_units = {} },
            { .tile = { .x = 4, .y = 4 }, .captured_units = {} },
            { .tile = { .x = 5, .y = 4 }, .captured_units = {} },
          } } } };
    REQUIRE( f( colony ) == expected );

    colony.buildings[e_colony_building::fort] = true;
    w.add_unit_indoors( colony.id, e_indoor_job::bells );
    w.add_unit_outdoors( colony.id, e_direction::nw,
                         e_outdoor_job::food );

    expected = {
      .colony_id        = colony.id,
      .defense_strength = 1,
      .barricade        = B::fort,
      .population       = 2,
      .valid_landings   = {
        { .ship_tile = { .tile           = { .x = 5, .y = 3 },
                           .captured_units = {} },
            .landings  = {
            { .tile = { .x = 4, .y = 2 }, .captured_units = {} },
            { .tile = { .x = 5, .y = 2 }, .captured_units = {} },
            { .tile = { .x = 4, .y = 4 }, .captured_units = {} },
            { .tile = { .x = 5, .y = 4 }, .captured_units = {} },
          } } } };
    REQUIRE( f( colony ) == expected );

    w.add_unit_on_map( e_unit_type::free_colonist,
                       colony.location );
    w.add_unit_on_map( e_unit_type::soldier, colony.location );
    w.add_unit_on_map( e_unit_type::artillery, colony.location );
    w.add_unit_on_map( e_unit_type::dragoon, colony.location );
    w.add_unit_indoors( colony.id, e_indoor_job::bells );
    colony.buildings[e_colony_building::fortress] = true;

    expected = {
      .colony_id        = colony.id,
      .defense_strength = 11,
      .barricade        = B::fortress,
      .population       = 3,
      .valid_landings   = {
        { .ship_tile = { .tile           = { .x = 5, .y = 3 },
                           .captured_units = {} },
            .landings  = {
            { .tile = { .x = 4, .y = 2 }, .captured_units = {} },
            { .tile = { .x = 5, .y = 2 }, .captured_units = {} },
            { .tile = { .x = 4, .y = 4 }, .captured_units = {} },
            { .tile = { .x = 5, .y = 4 }, .captured_units = {} },
          } } } };
    REQUIRE( f( colony ) == expected );
  }
}

// See the comments in the select_ref_landing_tiles function for
// details on what we are testing here.
TEST_CASE( "[ref] ref_colony_selection_metrics (buggy spot)" ) {
  world w;
  w.create_ref_landing_bug_map();
  //      0 1 2 3 4 5 6 7
  //  0*/ X,X,_,X,X,X,X,X, /*0
  //  1*/ X,X,_,X,X,X,X,X, /*1
  //  2*/ _,_,_,_,_,_,X,X, /*2
  //  3*/ X,X,X,X,C,_,X,X, /*3
  //  4*/ X,X,X,X,X,_,_,_, /*4
  //  5*/ X,X,X,X,X,_,X,X, /*5
  //  6*/ X,X,X,X,X,_,X,X, /*6
  //  7*/ X,X,X,X,X,_,X,X, /*7
  //      0 1 2 3 4 5 6 7
  w.update_terrain_connectivity();
  RefColonySelectionMetrics expected;

  auto const f =
      [&] [[clang::noinline]] ( Colony const& colony ) {
        return ref_colony_selection_metrics(
            w.ss(), w.connectivity(), colony.id );
      };

  using B = e_colony_barricade_type;

  Colony& colony = w.add_colony( { .x = 4, .y = 3 } );

  expected = {
    .colony_id        = 1,
    .defense_strength = 1,
    .barricade        = B::none,
    .population       = 0,
    .valid_landings   = {
      { .ship_tile = { .tile = { .x = 3, .y = 2 } },
          .landings  = { { .tile = { .x = 3, .y = 3 } } } },
      { .ship_tile = { .tile = { .x = 4, .y = 2 } },
          .landings  = { { .tile = { .x = 3, .y = 3 } } } },
      { .ship_tile = { .tile = { .x = 5, .y = 3 } },
          .landings  = { { .tile = { .x = 4, .y = 4 } } } },
      { .ship_tile = { .tile = { .x = 5, .y = 4 } },
          .landings  = { { .tile = { .x = 4, .y = 4 } } } } } };
  REQUIRE( f( colony ) == expected );
}

TEST_CASE( "[ref] ref_colony_selection_score" ) {
  world w;
  RefColonySelectionMetrics metrics;

  auto const f = [&] [[clang::noinline]] {
    return ref_colony_selection_score( metrics );
  };

  // Default.
  REQUIRE( f() == nothing );

  metrics = {
    .defense_strength = 15,
    .barricade        = e_colony_barricade_type::fort,
    .population       = 4,
    .valid_landings   = {},
  };
  REQUIRE( f() == nothing );

  metrics = {
    .defense_strength = 15,
    .barricade        = e_colony_barricade_type::fort,
    .population       = 4,
    .valid_landings   = { {} },
  };
  REQUIRE( f() == 41 );

  metrics = {
    .defense_strength = 1,
    .barricade        = e_colony_barricade_type::fortress,
    .population       = 4,
    .valid_landings   = { {} },
  };
  REQUIRE( f() == 0 );

  metrics = {
    .defense_strength = 1,
    .barricade        = e_colony_barricade_type::fortress,
    .population       = 5,
    .valid_landings   = { {} },
  };
  REQUIRE( f() == -1 );

  metrics = {
    .defense_strength = 100,
    .barricade        = e_colony_barricade_type::none,
    .population       = 20,
    .valid_landings   = { {} },
  };
  REQUIRE( f() == 80 );
}

TEST_CASE( "[ref] select_ref_landing_colony" ) {
  world w;
  vector<RefColonyMetricsScored> choices;
  maybe<RefColonySelectionMetrics> expected;

  auto const f = [&] [[clang::noinline]] {
    return select_ref_landing_colony( choices );
  };

  // Default.
  choices  = {};
  expected = {};
  REQUIRE( f() == expected );

  // Single.
  choices = {
    RefColonyMetricsScored{
      .metrics = { .colony_id = 5 },
      .score   = 100,
    },
  };
  expected = { .colony_id = 5 };
  REQUIRE( f() == expected );

  // Multiple.
  choices = {
    RefColonyMetricsScored{
      .metrics = { .colony_id = 1 },
      .score   = 100,
    },
    RefColonyMetricsScored{
      .metrics = { .colony_id = 2 },
      .score   = 90,
    },
    RefColonyMetricsScored{
      .metrics = { .colony_id = 3 },
      .score   = 50,
    },
    RefColonyMetricsScored{
      .metrics = { .colony_id = 4 },
      .score   = 15,
    },
    RefColonyMetricsScored{
      .metrics = { .colony_id = 5 },
      .score   = 15,
    },
    RefColonyMetricsScored{
      .metrics = { .colony_id = 6 },
      .score   = 75,
    },
    RefColonyMetricsScored{
      .metrics = { .colony_id = 7 },
      .score   = 99,
    },
    RefColonyMetricsScored{
      .metrics = { .colony_id = 8 },
      .score   = 100,
    },
  };
  expected = { .colony_id = 4 };
  REQUIRE( f() == expected );

  // With negative.
  choices = {
    RefColonyMetricsScored{
      .metrics = { .colony_id = 1 },
      .score   = 100,
    },
    RefColonyMetricsScored{
      .metrics = { .colony_id = 2 },
      .score   = 90,
    },
    RefColonyMetricsScored{
      .metrics = { .colony_id = 3 },
      .score   = 50,
    },
    RefColonyMetricsScored{
      .metrics = { .colony_id = 4 },
      .score   = 15,
    },
    RefColonyMetricsScored{
      .metrics = { .colony_id = 5 },
      .score   = -15,
    },
    RefColonyMetricsScored{
      .metrics = { .colony_id = 6 },
      .score   = 75,
    },
    RefColonyMetricsScored{
      .metrics = { .colony_id = 7 },
      .score   = 99,
    },
    RefColonyMetricsScored{
      .metrics = { .colony_id = 8 },
      .score   = 100,
    },
  };
  expected = { .colony_id = 5 };
  REQUIRE( f() == expected );
}

TEST_CASE( "[ref] select_ref_landing_tiles" ) {
  world w;
  RefColonySelectionMetrics metrics;
  maybe<RefColonyLandingTiles> expected;

  auto const f = [&] [[clang::noinline]] {
    return select_ref_landing_tiles( metrics );
  };

  auto const R  = RefLandingTile{};
  auto const R1 = RefLandingTile{ .tile = { .x = 1 } };

  // Default.
  metrics  = {};
  expected = nothing;
  REQUIRE( f() == expected );

  // Single.
  metrics.valid_landings = {
    { .ship_tile = R, .landings = { R, R, R } },
  };
  expected = { .ship_tile = R, .landings = { R, R, R } };
  REQUIRE( f() == expected );

  // Same size.
  metrics.valid_landings = {
    { .ship_tile = R, .landings = { R1, R, R } },
    { .ship_tile = R1, .landings = { R, R1, R } },
    { .ship_tile = R, .landings = { R, R, R1 } },
    { .ship_tile = R1, .landings = { R, R, R } },
  };
  expected = { .ship_tile = R, .landings = { R1, R, R } };
  REQUIRE( f() == expected );

  // Different sizes.
  metrics.valid_landings = {
    { .ship_tile = R, .landings = { R, R } },
    { .ship_tile = R, .landings = { R, R, R } },
    { .ship_tile = R1, .landings = { R, R, R1, R } },
    { .ship_tile = R, .landings = { R, R } },
  };
  expected = { .ship_tile = R1, .landings = { R, R, R1, R } };
  REQUIRE( f() == expected );
}

TEST_CASE( "[ref] filter_ref_landing_tiles" ) {
  world w;
}

TEST_CASE( "[ref] is_initial_visit_to_colony" ) {
  world w;
}

TEST_CASE( "[ref] ensure_manowar_availability" ) {
  world w;
}

TEST_CASE( "[ref] select_ref_formation" ) {
  world w;
}

TEST_CASE( "[ref] allocate_landing_units" ) {
  world w;
}

TEST_CASE( "[ref] make_ref_landing_plan" ) {
  world w;
}

TEST_CASE( "[ref] create_ref_landing_units" ) {
  world w;
}

TEST_CASE( "[ref] produce_REF_landing_units" ) {
  world w;
}

TEST_CASE( "[ref] offboard_ref_units" ) {
  world w;
}

TEST_CASE( "[ref] ref_should_forfeight" ) {
  world w;
}

TEST_CASE( "[ref] do_ref_forfeight" ) {
  world w;
}

TEST_CASE( "[ref] ref_forfeight_ui_routine" ) {
  world w;
}

TEST_CASE( "[ref] percent_ref_owned_population" ) {
  world w;
}

TEST_CASE( "[ref] ref_should_win" ) {
  world w;
}

TEST_CASE( "[ref] do_ref_win" ) {
  world w;
}

TEST_CASE( "[ref] ref_win_ui_routine" ) {
  world w;
}

TEST_CASE( "[ref] move_ref_harbor_ships_to_stock" ) {
  world w;
}

} // namespace
} // namespace rn
