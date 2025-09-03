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
#include "test/mocking.hpp"
#include "test/mocks/iagent.hpp"
#include "test/mocks/igui.hpp"
#include "test/util/coro.hpp"

// Revolution Now
#include "src/ref.rds.hpp"
#include "src/unit-ownership.hpp"
#include "src/visibility.hpp"

// ss
#include "src/ss/nation.hpp"
#include "src/ss/old-world-state.rds.hpp"
#include "src/ss/player.rds.hpp"
#include "src/ss/ref.hpp"
#include "src/ss/revolution.rds.hpp"
#include "src/ss/settings.rds.hpp"
#include "src/ss/units.hpp"

// refl
#include "src/refl/to-str.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;
using namespace ::rn::signal;
using namespace ::rn::detail;

using ::mock::matchers::_;
using ::mock::matchers::StrContains;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct world : testing::World {
  world() {
    add_player( e_player::french );
    set_default_player_type( e_player::french );
    set_default_player_as_human();
    // NOTE: map not created by default since many test casess
    // don't need it.
  }

  void create_default_map() {
    MapSquare const _ = make_ocean();
    MapSquare const X = make_grassland();
    // clang-format off
    vector<MapSquare> tiles{ /*
          0 1 2 3 4 5 6 7
      0*/ _,X,_,X,X,X,X,_, /*0
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

  Player const& ref_player =
      w.add_player( ref_player_for( w.default_nation() ) );

  auto const f =
      [&] [[clang::noinline]] ( Colony const& colony ) {
        return ref_colony_selection_metrics(
            w.ss(), w.connectivity(), colony.id );
      };

  SECTION( "inland next to inland lake" ) {
    Colony& colony = w.add_colony( { .x = 2, .y = 1 } );

    expected = { .colony_id       = colony.id,
                 .strength_metric = 1,
                 .population      = 0,
                 .valid_landings  = {} };
    REQUIRE( f( colony ) == expected );
  }

  SECTION( "map edge and island" ) {
    Colony& colony = w.add_colony( { .x = 0, .y = 7 } );

    expected = { .colony_id       = colony.id,
                 .strength_metric = 1,
                 .population      = 0,
                 .valid_landings  = {} };
    REQUIRE( f( colony ) == expected );
  }

  SECTION( "coastal with two ship tiles" ) {
    Colony& colony = w.add_colony( { .x = 5, .y = 4 } );

    expected = {
      .colony_id       = colony.id,
      .strength_metric = 1,
      .population      = 0,
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

  SECTION( "coastal with two ship tiles | dwelling adjacent" ) {
    Colony& colony = w.add_colony( { .x = 5, .y = 4 } );

    w.add_dwelling( { .x = 5, .y = 5 }, e_tribe::iroquois );

    expected = {
      .colony_id       = colony.id,
      .strength_metric = 1,
      .population      = 0,
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
                    { .tile           = { .x = 6, .y = 5 },
                      .captured_units = {} },
                  },
            },
          },
    };
    REQUIRE( f( colony ) == expected );
  }

  SECTION( "coastal with two ship tiles | colony adjacent" ) {
    Colony& colony = w.add_colony( { .x = 5, .y = 4 } );

    // Shouldn't happen in a normal game since colonies are not
    // supposed to be adjacent to each other, but it could happen
    // with modding and so we want to test that it is handled.
    w.add_colony( { .x = 5, .y = 5 } );

    expected = {
      .colony_id       = colony.id,
      .strength_metric = 1,
      .population      = 0,
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
      .colony_id       = colony.id,
      .strength_metric = 1,
      .population      = 0,
      .valid_landings  = {
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
      .colony_id       = colony.id,
      .strength_metric = 2,
      .population      = 2,
      .valid_landings  = {
        { .ship_tile = { .tile           = { .x = 5, .y = 3 },
                          .captured_units = {} },
           .landings  = {
            { .tile = { .x = 4, .y = 2 }, .captured_units = {} },
            { .tile = { .x = 5, .y = 2 }, .captured_units = {} },
            { .tile = { .x = 4, .y = 4 }, .captured_units = {} },
            { .tile = { .x = 5, .y = 4 }, .captured_units = {} },
          } } } };
    REQUIRE( f( colony ) == expected );

    // In a real game we should never have a brave over a colony,
    // but we want to test that it can be handled just in case.
    w.add_native_unit_on_map(
        e_native_unit_type::brave, colony.location,
        w.add_dwelling( { .x = 2, .y = 4 }, e_tribe::apache )
            .id );
    w.add_unit_on_map( e_unit_type::free_colonist,
                       colony.location );
    w.add_unit_on_map( e_unit_type::caravel, colony.location );
    w.add_unit_on_map( e_unit_type::scout, colony.location );
    w.add_unit_on_map( e_unit_type::soldier, colony.location );
    w.add_unit_on_map( e_unit_type::artillery, colony.location );
    w.add_unit_on_map( e_unit_type::dragoon, colony.location );
    w.add_unit_indoors( colony.id, e_indoor_job::bells );
    colony.buildings[e_colony_building::fortress] = true;

    expected = {
      .colony_id       = colony.id,
      .strength_metric = 24,
      .population      = 3,
      .valid_landings  = {
        { .ship_tile = { .tile           = { .x = 5, .y = 3 },
                          .captured_units = {} },
           .landings  = {
            { .tile = { .x = 4, .y = 2 }, .captured_units = {} },
            { .tile = { .x = 5, .y = 2 }, .captured_units = {} },
            { .tile = { .x = 4, .y = 4 }, .captured_units = {} },
            { .tile = { .x = 5, .y = 4 }, .captured_units = {} },
          } } } };
    REQUIRE( f( colony ) == expected );

    colony.commodities[e_commodity::muskets] = 100;
    w.add_unit_on_map( e_unit_type::damaged_artillery,
                       colony.location );

    expected = {
      .colony_id       = colony.id,
      .strength_metric = 36,
      .population      = 3,
      .valid_landings  = {
        { .ship_tile = { .tile           = { .x = 5, .y = 3 },
                          .captured_units = {} },
           .landings  = {
            { .tile = { .x = 4, .y = 2 }, .captured_units = {} },
            { .tile = { .x = 5, .y = 2 }, .captured_units = {} },
            { .tile = { .x = 4, .y = 4 }, .captured_units = {} },
            { .tile = { .x = 5, .y = 4 }, .captured_units = {} },
          } } } };
    REQUIRE( f( colony ) == expected );

    colony.buildings[e_colony_building::fortress] = false;
    colony.buildings[e_colony_building::fort]     = true;

    expected = {
      .colony_id       = colony.id,
      .strength_metric = 26,
      .population      = 3,
      .valid_landings  = {
        { .ship_tile = { .tile           = { .x = 5, .y = 3 },
                          .captured_units = {} },
           .landings  = {
            { .tile = { .x = 4, .y = 2 }, .captured_units = {} },
            { .tile = { .x = 5, .y = 2 }, .captured_units = {} },
            { .tile = { .x = 4, .y = 4 }, .captured_units = {} },
            { .tile = { .x = 5, .y = 4 }, .captured_units = {} },
          } } } };
    REQUIRE( f( colony ) == expected );

    colony.buildings[e_colony_building::fort]     = false;
    colony.buildings[e_colony_building::stockade] = true;

    expected = {
      .colony_id       = colony.id,
      .strength_metric = 17,
      .population      = 3,
      .valid_landings  = {
        { .ship_tile = { .tile           = { .x = 5, .y = 3 },
                          .captured_units = {} },
           .landings  = {
            { .tile = { .x = 4, .y = 2 }, .captured_units = {} },
            { .tile = { .x = 5, .y = 2 }, .captured_units = {} },
            { .tile = { .x = 4, .y = 4 }, .captured_units = {} },
            { .tile = { .x = 5, .y = 4 }, .captured_units = {} },
          } } } };
    REQUIRE( f( colony ) == expected );

    w.add_unit_on_map( e_unit_type::continental_army,
                       colony.location );

    expected = {
      .colony_id       = colony.id,
      .strength_metric = 21,
      .population      = 3,
      .valid_landings  = {
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

  SECTION(
      "coastal colony with one ship tile + captured units" ) {
    Colony& colony = w.add_colony( { .x = 4, .y = 3 } );

    using enum e_unit_type;

    w.add_unit_on_map( free_colonist, { .x = 5, .y = 2 } );
    w.add_unit_on_map( artillery, { .x = 5, .y = 4 } );
    w.add_unit_on_map( caravel, { .x = 5, .y = 3 } );
    w.add_unit_on_map( regular, { .x = 4, .y = 2 },
                       ref_player.type );

    w.add_native_unit_on_map(
        e_native_unit_type::brave, { .x = 4, .y = 4 },
        w.add_dwelling( { .x = 3, .y = 4 }, e_tribe::apache )
            .id );

    expected = {
      .colony_id       = colony.id,
      .strength_metric = 1,
      .population      = 0,
      .valid_landings  = {
        { .ship_tile = { .tile           = { .x = 5, .y = 3 },
                          .captured_units = { GenericUnitId{
                           3 } } },
           .landings  = {
            { .tile = { .x = 4, .y = 2 }, .captured_units = {} },
            { .tile           = { .x = 5, .y = 2 },
                .captured_units = { GenericUnitId{ 1 } } },
            { .tile           = { .x = 4, .y = 4 },
                .captured_units = { GenericUnitId{ 5 } } },
            { .tile           = { .x = 5, .y = 4 },
                .captured_units = { GenericUnitId{ 2 } } },
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

  Colony& colony = w.add_colony( { .x = 4, .y = 3 } );

  expected = {
    .colony_id       = 1,
    .strength_metric = 1,
    .population      = 0,
    .valid_landings  = {
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
    .strength_metric = 15,
    .population      = 4,
    .valid_landings  = {},
  };
  REQUIRE( f() == nothing );

  metrics = {
    .strength_metric = 15,
    .population      = 4,
    .valid_landings  = { {} },
  };
  REQUIRE( f() == 11 );

  metrics = {
    .strength_metric = 1,
    .population      = 4,
    .valid_landings  = { {} },
  };
  REQUIRE( f() == -3 );

  metrics = {
    .strength_metric = 1,
    .population      = 5,
    .valid_landings  = { {} },
  };
  REQUIRE( f() == -4 );

  metrics = {
    .strength_metric = 100,
    .population      = 20,
    .valid_landings  = { {} },
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
  RefColonyLandingTiles tiles;
  RefColonyLandingTiles expected;

  auto const f = [&] [[clang::noinline]] {
    filter_ref_landing_tiles( tiles );
  };

  using G = GenericUnitId;

  // Default.
  tiles    = {};
  expected = {};
  f();
  REQUIRE( tiles == expected );

  // No landings.
  tiles = {
    .ship_tile =
        {
          .tile           = {},
          .captured_units = {},
        },
    .landings = {},
  };
  expected = {};
  f();
  REQUIRE( tiles == expected );

  // Single landing, zero captured.
  tiles    = { .ship_tile = { .tile = {}, .captured_units = {} },
               .landings  = {
              { .tile = { .x = 1 }, .captured_units = {} } } };
  expected = { .ship_tile = { .tile = {}, .captured_units = {} },
               .landings  = { { .tile           = { .x = 1 },
                                .captured_units = {} } } };
  f();
  REQUIRE( tiles == expected );

  // Single landing, some captured.
  tiles    = { .ship_tile = { .tile = {}, .captured_units = {} },
               .landings  = { { .tile           = { .x = 1 },
                                .captured_units = { G{ 1 }, G{ 2 },
                                                    G{ 3 } } } } };
  expected = { .ship_tile = { .tile = {}, .captured_units = {} },
               .landings  = { { .tile           = { .x = 1 },
                                .captured_units = {
                                 G{ 1 }, G{ 2 }, G{ 3 } } } } };
  f();
  REQUIRE( tiles == expected );

  // Multiple landings, all zero.
  tiles    = { .ship_tile = { .tile = {}, .captured_units = {} },
               .landings  = {
              { .tile = { .x = 1 }, .captured_units = {} },
              { .tile = { .x = 2 }, .captured_units = {} },
              { .tile = { .x = 3 }, .captured_units = {} },
              { .tile = { .x = 4 }, .captured_units = {} } } };
  expected = {
    .ship_tile = { .tile = {}, .captured_units = {} },
    .landings  = {
      { .tile = { .x = 1 }, .captured_units = {} },
      { .tile = { .x = 2 }, .captured_units = {} },
      { .tile = { .x = 3 }, .captured_units = {} },
      { .tile = { .x = 4 }, .captured_units = {} } } };
  f();
  REQUIRE( tiles == expected );

  // Multiple landings, some zero.
  tiles = {
    .ship_tile = { .tile = {}, .captured_units = {} },
    .landings  = {
      { .tile           = { .x = 1 },
         .captured_units = { G{ 1 }, G{ 2 }, G{ 3 } } },
      { .tile = { .x = 2 }, .captured_units = {} },
      { .tile = { .x = 3 }, .captured_units = { G{ 4 } } },
      { .tile = { .x = 4 }, .captured_units = {} } } };
  expected = {
    .ship_tile = { .tile = {}, .captured_units = {} },
    .landings  = {
      { .tile = { .x = 2 }, .captured_units = {} },
      { .tile = { .x = 4 }, .captured_units = {} } } };
  f();
  REQUIRE( tiles == expected );

  // Multiple landings, one zero.
  tiles    = { .ship_tile = { .tile = {}, .captured_units = {} },
               .landings  = {
              { .tile           = { .x = 1 },
                    .captured_units = { G{ 1 }, G{ 2 }, G{ 3 } } },
              { .tile           = { .x = 2 },
                    .captured_units = { G{ 6 }, G{ 7 } } },
              { .tile           = { .x = 3 },
                    .captured_units = { G{ 4 }, G{ 9 }, G{ 10 } } },
              { .tile = { .x = 4 }, .captured_units = {} } } };
  expected = { .ship_tile = { .tile = {}, .captured_units = {} },
               .landings  = { { .tile           = { .x = 4 },
                                .captured_units = {} } } };
  f();
  REQUIRE( tiles == expected );

  // Multiple landings, no zeroes.
  tiles = {
    .ship_tile = { .tile = {}, .captured_units = {} },
    .landings  = {
      { .tile           = { .x = 1 },
         .captured_units = { G{ 1 }, G{ 2 }, G{ 3 } } },
      { .tile = { .x = 2 }, .captured_units = { G{ 5 } } },
      { .tile           = { .x = 3 },
         .captured_units = { G{ 4 }, G{ 9 }, G{ 10 } } },
      { .tile           = { .x = 4 },
         .captured_units = { G{ 6 }, G{ 7 } } } } };
  expected = {
    .ship_tile = { .tile = {}, .captured_units = {} },
    .landings  = {
      { .tile = { .x = 2 }, .captured_units = { G{ 5 } } } } };
  f();
  REQUIRE( tiles == expected );
}

TEST_CASE( "[ref] is_initial_visit_to_colony" ) {
  Colony colony;

  auto const f = [&] [[clang::noinline]] {
    return is_initial_visit_to_colony( colony );
  };

  REQUIRE( f() == true );
  ++colony.ref_landings;
  REQUIRE( f() == false );
  ++colony.ref_landings;
  REQUIRE( f() == false );
  ++colony.ref_landings;
  REQUIRE( f() == false );
  ++colony.ref_landings;
  REQUIRE( f() == false );
  colony.ref_landings = 0;
  REQUIRE( f() == true );
}

TEST_CASE( "[ref] ensure_manowar_availability" ) {
  world w;
  w.create_default_map();

  Player& player                 = w.default_player();
  e_nation const nation          = player.nation;
  e_player const ref_player_type = ref_player_for( nation );
  w.add_player( ref_player_type );

  auto const f = [&] [[clang::noinline]] {
    return detail::ensure_manowar_availability( w.ss().as_const,
                                                nation );
  };

  using E = e_ref_manowar_availability;

  // Default.
  REQUIRE( f() == E::none_but_can_add );

  w.add_unit_on_map( e_unit_type::frigate, { .x = 0, .y = 0 } );
  REQUIRE( f() == E::none_but_can_add );

  w.add_unit_on_map( e_unit_type::man_o_war, { .x = 0, .y = 0 },
                     player.type );
  REQUIRE( f() == E::none_but_can_add );

  w.add_unit_on_map( e_unit_type::man_o_war, { .x = 0, .y = 1 },
                     ref_player_type );
  REQUIRE( f() == E::available_on_map );

  player.revolution.expeditionary_force.man_o_war = 1;
  w.add_unit_on_map( e_unit_type::man_o_war,
                     { .x = 0, .y = 0 } );
  REQUIRE( f() == E::available_in_stock );
}

TEST_CASE( "[ref] select_ref_unit_count" ) {
  RefColonySelectionMetrics metrics;

  auto const f = [&] [[clang::noinline]] {
    return select_ref_unit_count( metrics );
  };

  // Default.
  REQUIRE( f() == 3 );

  metrics.strength_metric = 1;
  REQUIRE( f() == 3 );
  metrics.strength_metric = 2;
  REQUIRE( f() == 3 );
  metrics.strength_metric = 3;
  REQUIRE( f() == 3 );
  metrics.strength_metric = 4;
  REQUIRE( f() == 3 );
  metrics.strength_metric = 5;
  REQUIRE( f() == 3 );
  metrics.strength_metric = 6;
  REQUIRE( f() == 4 );
  metrics.strength_metric = 7;
  REQUIRE( f() == 4 );
  metrics.strength_metric = 8;
  REQUIRE( f() == 5 );
  metrics.strength_metric = 9;
  REQUIRE( f() == 5 );
  metrics.strength_metric = 10;
  REQUIRE( f() == 6 );
  metrics.strength_metric = 11;
  REQUIRE( f() == 6 );
  metrics.strength_metric = 12;
  REQUIRE( f() == 6 );
  metrics.strength_metric = 1000;
  REQUIRE( f() == 6 );
}

TEST_CASE( "[ref] select_ref_unit_sequence" ) {
  world w;
  RefColonySelectionMetrics metrics;

  Player& player = w.default_player();
  auto& force    = player.revolution.expeditionary_force;

  auto const f = [&] [[clang::noinline]] {
    return select_ref_unit_sequence( w.ss(), w.default_nation(),
                                     metrics );
  };

  using enum e_ref_unit_sequence;

  force.regular   = 10;
  force.cavalry   = 10;
  force.artillery = 10;
  force.man_o_war = 10;

  // Default.
  REQUIRE( f() == weak );

  metrics.strength_metric = 1;
  REQUIRE( f() == weak );
  metrics.strength_metric = 5;
  REQUIRE( f() == weak );
  metrics.strength_metric = 10;
  REQUIRE( f() == weak );
  metrics.strength_metric = 15;
  REQUIRE( f() == weak );
  metrics.strength_metric = 20;
  REQUIRE( f() == weak );
  metrics.strength_metric = 25;
  REQUIRE( f() == weak );
  metrics.strength_metric = 26;
  REQUIRE( f() == weak );
  metrics.strength_metric = 27;
  REQUIRE( f() == weak );
  metrics.strength_metric = 28;
  REQUIRE( f() == weak );
  metrics.strength_metric = 29;
  REQUIRE( f() == weak );
  metrics.strength_metric = 30;
  REQUIRE( f() == strong );
  metrics.strength_metric = 31;
  REQUIRE( f() == strong );
  metrics.strength_metric = 35;
  REQUIRE( f() == strong );
  metrics.strength_metric = 40;
  REQUIRE( f() == strong );
  metrics.strength_metric = 1000;
  REQUIRE( f() == strong );

  force.regular   = 10;
  force.cavalry   = 8;
  force.artillery = 8;
  REQUIRE( f() == strong );

  force.regular   = 10;
  force.cavalry   = 6;
  force.artillery = 6;
  REQUIRE( f() == strong );

  force.regular   = 10;
  force.cavalry   = 5;
  force.artillery = 6;
  REQUIRE( f() == strong );

  force.regular   = 10;
  force.cavalry   = 6;
  force.artillery = 5;
  REQUIRE( f() == strong );

  force.regular   = 10;
  force.cavalry   = 5;
  force.artillery = 5;
  REQUIRE( f() == weak );

  force.regular   = 9;
  force.cavalry   = 5;
  force.artillery = 5;
  REQUIRE( f() == strong );

  force.regular   = 9;
  force.cavalry   = 4;
  force.artillery = 5;
  REQUIRE( f() == weak );

  force.regular   = 9;
  force.cavalry   = 5;
  force.artillery = 4;
  REQUIRE( f() == weak );
}

TEST_CASE( "[ref] allocate_landing_units" ) {
  world w;
  RefLandingPlan expected;

  Player& colonial_player = w.default_player();
  auto& force = colonial_player.revolution.expeditionary_force;
  auto& regulars  = force.regular;
  auto& cavalry   = force.cavalry;
  auto& artillery = force.artillery;

  using enum e_ref_unit_sequence;
  using U = e_unit_type;

  RefColonyLandingTiles landing_tiles;
  int n_units_requested        = 0;
  bool initial_visit_to_colony = false;
  e_ref_unit_sequence sequence = weak;

  auto const f = [&] [[clang::noinline]] {
    return allocate_landing_units(
        as_const( colonial_player ), initial_visit_to_colony,
        landing_tiles, sequence, n_units_requested );
  };

  // Default.
  // ------------------------------------------------------------
  expected = {};
  REQUIRE( f() == expected );

  // tiles=1, n_units=0, init=true, seq=weak, stock=9/9/9.
  // ------------------------------------------------------------
  landing_tiles.ship_tile = {
    .tile           = { .x = 7, .y = 4 },
    .captured_units = { GenericUnitId{ 2 },
                        GenericUnitId{ 5 } } };
  landing_tiles.landings = {
    RefLandingTile{
      .tile           = { .x = 6, .y = 5 },
      .captured_units = { GenericUnitId{ 7 } },
    },
  };
  n_units_requested                   = 0;
  initial_visit_to_colony             = true;
  sequence                            = weak;
  tie( regulars, cavalry, artillery ) = tuple{ 9, 9, 9 };
  expected = { .ship_tile = landing_tiles.ship_tile };
  REQUIRE( f() == expected );

  // tiles=1, n_units=1, init=true, seq=weak, stock=9/9/9.
  // ------------------------------------------------------------
  landing_tiles.ship_tile = {
    .tile           = { .x = 7, .y = 4 },
    .captured_units = { GenericUnitId{ 2 },
                        GenericUnitId{ 5 } } };
  landing_tiles.landings = {
    RefLandingTile{
      .tile           = { .x = 6, .y = 5 },
      .captured_units = { GenericUnitId{ 7 } },
    },
  };
  n_units_requested                   = 1;
  initial_visit_to_colony             = true;
  sequence                            = weak;
  tie( regulars, cavalry, artillery ) = tuple{ 9, 9, 9 };

  expected = {
    .ship_tile     = landing_tiles.ship_tile,
    .landing_units = {
      { U::regular,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
    } };
  REQUIRE( f() == expected );

  // tiles=1, n_units=2, init=true, seq=weak, stock=9/9/9.
  // ------------------------------------------------------------
  landing_tiles.ship_tile = {
    .tile           = { .x = 7, .y = 4 },
    .captured_units = { GenericUnitId{ 2 },
                        GenericUnitId{ 5 } } };
  landing_tiles.landings = {
    RefLandingTile{
      .tile           = { .x = 6, .y = 5 },
      .captured_units = { GenericUnitId{ 7 } },
    },
  };
  n_units_requested                   = 2;
  initial_visit_to_colony             = true;
  sequence                            = weak;
  tie( regulars, cavalry, artillery ) = tuple{ 9, 9, 9 };

  expected = {
    .ship_tile     = landing_tiles.ship_tile,
    .landing_units = {
      { U::regular,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::cavalry,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
    } };
  REQUIRE( f() == expected );

  // tiles=1, n_units=3, init=true, seq=weak, stock=9/9/9.
  // ------------------------------------------------------------
  landing_tiles.ship_tile = {
    .tile           = { .x = 7, .y = 4 },
    .captured_units = { GenericUnitId{ 2 },
                        GenericUnitId{ 5 } } };
  landing_tiles.landings = {
    RefLandingTile{
      .tile           = { .x = 6, .y = 5 },
      .captured_units = { GenericUnitId{ 7 } },
    },
  };
  n_units_requested                   = 3;
  initial_visit_to_colony             = true;
  sequence                            = weak;
  tie( regulars, cavalry, artillery ) = tuple{ 9, 9, 9 };

  expected = {
    .ship_tile     = landing_tiles.ship_tile,
    .landing_units = {
      { U::regular,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::cavalry,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::artillery,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
    } };
  REQUIRE( f() == expected );

  // tiles=1, n_units=6, init=true, seq=weak, stock=9/9/9.
  // ------------------------------------------------------------
  landing_tiles.ship_tile = {
    .tile           = { .x = 7, .y = 4 },
    .captured_units = { GenericUnitId{ 2 },
                        GenericUnitId{ 5 } } };
  landing_tiles.landings = {
    RefLandingTile{
      .tile           = { .x = 6, .y = 5 },
      .captured_units = { GenericUnitId{ 7 } },
    },
  };
  n_units_requested                   = 6;
  initial_visit_to_colony             = true;
  sequence                            = weak;
  tie( regulars, cavalry, artillery ) = tuple{ 9, 9, 9 };

  expected = {
    .ship_tile     = landing_tiles.ship_tile,
    .landing_units = {
      { U::regular,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::cavalry,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::artillery,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::regular,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::regular,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::regular,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
    } };
  REQUIRE( f() == expected );

  // tiles=1, n_units=6, init=false, seq=weak, stock=9/9/9.
  // ------------------------------------------------------------
  landing_tiles.ship_tile = {
    .tile           = { .x = 7, .y = 4 },
    .captured_units = { GenericUnitId{ 2 },
                        GenericUnitId{ 5 } } };
  landing_tiles.landings = {
    RefLandingTile{
      .tile           = { .x = 6, .y = 5 },
      .captured_units = { GenericUnitId{ 7 } },
    },
  };
  n_units_requested                   = 6;
  initial_visit_to_colony             = false;
  sequence                            = weak;
  tie( regulars, cavalry, artillery ) = tuple{ 9, 9, 9 };

  expected = {
    .ship_tile     = landing_tiles.ship_tile,
    .landing_units = {
      { U::cavalry,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::artillery,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::regular,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::regular,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::regular,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::regular,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
    } };
  REQUIRE( f() == expected );

  // tiles=1, n_units=6, init=false, seq=strong, stock=9/9/9.
  // ------------------------------------------------------------
  landing_tiles.ship_tile = {
    .tile           = { .x = 7, .y = 4 },
    .captured_units = { GenericUnitId{ 2 },
                        GenericUnitId{ 5 } } };
  landing_tiles.landings = {
    RefLandingTile{
      .tile           = { .x = 6, .y = 5 },
      .captured_units = { GenericUnitId{ 7 } },
    },
  };
  n_units_requested                   = 6;
  initial_visit_to_colony             = false;
  sequence                            = strong;
  tie( regulars, cavalry, artillery ) = tuple{ 9, 9, 9 };

  expected = {
    .ship_tile     = landing_tiles.ship_tile,
    .landing_units = {
      { U::cavalry,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::cavalry,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::artillery,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::artillery,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::regular,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::regular,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
    } };
  REQUIRE( f() == expected );

  // tiles=1, n_units=6, init=true, seq=strong, stock=9/9/9.
  // ------------------------------------------------------------
  landing_tiles.ship_tile = {
    .tile           = { .x = 7, .y = 4 },
    .captured_units = { GenericUnitId{ 2 },
                        GenericUnitId{ 5 } } };
  landing_tiles.landings = {
    RefLandingTile{
      .tile           = { .x = 6, .y = 5 },
      .captured_units = { GenericUnitId{ 7 } },
    },
  };
  n_units_requested                   = 6;
  initial_visit_to_colony             = true;
  sequence                            = strong;
  tie( regulars, cavalry, artillery ) = tuple{ 9, 9, 9 };

  expected = {
    .ship_tile     = landing_tiles.ship_tile,
    .landing_units = {
      { U::regular,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::cavalry,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::cavalry,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::artillery,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::artillery,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::regular,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
    } };
  REQUIRE( f() == expected );

  // tiles=1, n_units=6, init=true, seq=strong, stock=9/9/9.
  // ------------------------------------------------------------
  landing_tiles.ship_tile = {
    .tile           = { .x = 7, .y = 4 },
    .captured_units = { GenericUnitId{ 2 },
                        GenericUnitId{ 5 } } };
  landing_tiles.landings = {
    RefLandingTile{
      .tile           = { .x = 6, .y = 5 },
      .captured_units = { GenericUnitId{ 7 } },
    },
  };
  n_units_requested                   = 4;
  initial_visit_to_colony             = true;
  sequence                            = strong;
  tie( regulars, cavalry, artillery ) = tuple{ 9, 9, 9 };

  expected = {
    .ship_tile     = landing_tiles.ship_tile,
    .landing_units = {
      { U::regular,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::cavalry,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::cavalry,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::artillery,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
    } };
  REQUIRE( f() == expected );

  // tiles=1, n_units=4, init=true, seq=strong, stock=1/9/9.
  // ------------------------------------------------------------
  landing_tiles.ship_tile = {
    .tile           = { .x = 7, .y = 4 },
    .captured_units = { GenericUnitId{ 2 },
                        GenericUnitId{ 5 } } };
  landing_tiles.landings = {
    RefLandingTile{
      .tile           = { .x = 6, .y = 5 },
      .captured_units = { GenericUnitId{ 7 } },
    },
  };
  n_units_requested                   = 4;
  initial_visit_to_colony             = true;
  sequence                            = strong;
  tie( regulars, cavalry, artillery ) = tuple{ 1, 9, 9 };

  expected = {
    .ship_tile     = landing_tiles.ship_tile,
    .landing_units = {
      { U::regular,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::cavalry,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::cavalry,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::artillery,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
    } };
  REQUIRE( f() == expected );

  // tiles=1, n_units=5, init=true, seq=strong, stock=1/9/9.
  // ------------------------------------------------------------
  landing_tiles.ship_tile = {
    .tile           = { .x = 7, .y = 4 },
    .captured_units = { GenericUnitId{ 2 },
                        GenericUnitId{ 5 } } };
  landing_tiles.landings = {
    RefLandingTile{
      .tile           = { .x = 6, .y = 5 },
      .captured_units = { GenericUnitId{ 7 } },
    },
  };
  n_units_requested                   = 5;
  initial_visit_to_colony             = true;
  sequence                            = strong;
  tie( regulars, cavalry, artillery ) = tuple{ 1, 9, 9 };

  expected = {
    .ship_tile     = landing_tiles.ship_tile,
    .landing_units = {
      { U::regular,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::cavalry,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::cavalry,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::artillery,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::artillery,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
    } };
  REQUIRE( f() == expected );

  // tiles=1, n_units=6, init=true, seq=strong, stock=1/9/9.
  // ------------------------------------------------------------
  landing_tiles.ship_tile = {
    .tile           = { .x = 7, .y = 4 },
    .captured_units = { GenericUnitId{ 2 },
                        GenericUnitId{ 5 } } };
  landing_tiles.landings = {
    RefLandingTile{
      .tile           = { .x = 6, .y = 5 },
      .captured_units = { GenericUnitId{ 7 } },
    },
  };
  n_units_requested                   = 6;
  initial_visit_to_colony             = true;
  sequence                            = strong;
  tie( regulars, cavalry, artillery ) = tuple{ 1, 9, 9 };

  expected = {
    .ship_tile     = landing_tiles.ship_tile,
    .landing_units = {
      { U::regular,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::cavalry,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::cavalry,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::artillery,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::artillery,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      // No more regulars, and we've max'd out cavalry and ar-
      // tillery.
    } };
  REQUIRE( f() == expected );

  // tiles=1, n_units=6, init=true, seq=strong, stock=0/9/9.
  // ------------------------------------------------------------
  landing_tiles.ship_tile = {
    .tile           = { .x = 7, .y = 4 },
    .captured_units = { GenericUnitId{ 2 },
                        GenericUnitId{ 5 } } };
  landing_tiles.landings = {
    RefLandingTile{
      .tile           = { .x = 6, .y = 5 },
      .captured_units = { GenericUnitId{ 7 } },
    },
  };
  n_units_requested                   = 6;
  initial_visit_to_colony             = true;
  sequence                            = strong;
  tie( regulars, cavalry, artillery ) = tuple{ 0, 9, 9 };

  expected = {
    .ship_tile     = landing_tiles.ship_tile,
    .landing_units = {
      { U::cavalry,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::cavalry,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::artillery,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::artillery,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      // No more regulars, and we've max'd out cavalry and ar-
      // tillery.
    } };
  REQUIRE( f() == expected );

  // tiles=1, n_units=6, init=true, seq=strong, stock=0/1/9.
  // ------------------------------------------------------------
  landing_tiles.ship_tile = {
    .tile           = { .x = 7, .y = 4 },
    .captured_units = { GenericUnitId{ 2 },
                        GenericUnitId{ 5 } } };
  landing_tiles.landings = {
    RefLandingTile{
      .tile           = { .x = 6, .y = 5 },
      .captured_units = { GenericUnitId{ 7 } },
    },
  };
  n_units_requested                   = 6;
  initial_visit_to_colony             = true;
  sequence                            = strong;
  tie( regulars, cavalry, artillery ) = tuple{ 0, 1, 9 };

  expected = {
    .ship_tile     = landing_tiles.ship_tile,
    .landing_units = {
      { U::cavalry,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::artillery,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::artillery,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
    } };
  REQUIRE( f() == expected );

  // tiles=1, n_units=6, init=true, seq=strong, stock=2/2/2.
  // ------------------------------------------------------------
  landing_tiles.ship_tile = {
    .tile           = { .x = 7, .y = 4 },
    .captured_units = { GenericUnitId{ 2 },
                        GenericUnitId{ 5 } } };
  landing_tiles.landings = {
    RefLandingTile{
      .tile           = { .x = 6, .y = 5 },
      .captured_units = { GenericUnitId{ 7 } },
    },
  };
  n_units_requested                   = 6;
  initial_visit_to_colony             = true;
  sequence                            = strong;
  tie( regulars, cavalry, artillery ) = tuple{ 2, 2, 2 };

  expected = {
    .ship_tile     = landing_tiles.ship_tile,
    .landing_units = {
      { U::regular,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::cavalry,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::cavalry,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::artillery,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::artillery,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::regular,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
    } };
  REQUIRE( f() == expected );

  // tiles=1, n_units=6, init=false, seq=strong, stock=2/2/2.
  // ------------------------------------------------------------
  landing_tiles.ship_tile = {
    .tile           = { .x = 7, .y = 4 },
    .captured_units = { GenericUnitId{ 2 },
                        GenericUnitId{ 5 } } };
  landing_tiles.landings = {
    RefLandingTile{
      .tile           = { .x = 6, .y = 5 },
      .captured_units = { GenericUnitId{ 7 } },
    },
  };
  n_units_requested                   = 6;
  initial_visit_to_colony             = false;
  sequence                            = strong;
  tie( regulars, cavalry, artillery ) = tuple{ 2, 2, 2 };

  expected = {
    .ship_tile     = landing_tiles.ship_tile,
    .landing_units = {
      { U::cavalry,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::cavalry,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::artillery,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::artillery,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::regular,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::regular,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
    } };
  REQUIRE( f() == expected );

  // tiles=1, n_units=6, init=false, seq=strong, stock=2/1/1.
  // ------------------------------------------------------------
  landing_tiles.ship_tile = {
    .tile           = { .x = 7, .y = 4 },
    .captured_units = { GenericUnitId{ 2 },
                        GenericUnitId{ 5 } } };
  landing_tiles.landings = {
    RefLandingTile{
      .tile           = { .x = 6, .y = 5 },
      .captured_units = { GenericUnitId{ 7 } },
    },
  };
  n_units_requested                   = 6;
  initial_visit_to_colony             = false;
  sequence                            = strong;
  tie( regulars, cavalry, artillery ) = tuple{ 2, 1, 1 };

  expected = {
    .ship_tile     = landing_tiles.ship_tile,
    .landing_units = {
      { U::cavalry,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::artillery,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::regular,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::regular,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
    } };
  REQUIRE( f() == expected );

  // tiles=1, n_units=5, init=true, seq=weak, stock=10/0/0.
  // ------------------------------------------------------------
  landing_tiles.ship_tile = {
    .tile           = { .x = 7, .y = 4 },
    .captured_units = { GenericUnitId{ 2 },
                        GenericUnitId{ 5 } } };
  landing_tiles.landings = {
    RefLandingTile{
      .tile           = { .x = 6, .y = 5 },
      .captured_units = { GenericUnitId{ 7 } },
    },
  };
  n_units_requested                   = 5;
  initial_visit_to_colony             = true;
  sequence                            = weak;
  tie( regulars, cavalry, artillery ) = tuple{ 10, 0, 0 };

  expected = {
    .ship_tile     = landing_tiles.ship_tile,
    .landing_units = {
      { U::regular,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::regular,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::regular,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::regular,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::regular,
        { .tile           = { .x = 6, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
    } };
  REQUIRE( f() == expected );

  // tiles=3, n_units=0, init=true, seq=weak, stock=9/9/9.
  // ------------------------------------------------------------
  landing_tiles.ship_tile = { .tile = { .x = 6, .y = 4 } };
  landing_tiles.landings  = {
    RefLandingTile{
       .tile           = { .x = 5, .y = 4 },
       .captured_units = {},
    },
    RefLandingTile{
       .tile           = { .x = 5, .y = 5 },
       .captured_units = { GenericUnitId{ 7 } },
    },
    RefLandingTile{
       .tile           = { .x = 6, .y = 5 },
       .captured_units = {},
    },
  };
  n_units_requested                   = 0;
  initial_visit_to_colony             = true;
  sequence                            = weak;
  tie( regulars, cavalry, artillery ) = tuple{ 9, 9, 9 };

  expected = { .ship_tile     = landing_tiles.ship_tile,
               .landing_units = {
                 //
               } };
  REQUIRE( f() == expected );

  // tiles=3, n_units=1, init=true, seq=weak, stock=9/9/9.
  // ------------------------------------------------------------
  landing_tiles.ship_tile = { .tile = { .x = 6, .y = 4 } };
  landing_tiles.landings  = {
    RefLandingTile{
       .tile           = { .x = 5, .y = 4 },
       .captured_units = {},
    },
    RefLandingTile{
       .tile           = { .x = 5, .y = 5 },
       .captured_units = { GenericUnitId{ 7 } },
    },
    RefLandingTile{
       .tile           = { .x = 6, .y = 5 },
       .captured_units = {},
    },
  };
  n_units_requested                   = 1;
  initial_visit_to_colony             = true;
  sequence                            = weak;
  tie( regulars, cavalry, artillery ) = tuple{ 9, 9, 9 };

  expected = {
    .ship_tile     = landing_tiles.ship_tile,
    .landing_units = {
      { U::regular,
        { .tile = { .x = 5, .y = 4 }, .captured_units = {} } },
    } };
  REQUIRE( f() == expected );

  // tiles=3, n_units=3, init=true, seq=weak, stock=9/9/9.
  // ------------------------------------------------------------
  landing_tiles.ship_tile = { .tile = { .x = 6, .y = 4 } };
  landing_tiles.landings  = {
    RefLandingTile{
       .tile           = { .x = 5, .y = 4 },
       .captured_units = {},
    },
    RefLandingTile{
       .tile           = { .x = 5, .y = 5 },
       .captured_units = { GenericUnitId{ 7 } },
    },
    RefLandingTile{
       .tile           = { .x = 6, .y = 5 },
       .captured_units = {},
    },
  };
  n_units_requested                   = 3;
  initial_visit_to_colony             = true;
  sequence                            = weak;
  tie( regulars, cavalry, artillery ) = tuple{ 9, 9, 9 };

  expected = {
    .ship_tile     = landing_tiles.ship_tile,
    .landing_units = {
      { U::regular,
        { .tile = { .x = 5, .y = 4 }, .captured_units = {} } },
      { U::regular,
        { .tile           = { .x = 5, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::regular,
        { .tile = { .x = 6, .y = 5 }, .captured_units = {} } },
    } };
  REQUIRE( f() == expected );

  // tiles=3, n_units=6, init=true, seq=weak, stock=9/9/9.
  // ------------------------------------------------------------
  landing_tiles.ship_tile = { .tile = { .x = 6, .y = 4 } };
  landing_tiles.landings  = {
    RefLandingTile{
       .tile           = { .x = 5, .y = 4 },
       .captured_units = {},
    },
    RefLandingTile{
       .tile           = { .x = 5, .y = 5 },
       .captured_units = { GenericUnitId{ 7 } },
    },
    RefLandingTile{
       .tile           = { .x = 6, .y = 5 },
       .captured_units = {},
    },
  };
  n_units_requested                   = 6;
  initial_visit_to_colony             = true;
  sequence                            = weak;
  tie( regulars, cavalry, artillery ) = tuple{ 9, 9, 9 };

  expected = {
    .ship_tile     = landing_tiles.ship_tile,
    .landing_units = {
      { U::regular,
        { .tile = { .x = 5, .y = 4 }, .captured_units = {} } },
      { U::regular,
        { .tile           = { .x = 5, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::regular,
        { .tile = { .x = 6, .y = 5 }, .captured_units = {} } },
      { U::cavalry,
        { .tile = { .x = 5, .y = 4 }, .captured_units = {} } },
      { U::artillery,
        { .tile           = { .x = 5, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::regular,
        { .tile = { .x = 6, .y = 5 }, .captured_units = {} } },
    } };
  REQUIRE( f() == expected );

  // tiles=3, n_units=6, init=false, seq=weak, stock=9/9/9.
  // ------------------------------------------------------------
  landing_tiles.ship_tile = { .tile = { .x = 6, .y = 4 } };
  landing_tiles.landings  = {
    RefLandingTile{
       .tile           = { .x = 5, .y = 4 },
       .captured_units = {},
    },
    RefLandingTile{
       .tile           = { .x = 5, .y = 5 },
       .captured_units = { GenericUnitId{ 7 } },
    },
    RefLandingTile{
       .tile           = { .x = 6, .y = 5 },
       .captured_units = {},
    },
  };
  n_units_requested                   = 6;
  initial_visit_to_colony             = false;
  sequence                            = weak;
  tie( regulars, cavalry, artillery ) = tuple{ 9, 9, 9 };

  expected = {
    .ship_tile     = landing_tiles.ship_tile,
    .landing_units = {
      { U::cavalry,
        { .tile = { .x = 5, .y = 4 }, .captured_units = {} } },
      { U::artillery,
        { .tile           = { .x = 5, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::regular,
        { .tile = { .x = 6, .y = 5 }, .captured_units = {} } },
      { U::regular,
        { .tile = { .x = 5, .y = 4 }, .captured_units = {} } },
      { U::regular,
        { .tile           = { .x = 5, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::regular,
        { .tile = { .x = 6, .y = 5 }, .captured_units = {} } },
    } };
  REQUIRE( f() == expected );

  // tiles=3, n_units=6, init=false, seq=strong, stock=9/9/9.
  // ------------------------------------------------------------
  landing_tiles.ship_tile = { .tile = { .x = 6, .y = 4 } };
  landing_tiles.landings  = {
    RefLandingTile{
       .tile           = { .x = 5, .y = 4 },
       .captured_units = {},
    },
    RefLandingTile{
       .tile           = { .x = 5, .y = 5 },
       .captured_units = { GenericUnitId{ 7 } },
    },
    RefLandingTile{
       .tile           = { .x = 6, .y = 5 },
       .captured_units = {},
    },
  };
  n_units_requested                   = 6;
  initial_visit_to_colony             = false;
  sequence                            = strong;
  tie( regulars, cavalry, artillery ) = tuple{ 9, 9, 9 };

  expected = {
    .ship_tile     = landing_tiles.ship_tile,
    .landing_units = {
      { U::cavalry,
        { .tile = { .x = 5, .y = 4 }, .captured_units = {} } },
      { U::cavalry,
        { .tile           = { .x = 5, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::artillery,
        { .tile = { .x = 6, .y = 5 }, .captured_units = {} } },
      { U::artillery,
        { .tile = { .x = 5, .y = 4 }, .captured_units = {} } },
      { U::regular,
        { .tile           = { .x = 5, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::regular,
        { .tile = { .x = 6, .y = 5 }, .captured_units = {} } },
    } };
  REQUIRE( f() == expected );

  // tiles=3, n_units=6, init=false, seq=strong, stock=9/1/1.
  // ------------------------------------------------------------
  landing_tiles.ship_tile = { .tile = { .x = 6, .y = 4 } };
  landing_tiles.landings  = {
    RefLandingTile{
       .tile           = { .x = 5, .y = 4 },
       .captured_units = {},
    },
    RefLandingTile{
       .tile           = { .x = 5, .y = 5 },
       .captured_units = { GenericUnitId{ 7 } },
    },
    RefLandingTile{
       .tile           = { .x = 6, .y = 5 },
       .captured_units = {},
    },
  };
  n_units_requested                   = 6;
  initial_visit_to_colony             = false;
  sequence                            = strong;
  tie( regulars, cavalry, artillery ) = tuple{ 9, 1, 1 };

  expected = {
    .ship_tile     = landing_tiles.ship_tile,
    .landing_units = {
      { U::cavalry,
        { .tile = { .x = 5, .y = 4 }, .captured_units = {} } },
      { U::artillery,
        { .tile           = { .x = 5, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::regular,
        { .tile = { .x = 6, .y = 5 }, .captured_units = {} } },
      { U::regular,
        { .tile = { .x = 5, .y = 4 }, .captured_units = {} } },
      { U::regular,
        { .tile           = { .x = 5, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::regular,
        { .tile = { .x = 6, .y = 5 }, .captured_units = {} } },
    } };
  REQUIRE( f() == expected );

  // tiles=3, n_units=5, init=false, seq=strong, stock=9/1/0.
  // ------------------------------------------------------------
  landing_tiles.ship_tile = { .tile = { .x = 6, .y = 4 } };
  landing_tiles.landings  = {
    RefLandingTile{
       .tile           = { .x = 5, .y = 4 },
       .captured_units = {},
    },
    RefLandingTile{
       .tile           = { .x = 5, .y = 5 },
       .captured_units = { GenericUnitId{ 7 } },
    },
    RefLandingTile{
       .tile           = { .x = 6, .y = 5 },
       .captured_units = {},
    },
  };
  n_units_requested                   = 5;
  initial_visit_to_colony             = false;
  sequence                            = strong;
  tie( regulars, cavalry, artillery ) = tuple{ 9, 1, 0 };

  expected = {
    .ship_tile     = landing_tiles.ship_tile,
    .landing_units = {
      { U::cavalry,
        { .tile = { .x = 5, .y = 4 }, .captured_units = {} } },
      { U::regular,
        { .tile           = { .x = 5, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::regular,
        { .tile = { .x = 6, .y = 5 }, .captured_units = {} } },
      { U::regular,
        { .tile = { .x = 5, .y = 4 }, .captured_units = {} } },
      { U::regular,
        { .tile           = { .x = 5, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
    } };
  REQUIRE( f() == expected );

  // tiles=3, n_units=5, init=false, seq=strong, stock=1/5/5.
  // ------------------------------------------------------------
  landing_tiles.ship_tile = { .tile = { .x = 6, .y = 4 } };
  landing_tiles.landings  = {
    RefLandingTile{
       .tile           = { .x = 5, .y = 4 },
       .captured_units = {},
    },
    RefLandingTile{
       .tile           = { .x = 5, .y = 5 },
       .captured_units = { GenericUnitId{ 7 } },
    },
    RefLandingTile{
       .tile           = { .x = 6, .y = 5 },
       .captured_units = {},
    },
  };
  n_units_requested                   = 5;
  initial_visit_to_colony             = false;
  sequence                            = strong;
  tie( regulars, cavalry, artillery ) = tuple{ 1, 5, 5 };

  expected = {
    .ship_tile     = landing_tiles.ship_tile,
    .landing_units = {
      { U::cavalry,
        { .tile = { .x = 5, .y = 4 }, .captured_units = {} } },
      { U::cavalry,
        { .tile           = { .x = 5, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::artillery,
        { .tile = { .x = 6, .y = 5 }, .captured_units = {} } },
      { U::artillery,
        { .tile = { .x = 5, .y = 4 }, .captured_units = {} } },
      { U::regular,
        { .tile           = { .x = 5, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
    } };
  REQUIRE( f() == expected );

  // tiles=3, n_units=6, init=false, seq=strong, stock=1/5/5.
  // ------------------------------------------------------------
  landing_tiles.ship_tile = { .tile = { .x = 6, .y = 4 } };
  landing_tiles.landings  = {
    RefLandingTile{
       .tile           = { .x = 5, .y = 4 },
       .captured_units = {},
    },
    RefLandingTile{
       .tile           = { .x = 5, .y = 5 },
       .captured_units = { GenericUnitId{ 7 } },
    },
    RefLandingTile{
       .tile           = { .x = 6, .y = 5 },
       .captured_units = {},
    },
  };
  n_units_requested                   = 6;
  initial_visit_to_colony             = false;
  sequence                            = strong;
  tie( regulars, cavalry, artillery ) = tuple{ 1, 5, 5 };

  expected = {
    .ship_tile     = landing_tiles.ship_tile,
    .landing_units = {
      { U::cavalry,
        { .tile = { .x = 5, .y = 4 }, .captured_units = {} } },
      { U::cavalry,
        { .tile           = { .x = 5, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::artillery,
        { .tile = { .x = 6, .y = 5 }, .captured_units = {} } },
      { U::artillery,
        { .tile = { .x = 5, .y = 4 }, .captured_units = {} } },
      { U::regular,
        { .tile           = { .x = 5, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
    } };
  REQUIRE( f() == expected );

  // tiles=3, n_units=6, init=false, seq=strong, stock=0/5/5.
  // ------------------------------------------------------------
  landing_tiles.ship_tile = { .tile = { .x = 6, .y = 4 } };
  landing_tiles.landings  = {
    RefLandingTile{
       .tile           = { .x = 5, .y = 4 },
       .captured_units = {},
    },
    RefLandingTile{
       .tile           = { .x = 5, .y = 5 },
       .captured_units = { GenericUnitId{ 7 } },
    },
    RefLandingTile{
       .tile           = { .x = 6, .y = 5 },
       .captured_units = {},
    },
  };
  n_units_requested                   = 6;
  initial_visit_to_colony             = false;
  sequence                            = strong;
  tie( regulars, cavalry, artillery ) = tuple{ 0, 5, 5 };

  expected = {
    .ship_tile     = landing_tiles.ship_tile,
    .landing_units = {
      { U::cavalry,
        { .tile = { .x = 5, .y = 4 }, .captured_units = {} } },
      { U::cavalry,
        { .tile           = { .x = 5, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::artillery,
        { .tile = { .x = 6, .y = 5 }, .captured_units = {} } },
      { U::artillery,
        { .tile = { .x = 5, .y = 4 }, .captured_units = {} } },
    } };
  REQUIRE( f() == expected );

  // tiles=3, n_units=6, init=true, seq=strong, stock=0/5/5.
  // ------------------------------------------------------------
  landing_tiles.ship_tile = { .tile = { .x = 6, .y = 4 } };
  landing_tiles.landings  = {
    RefLandingTile{
       .tile           = { .x = 5, .y = 4 },
       .captured_units = {},
    },
    RefLandingTile{
       .tile           = { .x = 5, .y = 5 },
       .captured_units = { GenericUnitId{ 7 } },
    },
    RefLandingTile{
       .tile           = { .x = 6, .y = 5 },
       .captured_units = {},
    },
  };
  n_units_requested                   = 6;
  initial_visit_to_colony             = true;
  sequence                            = strong;
  tie( regulars, cavalry, artillery ) = tuple{ 0, 5, 5 };

  expected = {
    .ship_tile     = landing_tiles.ship_tile,
    .landing_units = {
      { U::cavalry,
        { .tile = { .x = 5, .y = 4 }, .captured_units = {} } },
      { U::cavalry,
        { .tile           = { .x = 5, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
      { U::artillery,
        { .tile = { .x = 6, .y = 5 }, .captured_units = {} } },
      { U::artillery,
        { .tile = { .x = 5, .y = 4 }, .captured_units = {} } },
    } };
  REQUIRE( f() == expected );

  // tiles=3, n_units=6, init=true, seq=strong, stock=0/0/5.
  // ------------------------------------------------------------
  landing_tiles.ship_tile = { .tile = { .x = 6, .y = 4 } };
  landing_tiles.landings  = {
    RefLandingTile{
       .tile           = { .x = 5, .y = 4 },
       .captured_units = {},
    },
    RefLandingTile{
       .tile           = { .x = 5, .y = 5 },
       .captured_units = { GenericUnitId{ 7 } },
    },
    RefLandingTile{
       .tile           = { .x = 6, .y = 5 },
       .captured_units = {},
    },
  };
  n_units_requested                   = 6;
  initial_visit_to_colony             = true;
  sequence                            = strong;
  tie( regulars, cavalry, artillery ) = tuple{ 0, 0, 5 };

  expected = {
    .ship_tile     = landing_tiles.ship_tile,
    .landing_units = {
      { U::artillery,
        { .tile = { .x = 5, .y = 4 }, .captured_units = {} } },
      { U::artillery,
        { .tile           = { .x = 5, .y = 5 },
          .captured_units = { GenericUnitId{ 7 } } } },
    } };
  REQUIRE( f() == expected );

  // tiles=3, n_units=6, init=true, seq=strong, stock=0/0/0.
  // ------------------------------------------------------------
  landing_tiles.ship_tile = { .tile = { .x = 6, .y = 4 } };
  landing_tiles.landings  = {
    RefLandingTile{
       .tile           = { .x = 5, .y = 4 },
       .captured_units = {},
    },
    RefLandingTile{
       .tile           = { .x = 5, .y = 5 },
       .captured_units = { GenericUnitId{ 7 } },
    },
    RefLandingTile{
       .tile           = { .x = 6, .y = 5 },
       .captured_units = {},
    },
  };
  n_units_requested                   = 6;
  initial_visit_to_colony             = true;
  sequence                            = strong;
  tie( regulars, cavalry, artillery ) = tuple{ 0, 0, 0 };

  expected = { .ship_tile     = landing_tiles.ship_tile,
               .landing_units = {} };
  REQUIRE( f() == expected );
}

TEST_CASE( "[ref] create_ref_landing_units" ) {
  world w;
  RefLandingPlan plan;
  RefLandingUnits expected;

  Player& player = w.default_player();
  auto& force    = player.revolution.expeditionary_force;

  w.add_player( ref_player_for( w.default_nation() ) );

  auto const f = [&] [[clang::noinline]] {
    return create_ref_landing_units( w.ss(), w.default_nation(),
                                     plan );
  };

  force.regular   = 2;
  force.cavalry   = 2;
  force.artillery = 2;
  force.man_o_war = 2;

  SECTION( "ship only" ) {
    plan = {
      .ship_tile = { .tile           = { .x = 7, .y = 4 },
                     .captured_units = { GenericUnitId{ 5 } } },

      .landing_units = {} };
    expected = {
      .ship = { .unit_id      = UnitId{ 1 },
                .landing_tile = { .tile = { .x = 7, .y = 4 },
                                  .captured_units =
                                      { GenericUnitId{ 5 } } } },

      .landed_units = {} };
    REQUIRE( f() == expected );
    REQUIRE( force.regular == 2 );
    REQUIRE( force.cavalry == 2 );
    REQUIRE( force.artillery == 2 );
    REQUIRE( force.man_o_war == 1 );

    REQUIRE( w.units().all().size() == 1 );

    REQUIRE( w.units().exists( UnitId{ 1 } ) );
    REQUIRE( w.units().unit_for( UnitId{ 1 } ).type() ==
             e_unit_type::man_o_war );
    REQUIRE( as_const( w.units() ).ownership_of( UnitId{ 1 } ) ==
             UnitOwnership::free{} );
  }

  SECTION( "multiple units" ) {
    plan = {
      .ship_tile     = { .tile           = { .x = 7, .y = 4 },
                         .captured_units = { GenericUnitId{ 5 } } },
      .landing_units = {
        { e_unit_type::regular,
          { .tile = { .x = 6, .y = 5 }, .captured_units = {} } },
        { e_unit_type::cavalry,
          { .tile           = { .x = 6, .y = 6 },
            .captured_units = { GenericUnitId{ 7 } } } },
        { e_unit_type::cavalry,
          { .tile = { .x = 7, .y = 5 }, .captured_units = {} } },
        { e_unit_type::artillery,
          { .tile = { .x = 7, .y = 6 }, .captured_units = {} } },
      } };
    expected = {
      .ship = { .unit_id      = UnitId{ 1 },
                .landing_tile = { .tile = { .x = 7, .y = 4 },
                                  .captured_units =
                                      { GenericUnitId{ 5 } } } },

      .landed_units = {
        { .unit_id      = UnitId{ 2 },
          .landing_tile = { .tile           = { .x = 6, .y = 5 },
                            .captured_units = {} } },
        { .unit_id      = UnitId{ 3 },
          .landing_tile = { .tile           = { .x = 6, .y = 6 },
                            .captured_units = { GenericUnitId{
                              7 } } } },
        { .unit_id      = UnitId{ 4 },
          .landing_tile = { .tile           = { .x = 7, .y = 5 },
                            .captured_units = {} } },
        { .unit_id      = UnitId{ 5 },
          .landing_tile = { .tile           = { .x = 7, .y = 6 },
                            .captured_units = {} } },
      } };
    REQUIRE( f() == expected );
    REQUIRE( force.regular == 1 );
    REQUIRE( force.cavalry == 0 );
    REQUIRE( force.artillery == 1 );
    REQUIRE( force.man_o_war == 1 );

    REQUIRE( w.units().all().size() == 5 );

    REQUIRE( w.units().exists( UnitId{ 1 } ) );
    REQUIRE( w.units().unit_for( UnitId{ 1 } ).type() ==
             e_unit_type::man_o_war );
    REQUIRE( as_const( w.units() ).ownership_of( UnitId{ 1 } ) ==
             UnitOwnership::free{} );

    REQUIRE( w.units().exists( UnitId{ 2 } ) );
    REQUIRE( w.units().unit_for( UnitId{ 2 } ).type() ==
             e_unit_type::regular );
    REQUIRE( as_const( w.units() ).ownership_of( UnitId{ 2 } ) ==
             UnitOwnership::cargo{ .holder = UnitId{ 1 } } );

    REQUIRE( w.units().exists( UnitId{ 3 } ) );
    REQUIRE( w.units().unit_for( UnitId{ 3 } ).type() ==
             e_unit_type::cavalry );
    REQUIRE( as_const( w.units() ).ownership_of( UnitId{ 3 } ) ==
             UnitOwnership::cargo{ .holder = UnitId{ 1 } } );

    REQUIRE( w.units().exists( UnitId{ 4 } ) );
    REQUIRE( w.units().unit_for( UnitId{ 4 } ).type() ==
             e_unit_type::cavalry );
    REQUIRE( as_const( w.units() ).ownership_of( UnitId{ 4 } ) ==
             UnitOwnership::cargo{ .holder = UnitId{ 1 } } );

    REQUIRE( w.units().exists( UnitId{ 5 } ) );
    REQUIRE( w.units().unit_for( UnitId{ 5 } ).type() ==
             e_unit_type::artillery );
    REQUIRE( as_const( w.units() ).ownership_of( UnitId{ 5 } ) ==
             UnitOwnership::cargo{ .holder = UnitId{ 1 } } );
  }
}

TEST_CASE( "[ref] produce_REF_landing_units" ) {
  world w;
}

TEST_CASE( "[ref] offboard_ref_units" ) {
  world w;
}

TEST_CASE( "[ref] ref_should_forfeight" ) {
  world w;
  w.create_default_map();

  Player& colonial_player = w.default_player();
  Player& ref_player =
      w.add_player( ref_player_for( w.default_nation() ) );

  auto const f = [&] [[clang::noinline]] {
    return ref_should_forfeight( w.ss(), ref_player );
  };

  using enum e_forfeight_reason;

  colonial_player.revolution.expeditionary_force.regular = 1;
  REQUIRE( f() == nothing );

  colonial_player.revolution.expeditionary_force.man_o_war = 1;
  REQUIRE( f() == nothing );

  colonial_player.revolution.expeditionary_force.regular = 0;
  REQUIRE( f() == no_more_land_units_in_stock );

  colonial_player.revolution.expeditionary_force.cavalry = 1;
  REQUIRE( f() == nothing );

  colonial_player.revolution.expeditionary_force.cavalry = 0;
  REQUIRE( f() == no_more_land_units_in_stock );

  colonial_player.revolution.expeditionary_force.artillery = 1;
  REQUIRE( f() == nothing );

  colonial_player.revolution.expeditionary_force.artillery = 0;
  REQUIRE( f() == no_more_land_units_in_stock );

  w.add_unit_on_map( e_unit_type::free_colonist,
                     { .x = 3, .y = 0 }, colonial_player.type );
  REQUIRE( f() == no_more_land_units_in_stock );

  w.add_unit_on_map( e_unit_type::free_colonist,
                     { .x = 1, .y = 0 }, ref_player.type );
  REQUIRE( f() == no_more_land_units_in_stock );

  w.add_unit_on_map( e_unit_type::man_o_war, { .x = 0, .y = 0 },
                     ref_player.type );
  REQUIRE( f() == no_more_land_units_in_stock );

  UnitId const soldier_id =
      w.add_unit_on_map( e_unit_type::soldier,
                         { .x = 1, .y = 0 }, ref_player.type )
          .id();
  REQUIRE( f() == nothing );

  UnitOwnershipChanger( w.ss(), soldier_id ).destroy();
  REQUIRE( f() == no_more_land_units_in_stock );

  w.add_colony( { .x = 3, .y = 0 }, colonial_player.type );
  REQUIRE( f() == no_more_land_units_in_stock );

  w.add_colony( { .x = 1, .y = 0 }, ref_player.type );
  REQUIRE( f() == nothing );
}

TEST_CASE( "[ref] do_ref_forfeight" ) {
  world w;
  w.create_default_map();
  Player& colonial_player = w.default_player();
  Player& ref_player =
      w.add_player( ref_player_for( w.default_nation() ) );

  auto const f = [&] [[clang::noinline]] {
    do_ref_forfeight( w.ss(), ref_player );
  };

  ref_player.control = e_player_control::ai;
  colonial_player.revolution.status =
      e_revolution_status::declared;

  using enum e_unit_type;
  {
    w.add_unit_on_map( free_colonist, { .x = 1, .y = 0 } );
    w.add_unit_on_map( soldier, { .x = 1, .y = 0 } );
    UnitId const ship_id =
        w.add_unit_on_map( man_o_war, { .x = 0, .y = 0 } ).id();
    w.add_unit_in_cargo( dragoon, ship_id );
  }
  {
    e_player const type = ref_player.type;
    w.add_unit_on_map( free_colonist, { .x = 1, .y = 0 }, type );
    w.add_unit_on_map( soldier, { .x = 1, .y = 0 }, type );
    UnitId const ship_id =
        w.add_unit_on_map( man_o_war, { .x = 0, .y = 0 }, type )
            .id();
    w.add_unit_in_cargo( dragoon, ship_id );
  }

  REQUIRE( ref_player.control == e_player_control::ai );
  REQUIRE( colonial_player.revolution.status ==
           e_revolution_status::declared );
  REQUIRE( w.units().all().size() == 8 );
  REQUIRE( w.units().exists( UnitId{ 1 } ) );
  REQUIRE( w.units().exists( UnitId{ 2 } ) );
  REQUIRE( w.units().exists( UnitId{ 3 } ) );
  REQUIRE( w.units().exists( UnitId{ 4 } ) );
  REQUIRE( w.units().exists( UnitId{ 5 } ) );
  REQUIRE( w.units().exists( UnitId{ 6 } ) );
  REQUIRE( w.units().exists( UnitId{ 7 } ) );
  REQUIRE( w.units().exists( UnitId{ 8 } ) );

  f();

  REQUIRE( ref_player.control == e_player_control::inactive );
  REQUIRE( colonial_player.revolution.status ==
           e_revolution_status::won );
  REQUIRE( w.units().all().size() == 4 );
  REQUIRE( w.units().exists( UnitId{ 1 } ) );
  REQUIRE( w.units().exists( UnitId{ 2 } ) );
  REQUIRE( w.units().exists( UnitId{ 3 } ) );
  REQUIRE( w.units().exists( UnitId{ 4 } ) );
  REQUIRE( !w.units().exists( UnitId{ 5 } ) );
  REQUIRE( !w.units().exists( UnitId{ 6 } ) );
  REQUIRE( !w.units().exists( UnitId{ 7 } ) );
  REQUIRE( !w.units().exists( UnitId{ 8 } ) );
}

TEST_CASE( "[ref] ref_forfeight_ui_routine" ) {
  world w;

  Player& ref_player =
      w.add_player( ref_player_for( w.default_nation() ) );

  auto const f = [&] [[clang::noinline]] {
    co_await_test( ref_forfeight_ui_routine( w.ss(), w.gui(),
                                             ref_player ) );
  };

  w.gui().EXPECT__message_box(
      StrContains( "The REF has forfeighted" ) );
  w.gui().EXPECT__message_box( _ );

  f();
}

TEST_CASE( "[ref] percent_ref_owned_population" ) {
  world w;
  w.create_default_map();
  Player& ref_player =
      w.add_player( ref_player_for( w.default_nation() ) );

  w.add_player( e_player::english );

  auto const f = [&] [[clang::noinline]] {
    return percent_ref_owned_population( w.ss(), ref_player );
  };

  // Default.
  REQUIRE( f() == 100 );

  ColonyId const english_colony_id =
      w.add_colony( { .x = 1, .y = 0 }, e_player::english ).id;
  ColonyId const french_colony_id =
      w.add_colony( { .x = 3, .y = 0 }, e_player::french ).id;
  ColonyId const ref_french_colony_id =
      w.add_colony( { .x = 5, .y = 0 }, e_player::ref_french )
          .id;

  w.add_unit_indoors( english_colony_id, e_indoor_job::bells );
  w.add_unit_indoors( english_colony_id, e_indoor_job::bells );
  w.add_unit_indoors( french_colony_id, e_indoor_job::bells );
  w.add_unit_indoors( french_colony_id, e_indoor_job::bells );
  w.add_unit_indoors( ref_french_colony_id,
                      e_indoor_job::bells );
  w.add_unit_indoors( ref_french_colony_id,
                      e_indoor_job::bells );

  w.add_unit_on_map( e_unit_type::free_colonist,
                     { .x = 1, .y = 0 }, e_player::english );
  w.add_unit_on_map( e_unit_type::soldier, { .x = 1, .y = 0 },
                     e_player::english );
  w.add_unit_on_map( e_unit_type::free_colonist,
                     { .x = 3, .y = 0 }, e_player::french );
  w.add_unit_on_map( e_unit_type::soldier, { .x = 3, .y = 0 },
                     e_player::french );
  w.add_unit_on_map( e_unit_type::free_colonist,
                     { .x = 5, .y = 0 }, e_player::ref_french );
  w.add_unit_on_map( e_unit_type::soldier, { .x = 5, .y = 0 },
                     e_player::ref_french );

  REQUIRE( f() == 50 );

  w.add_unit_indoors( english_colony_id, e_indoor_job::bells );
  REQUIRE( f() == 50 );

  w.add_unit_indoors( french_colony_id, e_indoor_job::bells );
  REQUIRE( f() == 40 );
}

TEST_CASE( "[ref] ref_should_win" ) {
  world w;
  w.create_default_map();
  w.update_terrain_connectivity();
  Player& ref_player =
      w.add_player( ref_player_for( w.default_nation() ) );

  w.add_player( e_player::english );

  auto const f = [&] [[clang::noinline]] {
    return ref_should_win( w.ss(), w.connectivity(),
                           ref_player );
  };

  using enum e_ref_win_reason;
  using enum e_indoor_job;

  // Default.
  REQUIRE( f() == port_colonies_captured );

  ColonyId const english_colony_id =
      w.add_colony( { .x = 6, .y = 0 }, e_player::english ).id;
  w.add_unit_indoors( english_colony_id, bells );
  w.add_unit_indoors( english_colony_id, bells );
  REQUIRE( f() == port_colonies_captured );

  ColonyId const ref_french_colony_id =
      w.add_colony( { .x = 5, .y = 2 }, e_player::ref_french )
          .id;
  w.add_unit_indoors( ref_french_colony_id, bells );
  w.add_unit_indoors( ref_french_colony_id, bells );
  REQUIRE( f() == port_colonies_captured );

  ColonyId const ref_french_colony_id_2 =
      w.add_colony( { .x = 3, .y = 4 }, e_player::ref_french )
          .id;
  REQUIRE( f() == port_colonies_captured );

  ColonyId const french_colony_id =
      w.add_colony( { .x = 3, .y = 0 }, e_player::french ).id;
  w.add_unit_indoors( french_colony_id, bells );
  w.add_unit_indoors( french_colony_id, bells );
  REQUIRE( f() == port_colonies_captured );

  ColonyId const french_colony_id_2 =
      w.add_colony( { .x = 5, .y = 4 }, e_player::french ).id;
  w.add_unit_indoors( french_colony_id_2, bells );
  REQUIRE( f() == nothing );

  w.add_unit_on_map( e_unit_type::free_colonist,
                     { .x = 1, .y = 0 }, e_player::english );
  w.add_unit_on_map( e_unit_type::soldier, { .x = 1, .y = 0 },
                     e_player::english );
  w.add_unit_on_map( e_unit_type::free_colonist,
                     { .x = 3, .y = 0 }, e_player::french );
  w.add_unit_on_map( e_unit_type::soldier, { .x = 3, .y = 0 },
                     e_player::french );
  w.add_unit_on_map( e_unit_type::free_colonist,
                     { .x = 5, .y = 0 }, e_player::ref_french );
  w.add_unit_on_map( e_unit_type::soldier, { .x = 5, .y = 0 },
                     e_player::ref_french );

  REQUIRE( f() == nothing );

  w.add_unit_indoors( ref_french_colony_id, bells );
  w.add_unit_indoors( ref_french_colony_id, cloth );
  w.add_unit_indoors( ref_french_colony_id, cloth );
  w.add_unit_indoors( ref_french_colony_id, cloth );
  w.add_unit_indoors( ref_french_colony_id, cigars );
  w.add_unit_indoors( ref_french_colony_id, cigars );
  w.add_unit_indoors( ref_french_colony_id, cigars );
  w.add_unit_indoors( ref_french_colony_id, tools );
  w.add_unit_indoors( ref_french_colony_id, tools );
  w.add_unit_indoors( ref_french_colony_id, tools );
  w.add_unit_indoors( ref_french_colony_id, muskets );
  w.add_unit_indoors( ref_french_colony_id, muskets );
  w.add_unit_indoors( ref_french_colony_id, muskets );
  w.add_unit_indoors( ref_french_colony_id_2, bells );
  w.add_unit_indoors( ref_french_colony_id_2, bells );
  w.add_unit_indoors( ref_french_colony_id_2, bells );
  w.add_unit_indoors( ref_french_colony_id_2, cloth );
  w.add_unit_indoors( ref_french_colony_id_2, cloth );
  w.add_unit_indoors( ref_french_colony_id_2, cloth );
  w.add_unit_indoors( ref_french_colony_id_2, cigars );
  w.add_unit_indoors( ref_french_colony_id_2, cigars );
  w.add_unit_indoors( ref_french_colony_id_2, cigars );
  w.add_unit_indoors( ref_french_colony_id_2, tools );
  w.add_unit_indoors( ref_french_colony_id_2, tools );
  REQUIRE( f() == nothing );

  w.add_unit_indoors( ref_french_colony_id_2, tools );
  REQUIRE( f() == ref_controls_90_percent_population );
}

TEST_CASE( "[ref] do_ref_win" ) {
  world w;

  Player& colonial_player = w.default_player();
  Player& ref_player =
      w.add_player( ref_player_for( w.default_nation() ) );

  auto const f = [&] [[clang::noinline]] {
    return do_ref_win( w.ss(), ref_player );
  };

  using enum e_revolution_status;

  colonial_player.revolution.status = not_declared;
  f();
  REQUIRE( colonial_player.revolution.status == lost );

  colonial_player.revolution.status = declared;
  f();
  REQUIRE( colonial_player.revolution.status == lost );
}

TEST_CASE( "[ref] ref_win_ui_routine" ) {
  world w;

  Player& ref_player =
      w.add_player( ref_player_for( w.default_nation() ) );

  e_ref_win_reason reason = {};

  auto const f = [&] [[clang::noinline]] {
    co_await_test( ref_win_ui_routine( w.ss().as_const, w.gui(),
                                       ref_player, reason ) );
  };

  w.gui().EXPECT__message_box( "The REF has won." );
  w.gui().EXPECT__message_box( _ );
  f();
}

TEST_CASE( "[ref] move_ref_harbor_ships_to_stock" ) {
  world w;
  w.create_default_map();

  Player& colonial_player = w.default_player();
  auto& force = colonial_player.revolution.expeditionary_force;
  Player& ref_player =
      w.add_player( ref_player_for( w.default_nation() ) );

  auto const f = [&] [[clang::noinline]] {
    return move_ref_harbor_ships_to_stock( w.ss(), ref_player );
  };

  using enum e_unit_type;

  // Default.
  REQUIRE( f() == 0 );

  // With some units.
  force.regular   = 8;
  force.cavalry   = 9;
  force.artillery = 10;
  force.man_o_war = 11;

  UnitId const unit_id_1 =
      w.add_unit_in_port( free_colonist, ref_player.type ).id();
  UnitId const unit_id_2 =
      w.add_unit_in_port( man_o_war, ref_player.type ).id();
  UnitId const unit_id_3 =
      w.add_unit_in_port( soldier, ref_player.type ).id();
  UnitId const unit_id_4 =
      w.add_unit_in_port( man_o_war, ref_player.type ).id();
  UnitId const unit_id_5 =
      w.add_unit_in_port( artillery, ref_player.type ).id();
  UnitId const unit_id_6 =
      w.add_unit_in_port( man_o_war, ref_player.type ).id();
  UnitId const unit_id_7 =
      w.add_unit_on_map( man_o_war, { .x = 0, .y = 0 },
                         ref_player.type )
          .id();
  UnitId const unit_id_8 =
      w.add_unit_on_map( regular, { .x = 1, .y = 0 },
                         ref_player.type )
          .id();

  UnitId const unit_id_9 =
      w.add_unit_in_port( free_colonist, colonial_player.type )
          .id();
  UnitId const unit_id_10 =
      w.add_unit_in_port( man_o_war, colonial_player.type ).id();
  UnitId const unit_id_11 =
      w.add_unit_in_port( soldier, colonial_player.type ).id();
  UnitId const unit_id_12 =
      w.add_unit_in_port( man_o_war, colonial_player.type ).id();
  UnitId const unit_id_13 =
      w.add_unit_in_port( artillery, colonial_player.type ).id();
  UnitId const unit_id_14 =
      w.add_unit_in_port( man_o_war, colonial_player.type ).id();
  UnitId const unit_id_15 =
      w.add_unit_on_map( man_o_war, { .x = 0, .y = 0 },
                         colonial_player.type )
          .id();
  UnitId const unit_id_16 =
      w.add_unit_on_map( regular, { .x = 1, .y = 0 },
                         colonial_player.type )
          .id();

  w.old_world( ref_player.type ).harbor_state.selected_unit =
      unit_id_4;

  REQUIRE( w.units().exists( unit_id_1 ) );
  REQUIRE( w.units().exists( unit_id_2 ) );
  REQUIRE( w.units().exists( unit_id_3 ) );
  REQUIRE( w.units().exists( unit_id_4 ) );
  REQUIRE( w.units().exists( unit_id_5 ) );
  REQUIRE( w.units().exists( unit_id_6 ) );
  REQUIRE( w.units().exists( unit_id_7 ) );
  REQUIRE( w.units().exists( unit_id_8 ) );
  REQUIRE( w.units().exists( unit_id_9 ) );
  REQUIRE( w.units().exists( unit_id_10 ) );
  REQUIRE( w.units().exists( unit_id_11 ) );
  REQUIRE( w.units().exists( unit_id_12 ) );
  REQUIRE( w.units().exists( unit_id_13 ) );
  REQUIRE( w.units().exists( unit_id_14 ) );
  REQUIRE( w.units().exists( unit_id_15 ) );
  REQUIRE( w.units().exists( unit_id_16 ) );
  REQUIRE( force.regular == 8 );
  REQUIRE( force.cavalry == 9 );
  REQUIRE( force.artillery == 10 );
  REQUIRE( force.man_o_war == 11 );
  REQUIRE( w.old_world( ref_player.type )
               .harbor_state.selected_unit == unit_id_4 );

  REQUIRE( f() == 3 );

  REQUIRE( w.units().exists( unit_id_1 ) );
  REQUIRE( !w.units().exists( unit_id_2 ) );
  REQUIRE( w.units().exists( unit_id_3 ) );
  REQUIRE( !w.units().exists( unit_id_4 ) );
  REQUIRE( w.units().exists( unit_id_5 ) );
  REQUIRE( !w.units().exists( unit_id_6 ) );
  REQUIRE( w.units().exists( unit_id_7 ) );
  REQUIRE( w.units().exists( unit_id_8 ) );
  REQUIRE( w.units().exists( unit_id_9 ) );
  REQUIRE( w.units().exists( unit_id_10 ) );
  REQUIRE( w.units().exists( unit_id_11 ) );
  REQUIRE( w.units().exists( unit_id_12 ) );
  REQUIRE( w.units().exists( unit_id_13 ) );
  REQUIRE( w.units().exists( unit_id_14 ) );
  REQUIRE( w.units().exists( unit_id_15 ) );
  REQUIRE( w.units().exists( unit_id_16 ) );
  REQUIRE( force.regular == 8 );
  REQUIRE( force.cavalry == 9 );
  REQUIRE( force.artillery == 10 );
  REQUIRE( force.man_o_war == 14 );
  REQUIRE( w.old_world( ref_player.type )
               .harbor_state.selected_unit == nothing );
}

} // namespace
} // namespace rn
