/****************************************************************
**disband-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-10.
*
* Description: Unit tests for the disband module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/disband.hpp"

// Testing.
#include "test/fake/world.hpp"
#include "test/mocks/iagent.hpp"
#include "test/mocks/igui.hpp"
#include "test/mocks/render/itextometer.hpp"
#include "test/util/coro.hpp"

// Revolution Now
#include "src/map-square.hpp"
#include "src/mock/matchers.hpp"
#include "src/unit-ownership.hpp"
#include "src/visibility.hpp"

// ss
#include "src/ss/colonies.hpp"
#include "src/ss/fog-square.rds.hpp"
#include "src/ss/native-unit.rds.hpp"
#include "src/ss/natives.hpp"
#include "src/ss/ref.hpp"
#include "src/ss/settings.rds.hpp"
#include "src/ss/terrain.hpp"
#include "src/ss/unit-composition.hpp"
#include "src/ss/unit.hpp"
#include "src/ss/units.hpp"

// gfx
#include "src/gfx/iter.hpp"

// refl
#include "src/refl/to-str.hpp"

// base
#include "src/base/scope-exit.hpp"
#include "src/base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;
using namespace ::rn::signal;

using ::gfx::point;
using ::gfx::rect_iterator;
using ::gfx::size;
using ::mock::matchers::_;
using ::mock::matchers::Field;
using ::mock::matchers::StrContains;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct world : testing::World {
  world() {
    add_player( e_player::english );
    add_player( e_player::french );
    set_default_player_type( e_player::english );
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

  void mark_all_unexplored() {
    for( point const p :
         rect_iterator( terrain().world_rect_tiles() ) )
      player_square( p ) = PlayerSquare::unexplored{};
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[disband] disbandable_entities_on_tile" ) {
  world w;
  DisbandingPermissions expected;
  point const tile = { .x = 2, .y = 1 };

  VisibilityEntire const full_viz( w.ss() );
  VisibilityForPlayer const player_viz(
      w.ss(), w.default_player_type() );

  IVisibility const* viz = {};

  auto f = [&] {
    BASE_CHECK( viz );
    return disbandable_entities_on_tile( w.ss(), *viz, tile );
  };

  using explored = PlayerSquare::explored;
  using fogged   = FogStatus::fogged;
  using clear    = FogStatus::clear;

  auto add_unit = [&](
                      maybe<e_player> const player = nothing,
                      maybe<point> const p = nothing ) -> auto& {
    return w.add_unit_on_map(
        e_unit_type::free_colonist, p.value_or( tile ),
        player.value_or( w.default_player_type() ) );
  };

  auto add_brave = [&]( Dwelling const& dwelling,
                        maybe<point> const p =
                            nothing ) -> auto& {
    return w.add_native_unit_on_map(
        e_native_unit_type::armed_brave, p.value_or( tile ),
        dwelling.id );
  };

  auto expect_unit = mp::overload{
    [&]( Unit const& unit ) {
      expected.disbandable.units.push_back( unit.id() );
    },
    [&]( NativeUnit const& native_unit ) {
      expected.disbandable.units.push_back( native_unit.id );
    } };

  auto add_colony = [&]( maybe<e_player> const player =
                             nothing ) -> auto& {
    return w.add_colony(
        tile, player.value_or( w.default_player_type() ) );
  };

  auto expect_colony = [&]( Colony const& colony ) {
    expected.disbandable.colony = colony;
  };

  auto add_dwelling = [&]() -> auto& {
    return w.add_dwelling( tile, e_tribe::sioux );
  };

  auto expect_dwelling = [&]( Dwelling const& dwelling ) {
    expected.disbandable.dwelling = dwelling;
  };

  auto make_clear = [&] {
    w.player_square( tile )
        .emplace<explored>()
        .fog_status.emplace<clear>();
  };

  auto make_fogged =
      mp::overload{ [&]( Dwelling const& dwelling ) {
                     w.player_square( tile )
                         .emplace<explored>()
                         .fog_status.emplace<fogged>()
                         .contents.dwelling = dwelling;
                   },
                    [&]( Colony const& colony ) {
                      w.player_square( tile )
                          .emplace<explored>()
                          .fog_status.emplace<fogged>()
                          .contents.colony = colony;
                    },
                    [&]() {
                      w.player_square( tile )
                          .emplace<explored>()
                          .fog_status.emplace<fogged>();
                    } };

  SECTION( "cheat=yes" ) {
    w.settings().cheat_options.enabled = true;
    SECTION( "empty" ) {
      viz = &full_viz;
      add_unit( w.default_player_type(), tile.moved_down( 1 ) );
      REQUIRE( f() == expected );
    }
    SECTION( "viz=full" ) {
      viz = &full_viz;
      SECTION( "dwelling" ) {
        expect_dwelling( add_dwelling() );
        REQUIRE( f() == expected );
      }
      SECTION( "dwelling+braves" ) {
        Dwelling const& dwelling = add_dwelling();
        expect_dwelling( dwelling );
        expect_unit( add_brave( dwelling ) );
        REQUIRE( f() == expected );
      }
      SECTION( "colony, friendly, no units" ) {
        expect_colony( add_colony() );
        REQUIRE( f() == expected );
      }
      SECTION( "colony, friendly, with units" ) {
        expect_colony( add_colony() );
        expect_unit( add_unit() );
        expect_unit( add_unit() );
        REQUIRE( f() == expected );
      }
      SECTION( "colony, foreign, no units" ) {
        expect_colony( add_colony( e_player::french ) );
        REQUIRE( f() == expected );
      }
      SECTION( "colony, foreign, with units" ) {
        expect_colony( add_colony( e_player::french ) );
        expect_unit( add_unit() );
        expect_unit( add_unit() );
        REQUIRE( f() == expected );
      }
      SECTION( "one unit, friendly" ) {
        expect_unit( add_unit() );
        REQUIRE( f() == expected );
      }
      SECTION( "two units, friendly" ) {
        expect_unit( add_unit() );
        expect_unit( add_unit() );
        REQUIRE( f() == expected );
      }
      SECTION( "one unit, foreign" ) {
        expect_unit( add_unit( e_player::french ) );
        REQUIRE( f() == expected );
      }
      SECTION( "two units, foreign" ) {
        expect_unit( add_unit( e_player::french ) );
        expect_unit( add_unit( e_player::french ) );
        REQUIRE( f() == expected );
      }
    }
    SECTION( "viz=player" ) {
      viz = &player_viz;
      SECTION( "dwelling, hidden" ) {
        add_dwelling();
        REQUIRE( f() == expected );
      }
      SECTION( "dwelling, fogged" ) {
        Dwelling const& dwelling = add_dwelling();
        expect_dwelling( dwelling );
        make_fogged( dwelling );
        REQUIRE( f() == expected );
      }
      SECTION( "dwelling, clear" ) {
        expect_dwelling( add_dwelling() );
        make_clear();
        REQUIRE( f() == expected );
      }
      SECTION( "dwelling+braves, hidden" ) {
        Dwelling const& dwelling = add_dwelling();
        add_brave( dwelling );
        REQUIRE( f() == expected );
      }
      SECTION( "dwelling+braves, fogged" ) {
        Dwelling const& dwelling = add_dwelling();
        make_fogged( dwelling );
        expect_dwelling( dwelling );
        add_brave( dwelling );
        REQUIRE( f() == expected );
      }
      SECTION( "dwelling+braves, clear" ) {
        Dwelling const& dwelling = add_dwelling();
        expect_dwelling( dwelling );
        NativeUnit const& brave = add_brave( dwelling );
        expect_unit( brave );
        make_clear();
        REQUIRE( f() == expected );
      }
      SECTION( "brave, hidden" ) {
        Dwelling const& dwelling = add_dwelling();
        add_brave( dwelling );
        REQUIRE( f() == expected );
      }
      SECTION( "brave, clear" ) {
        Dwelling const& dwelling = add_dwelling();
        expect_dwelling( dwelling );
        NativeUnit const& brave = add_brave( dwelling );
        expect_unit( brave );
        make_clear();
        REQUIRE( f() == expected );
      }
      SECTION( "colony, friendly, no units" ) {
        make_clear();
        expect_colony( add_colony() );
        REQUIRE( f() == expected );
      }
      SECTION( "colony, friendly, with units" ) {
        make_clear();
        expect_colony( add_colony() );
        expect_unit( add_unit() );
        expect_unit( add_unit() );
        REQUIRE( f() == expected );
      }
      SECTION( "colony, foreign, hidden" ) {
        add_colony( e_player::french );
        REQUIRE( f() == expected );
      }
      SECTION( "colony, foreign, fogged" ) {
        Colony const& colony = add_colony( e_player::french );
        expect_colony( colony );
        make_fogged( colony );
        REQUIRE( f() == expected );
      }
      SECTION( "colony, foreign, clear" ) {
        expect_colony( add_colony( e_player::french ) );
        make_clear();
        REQUIRE( f() == expected );
      }
      SECTION( "colony, foreign, with units" ) {
        expect_colony( add_colony( e_player::french ) );
        expect_unit( add_unit( e_player::french ) );
        expect_unit( add_unit( e_player::french ) );
        make_clear();
        REQUIRE( f() == expected );
      }
      SECTION( "one unit, friendly" ) {
        expect_unit( add_unit() );
        REQUIRE( f() == expected );
      }
      SECTION( "two units, friendly" ) {
        make_clear();
        expect_unit( add_unit() );
        expect_unit( add_unit() );
        REQUIRE( f() == expected );
      }
      SECTION( "one unit, foreign, hidden" ) {
        add_unit( e_player::french );
        REQUIRE( f() == expected );
      }
      SECTION( "one unit, foreign, clear" ) {
        make_clear();
        expect_unit( add_unit( e_player::french ) );
        REQUIRE( f() == expected );
      }
      SECTION( "two units, foreign, clear" ) {
        make_clear();
        expect_unit( add_unit( e_player::french ) );
        expect_unit( add_unit( e_player::french ) );
        REQUIRE( f() == expected );
      }
    }
  }

  SECTION( "cheat=no" ) {
    w.settings().cheat_options.enabled = false;
    SECTION( "empty" ) {
      viz = &full_viz;
      add_unit( w.default_player_type(), tile.moved_down( 1 ) );
      REQUIRE( f() == expected );
    }
    SECTION( "viz=player" ) {
      viz = &player_viz;
      SECTION( "dwelling, hidden" ) {
        add_dwelling();
        REQUIRE( f() == expected );
      }
      SECTION( "dwelling, fogged" ) {
        Dwelling const& dwelling = add_dwelling();
        make_fogged( dwelling );
        REQUIRE( f() == expected );
      }
      SECTION( "dwelling, clear" ) {
        make_clear();
        REQUIRE( f() == expected );
      }
      SECTION( "dwelling+braves, hidden" ) {
        Dwelling const& dwelling = add_dwelling();
        add_brave( dwelling );
        REQUIRE( f() == expected );
      }
      SECTION( "dwelling+braves, fogged" ) {
        Dwelling const& dwelling = add_dwelling();
        make_fogged( dwelling );
        add_brave( dwelling );
        REQUIRE( f() == expected );
      }
      SECTION( "dwelling+braves, clear" ) {
        Dwelling const& dwelling = add_dwelling();
        add_brave( dwelling );
        make_clear();
        REQUIRE( f() == expected );
      }
      SECTION( "brave, hidden" ) {
        Dwelling const& dwelling = add_dwelling();
        add_brave( dwelling );
        REQUIRE( f() == expected );
      }
      SECTION( "brave, clear" ) {
        Dwelling const& dwelling = add_dwelling();
        add_brave( dwelling );
        make_clear();
        REQUIRE( f() == expected );
      }
      SECTION( "colony, friendly, no units" ) {
        make_clear();
        add_colony();
        REQUIRE( f() == expected );
      }
      SECTION( "colony, friendly, with units" ) {
        make_clear();
        add_colony();
        expect_unit( add_unit() );
        expect_unit( add_unit() );
        REQUIRE( f() == expected );
      }
      SECTION( "colony, foreign, hidden" ) {
        add_colony( e_player::french );
        REQUIRE( f() == expected );
      }
      SECTION( "colony, foreign, fogged" ) {
        Colony const& colony = add_colony( e_player::french );
        make_fogged( colony );
        REQUIRE( f() == expected );
      }
      SECTION( "colony, foreign, clear" ) {
        add_colony( e_player::french );
        make_clear();
        REQUIRE( f() == expected );
      }
      SECTION( "colony, foreign, with units" ) {
        add_colony( e_player::french );
        add_unit( e_player::french );
        add_unit( e_player::french );
        make_clear();
        REQUIRE( f() == expected );
      }
      SECTION( "one unit, friendly" ) {
        expect_unit( add_unit() );
        REQUIRE( f() == expected );
      }
      SECTION( "two units, friendly" ) {
        make_clear();
        expect_unit( add_unit() );
        expect_unit( add_unit() );
        REQUIRE( f() == expected );
      }
      SECTION( "one unit, foreign, hidden" ) {
        add_unit( e_player::french );
        REQUIRE( f() == expected );
      }
      SECTION( "one unit, foreign, clear" ) {
        make_clear();
        add_unit( e_player::french );
        REQUIRE( f() == expected );
      }
      SECTION( "two units, foreign, clear" ) {
        make_clear();
        add_unit( e_player::french );
        add_unit( e_player::french );
        REQUIRE( f() == expected );
      }
    }
  }
}

TEST_CASE( "[disband] disband_tile_ui_interaction" ) {
  world w;
  point const tile = { .x = 2, .y = 1 };

  DisbandingPermissions perms;
  EntitiesOnTile expected;

  // Although this method uses viz, it does so only deep in a UI
  // method that unfortunately we can't test here. See the end of
  // this test case. So we just use a full visibility.
  VisibilityEntire const full_viz( w.ss() );

  rr::MockTextometer textometer;
  textometer.EXPECT__font_height().by_default().returns(
      8 ); // arbitrary number.
  textometer.EXPECT__dimensions_for_line( _, _ )
      .by_default()
      .returns( size{ .w = 10, .h = 10 } ); // arbitrary numbers.
  textometer.EXPECT__spacing_between_chars( _ )
      .by_default()
      .returns( 1 ); // arbitrary number.
  textometer.EXPECT__spacing_between_lines( _ )
      .by_default()
      .returns( 1 ); // arbitrary number.

  auto f = [&] {
    return co_await_test( disband_tile_ui_interaction(
        w.ss(), w.gui(), textometer, w.default_player(),
        full_viz, perms ) );
  };

  auto expect_ask_one = [&]( string const& fragment,
                             maybe<string> const resp ) {
    w.gui()
        .EXPECT__choice( Field( &ChoiceConfig::msg,
                                StrContains( fragment ) ) )
        .returns( resp );
  };

  // Default.
  expected = {};
  REQUIRE( f() == expected );

  UnitId const farmer_id =
      w.add_unit_on_map( e_unit_type::expert_farmer, tile ).id();
  UnitId const free_colonist =
      w.add_unit_on_map( e_unit_type::free_colonist, tile ).id();

  Colony& colony = w.add_colony( tile );
  colony.name    = "abc";

  Dwelling const& dwelling =
      w.add_dwelling( tile, e_tribe::iroquois );

  // One unit only, no disband.
  perms = { .disbandable = { .units = { farmer_id } } };
  expect_ask_one( "Expert Farmer", nothing );
  expected = {};
  REQUIRE( f() == expected );

  // One unit only, disbanded.
  perms = { .disbandable = { .units = { farmer_id } } };
  expect_ask_one( "Expert Farmer", "yes" );
  expected = { .units = { farmer_id } };
  REQUIRE( f() == expected );

  // One colony, no disband.
  perms = { .disbandable = { .colony = colony } };
  expect_ask_one( "Really disband [abc]", nothing );
  expected = {};
  REQUIRE( f() == expected );

  // One colony, disbanded.
  perms = { .disbandable = { .colony = colony } };
  expect_ask_one( "Really disband [abc]", "yes" );
  expected = { .colony = colony };
  REQUIRE( f() == expected );

  // One colony, ship in port.
  {
    UnitId const ship =
        w.add_unit_on_map( e_unit_type::caravel, tile ).id();
    perms = { .disbandable = { .colony = colony } };
    w.gui().EXPECT__message_box(
        StrContains( "has a ship in its port" ) );
    expected = {};
    REQUIRE( f() == expected );
    UnitOwnershipChanger( w.ss(), ship ).destroy();
  }

  // One dwelling, no disband.
  perms = { .disbandable = { .dwelling = dwelling } };
  expect_ask_one( "Really disband [Iroquois] Village", nothing );
  expected = {};
  REQUIRE( f() == expected );

  // One dwelling, disbanded.
  perms = { .disbandable = { .dwelling = dwelling } };
  expect_ask_one( "Really disband [Iroquois] Village", "yes" );
  expected = { .dwelling = dwelling };
  REQUIRE( f() == expected );

  // Multiple units, none selected.
  perms = {
    .disbandable = { .units = { farmer_id, free_colonist } } };
  expected = { .units = {} };
  w.gui()
      .EXPECT__ok_cancel_box( "Select unit(s) to disband:", _ )
      .returns( ui::e_ok_cancel::ok );
  REQUIRE( f() == expected );

  // Multiple units, one selected.
  // Unfortunately, can't test cases where things are selected
  // since the code that builds the UI and computes results is
  // not accessible from a unit test, i.e. we can't in any way
  // simulate the clicking of the check boxes and we don't have a
  // way to just inject the results.
}

TEST_CASE( "[disband] execute_disband" ) {
  world w;
  point tile = { .x = 2, .y = 1 };
  EntitiesOnTile entities;

  VisibilityEntire const full_viz( w.ss() );
  VisibilityForPlayer const player_viz(
      w.ss(), w.default_player_type() );

  IVisibility const* viz = &full_viz;

  auto f = [&] {
    BASE_CHECK( viz );
    return co_await_test(
        execute_disband( w.ss(), w.ts(), w.default_player(),
                         *viz, tile, entities ) );
  };

  auto add_unit =
      [&]( e_unit_type const type = e_unit_type::free_colonist,
           maybe<e_player> const player = nothing ) -> auto& {
    return w.add_unit_on_map(
        type, tile, player.value_or( w.default_player_type() ) );
  };

  auto add_cargo_unit =
      [&]( UnitId const holder,
           e_unit_type const type =
               e_unit_type::free_colonist ) -> auto& {
    return w.add_unit_in_cargo( type, holder );
  };

  auto add_brave = [&]( Dwelling const& dwelling,
                        maybe<point> const p =
                            nothing ) -> auto& {
    return w.add_native_unit_on_map(
        e_native_unit_type::armed_brave, p.value_or( tile ),
        dwelling.id );
  };

  auto add_colony = [&]( maybe<e_player> const player =
                             nothing ) -> auto& {
    return w.add_colony(
        tile, player.value_or( w.default_player_type() ) );
  };

  auto add_dwelling = [&]() -> auto& {
    return w.add_dwelling( tile, e_tribe::sioux );
  };

  // Default.
  f();

  // One foreign euro unit.
  {
    UnitId const unit_id =
        add_unit( e_unit_type::free_colonist, e_player::french )
            .id();
    entities = { .units = { unit_id } };
    f();
    REQUIRE( !w.units().exists( unit_id ) );
    REQUIRE( w.player_square( tile )
                 .holds<PlayerSquare::unexplored>() );
  }

  // One euro unit.
  {
    w.mark_all_unexplored();
    UnitId const unit_id = add_unit().id();
    REQUIRE( w.player_square( tile )
                 .holds<PlayerSquare::explored>() );
    entities = { .units = { unit_id } };
    f();
    REQUIRE( !w.units().exists( unit_id ) );
    REQUIRE( w.player_square( tile )
                 .holds<PlayerSquare::explored>() );
  }

  // One brave.
  {
    w.mark_all_unexplored();
    Dwelling const& dwelling          = add_dwelling();
    NativeUnitId const native_unit_id = add_brave( dwelling ).id;
    entities = { .units = { native_unit_id } };
    f();
    REQUIRE( !w.units().exists( native_unit_id ) );
    REQUIRE( w.player_square( tile )
                 .holds<PlayerSquare::unexplored>() );
  }

  // Two euro units.
  {
    UnitId const unit_id_1 = add_unit().id();
    UnitId const unit_id_2 = add_unit().id();
    UnitId const unit_id_3 = add_unit().id();
    entities = { .units = { unit_id_1, unit_id_2 } };
    f();
    REQUIRE( !w.units().exists( unit_id_1 ) );
    REQUIRE( !w.units().exists( unit_id_2 ) );
    REQUIRE( w.units().exists( unit_id_3 ) );
  }

  // Ship with no cargo units.
  {
    UnitId const ship_id = add_unit( e_unit_type::galleon ).id();
    UnitId const unit_id_1 = add_unit().id();
    UnitId const unit_id_2 = add_unit().id();
    entities               = { .units = { ship_id } };
    f();
    REQUIRE( !w.units().exists( ship_id ) );
    REQUIRE( w.units().exists( unit_id_1 ) );
    REQUIRE( w.units().exists( unit_id_2 ) );
  }

  // Ship with cargo units, disband only ship, units offloaded.
  {
    UnitId const ship_id = add_unit( e_unit_type::galleon ).id();
    UnitId const unit_id_1 = add_cargo_unit( ship_id ).id();
    UnitId const unit_id_2 = add_cargo_unit( ship_id ).id();
    entities               = { .units = { ship_id } };
    f();
    REQUIRE( !w.units().exists( ship_id ) );
    REQUIRE( w.units().exists( unit_id_1 ) );
    REQUIRE( w.units().exists( unit_id_2 ) );
    REQUIRE( as_const( w.units() )
                 .ownership_of( unit_id_1 )
                 .holds<UnitOwnership::world>() );
    REQUIRE( as_const( w.units() )
                 .ownership_of( unit_id_2 )
                 .holds<UnitOwnership::world>() );
  }

  // Ship with cargo units, disband all.
  {
    UnitId const ship_id = add_unit( e_unit_type::galleon ).id();
    UnitId const unit_id_1 = add_cargo_unit( ship_id ).id();
    UnitId const unit_id_2 = add_cargo_unit( ship_id ).id();
    entities = { .units = { ship_id, unit_id_1, unit_id_2 } };
    f();
    REQUIRE( !w.units().exists( ship_id ) );
    REQUIRE( !w.units().exists( unit_id_1 ) );
    REQUIRE( !w.units().exists( unit_id_2 ) );
  }

  // Ship with cargo units, disband only ship, units destroyed as
  // part of ship disbanding since they can't be offloaded into
  // water.
  {
    SCOPED_SET_AND_RESTORE( tile, point{ .x = 1, .y = 1 } );
    BASE_CHECK( is_water( w.square( tile ) ) );
    UnitId const ship_id = add_unit( e_unit_type::galleon ).id();
    UnitId const unit_id_1 = add_cargo_unit( ship_id ).id();
    UnitId const unit_id_2 = add_cargo_unit( ship_id ).id();
    entities               = { .units = { ship_id } };
    f();
    REQUIRE( !w.units().exists( ship_id ) );
    REQUIRE( !w.units().exists( unit_id_1 ) );
    REQUIRE( !w.units().exists( unit_id_2 ) );
  }

  // Ship with cargo units, disband all units, tests that units
  // don't attempt to be disbanded after they've already been
  // disbanded as part of the ship (which has to disband them
  // since it is over water in this section).
  {
    SCOPED_SET_AND_RESTORE( tile, point{ .x = 1, .y = 1 } );
    BASE_CHECK( is_water( w.square( tile ) ) );
    UnitId const ship_id = add_unit( e_unit_type::galleon ).id();
    UnitId const unit_id_1 = add_cargo_unit( ship_id ).id();
    UnitId const unit_id_2 = add_cargo_unit( ship_id ).id();
    entities = { .units = { ship_id, unit_id_1, unit_id_2 } };
    f();
    REQUIRE( !w.units().exists( ship_id ) );
    REQUIRE( !w.units().exists( unit_id_1 ) );
    REQUIRE( !w.units().exists( unit_id_2 ) );
  }

  // Foreign colony.
  {
    SCOPED_SET_AND_RESTORE( viz, &player_viz );
    w.mark_all_unexplored();
    // The reference returned by add_colony will be dangling
    // after the disbanding, so we just copy it straight away.
    entities = { .colony = add_colony( e_player::french ) };
    f();
    REQUIRE(
        !w.colonies().exists( entities.colony.value().id ) );
    REQUIRE( w.player_square( tile )
                 .holds<PlayerSquare::explored>() );
  }

  // Friendly colony.
  {
    entities = { .colony = add_colony() };
    f();
    REQUIRE(
        !w.colonies().exists( entities.colony.value().id ) );
  }

  tile = { .x = 2, .y = 0 };
  BASE_CHECK( is_land( w.square( tile ) ) );

  // Dwelling, full visibility.
  {
    w.mark_all_unexplored();
    entities = { .dwelling = add_dwelling() };
    f();
    REQUIRE( !w.natives().dwelling_exists(
        entities.dwelling.value().id ) );
    REQUIRE( w.player_square( tile )
                 .holds<PlayerSquare::unexplored>() );
  }

  // Dwelling, player visibility.
  {
    SCOPED_SET_AND_RESTORE( viz, &player_viz );
    w.mark_all_unexplored();
    entities = { .dwelling = add_dwelling() };
    f();
    REQUIRE( !w.natives().dwelling_exists(
        entities.dwelling.value().id ) );
    REQUIRE( w.player_square( tile )
                 .holds<PlayerSquare::explored>() );
  }

  // Fogged foreign colony, full visibility.
  {
    w.mark_all_unexplored();
    Colony const fake_colony;
    w.player_square( tile )
        .emplace<PlayerSquare::explored>()
        .fog_status.emplace<FogStatus::fogged>()
        .contents.colony = fake_colony;
    entities             = { .colony = fake_colony };
    f();
    REQUIRE( w.player_square( tile )
                 .holds<PlayerSquare::explored>() );
    REQUIRE( w.player_square( tile )
                 .get<PlayerSquare::explored>()
                 .fog_status.holds<FogStatus::fogged>() );
  }

  // Fogged foreign colony, player visibility.
  {
    SCOPED_SET_AND_RESTORE( viz, &player_viz );
    w.mark_all_unexplored();
    Colony const fake_colony;
    w.player_square( tile )
        .emplace<PlayerSquare::explored>()
        .fog_status.emplace<FogStatus::fogged>()
        .contents.colony = fake_colony;
    entities             = { .colony = fake_colony };
    f();
    REQUIRE( w.player_square( tile )
                 .holds<PlayerSquare::explored>() );
  }
}

TEST_CASE( "[disband] execute_disband / destroy tribe" ) {
  world w;
  point tile = { .x = 2, .y = 1 };
  EntitiesOnTile entities;

  VisibilityEntire const full_viz( w.ss() );
  VisibilityForPlayer const player_viz( w.ss(),
                                        e_player::english );

  IVisibility const* viz = &full_viz;

  auto f = [&] {
    BASE_CHECK( viz );
    return co_await_test( execute_disband(
        w.ss(), w.ts(), w.english(), *viz, tile, entities ) );
  };

  auto add_dwelling = [&]() -> auto& {
    return w.add_dwelling( tile, e_tribe::sioux );
  };

  // Dwelling, full visibility.
  {
    w.mark_all_unexplored();
    entities = { .dwelling = add_dwelling() };
    w.agent().EXPECT__message_box(
        "The [Sioux] tribe has been wiped out." );
    w.agent().EXPECT__handle(
        TribeWipedOut{ .tribe = e_tribe::sioux } );
    f();
    REQUIRE( !w.natives().dwelling_exists(
        entities.dwelling.value().id ) );
    REQUIRE( w.player_square( tile )
                 .holds<PlayerSquare::unexplored>() );
  }

  // Dwelling, player visibility.
  {
    SCOPED_SET_AND_RESTORE( viz, &player_viz );
    w.mark_all_unexplored();
    entities = { .dwelling = add_dwelling() };
    w.agent()
        .EXPECT__message_box(
            "The [Sioux] tribe has been wiped out." )
        .invokes( [&] {
          // This checks that the visibility is cleared before we
          // pop up the tribe-destroyed message.
          REQUIRE( w.player_square( tile )
                       .holds<PlayerSquare::explored>() );
        } );
    w.agent().EXPECT__handle(
        TribeWipedOut{ .tribe = e_tribe::sioux } );
    f();
    REQUIRE( !w.natives().dwelling_exists(
        entities.dwelling.value().id ) );
    REQUIRE( w.player_square( tile )
                 .holds<PlayerSquare::explored>() );
  }
}

} // namespace
} // namespace rn
