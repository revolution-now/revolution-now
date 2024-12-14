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

// Revolution Now
#include "src/visibility.hpp"

// ss
#include "src/ss/fog-square.rds.hpp"
#include "src/ss/native-unit.rds.hpp"
#include "src/ss/ref.hpp"
#include "src/ss/settings.rds.hpp"
#include "src/ss/unit-composition.hpp"
#include "src/ss/unit.hpp"

// refl
#include "src/refl/to-str.hpp"

// base
#include "src/base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

using ::gfx::point;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct world : testing::World {
  world() {
    add_player( e_nation::english );
    add_player( e_nation::french );
    set_default_player( e_nation::english );
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
TEST_CASE( "[disband] disbandable_entities_on_tile" ) {
  world w;
  DisbandingPermissions expected;
  point tile = { .x = 2, .y = 1 };

  VisibilityEntire const full_viz( w.ss() );
  VisibilityForNation const nation_viz( w.ss(),
                                        e_nation::english );

  IVisibility const* viz = {};

  auto f = [&] {
    BASE_CHECK( viz );
    return disbandable_entities_on_tile( w.ss(), *viz, tile );
  };

  using explored   = PlayerSquare::explored;
  using fogged     = FogStatus::fogged;
  using clear      = FogStatus::clear;

  auto add_unit = [&](
                      maybe<e_nation> const nation = nothing,
                      maybe<point> const p = nothing ) -> auto& {
    return w.add_unit_on_map(
        e_unit_type::free_colonist, p.value_or( tile ),
        nation.value_or( w.default_nation() ) );
  };

  auto add_brave = [&]( Dwelling const& dwelling,
                        maybe<point> const p =
                            nothing ) -> auto& {
    return w.add_native_unit_on_map(
        e_native_unit_type::armed_brave, p.value_or( tile ),
        dwelling.id );
  };

  auto expect_unit = overload{
    [&]( Unit const& unit ) {
      expected.disbandable.units.push_back( unit.id() );
    },
    [&]( NativeUnit const& native_unit ) {
      expected.disbandable.units.push_back( native_unit.id );
    } };

  auto expect_no_unit = overload{
    [&]( Unit const& unit ) {
      expected.non_disbandable.units.push_back( unit.id() );
    },
    [&]( NativeUnit const& native_unit ) {
      expected.non_disbandable.units.push_back( native_unit.id );
    } };

  auto add_colony = [&]( maybe<e_nation> const nation =
                             nothing ) -> auto& {
    return w.add_colony( tile,
                         nation.value_or( w.default_nation() ) );
  };

  auto expect_colony = [&]( Colony const& colony ) {
    expected.disbandable.colony = colony;
  };

  auto expect_no_colony = [&]( Colony const& colony ) {
    expected.non_disbandable.colony = colony;
  };

  auto add_dwelling = [&]() -> auto& {
    return w.add_dwelling( tile, e_tribe::sioux );
  };

  auto expect_dwelling = [&]( Dwelling const& dwelling ) {
    expected.disbandable.dwelling = dwelling;
  };

  auto expect_no_dwelling = [&]( Dwelling const& dwelling ) {
    expected.non_disbandable.dwelling = dwelling;
  };

  auto make_clear = [&] {
    w.player_square( tile )
        .emplace<explored>()
        .fog_status.emplace<clear>();
  };

  auto make_fogged =
      overload{ [&]( Dwelling const& dwelling ) {
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
      add_unit( w.default_nation(), tile.moved_down( 1 ) );
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
        expect_colony( add_colony( e_nation::french ) );
        REQUIRE( f() == expected );
      }
      SECTION( "colony, foreign, with units" ) {
        expect_colony( add_colony( e_nation::french ) );
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
        expect_unit( add_unit( e_nation::french ) );
        REQUIRE( f() == expected );
      }
      SECTION( "two units, foreign" ) {
        expect_unit( add_unit( e_nation::french ) );
        expect_unit( add_unit( e_nation::french ) );
        REQUIRE( f() == expected );
      }
    }
    SECTION( "viz=player" ) {
      viz = &nation_viz;
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
        add_colony( e_nation::french );
        REQUIRE( f() == expected );
      }
      SECTION( "colony, foreign, fogged" ) {
        Colony const& colony = add_colony( e_nation::french );
        expect_colony( colony );
        make_fogged( colony );
        REQUIRE( f() == expected );
      }
      SECTION( "colony, foreign, clear" ) {
        expect_colony( add_colony( e_nation::french ) );
        make_clear();
        REQUIRE( f() == expected );
      }
      SECTION( "colony, foreign, with units" ) {
        expect_colony( add_colony( e_nation::french ) );
        expect_unit( add_unit( e_nation::french ) );
        expect_unit( add_unit( e_nation::french ) );
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
        add_unit( e_nation::french );
        REQUIRE( f() == expected );
      }
      SECTION( "one unit, foreign, clear" ) {
        make_clear();
        expect_unit( add_unit( e_nation::french ) );
        REQUIRE( f() == expected );
      }
      SECTION( "two units, foreign, clear" ) {
        make_clear();
        expect_unit( add_unit( e_nation::french ) );
        expect_unit( add_unit( e_nation::french ) );
        REQUIRE( f() == expected );
      }
    }
  }

  SECTION( "cheat=no" ) {
    w.settings().cheat_options.enabled = false;
    SECTION( "empty" ) {
      viz = &full_viz;
      add_unit( w.default_nation(), tile.moved_down( 1 ) );
      REQUIRE( f() == expected );
    }
    SECTION( "viz=player" ) {
      viz = &nation_viz;
      SECTION( "dwelling, hidden" ) {
        add_dwelling();
        REQUIRE( f() == expected );
      }
      SECTION( "dwelling, fogged" ) {
        Dwelling const& dwelling = add_dwelling();
        expect_no_dwelling( dwelling );
        make_fogged( dwelling );
        REQUIRE( f() == expected );
      }
      SECTION( "dwelling, clear" ) {
        expect_no_dwelling( add_dwelling() );
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
        expect_no_dwelling( dwelling );
        add_brave( dwelling );
        REQUIRE( f() == expected );
      }
      SECTION( "dwelling+braves, clear" ) {
        Dwelling const& dwelling = add_dwelling();
        expect_no_dwelling( dwelling );
        NativeUnit const& brave = add_brave( dwelling );
        expect_no_unit( brave );
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
        expect_no_dwelling( dwelling );
        NativeUnit const& brave = add_brave( dwelling );
        expect_no_unit( brave );
        make_clear();
        REQUIRE( f() == expected );
      }
      SECTION( "colony, friendly, no units" ) {
        make_clear();
        expect_no_colony( add_colony() );
        REQUIRE( f() == expected );
      }
      SECTION( "colony, friendly, with units" ) {
        make_clear();
        expect_no_colony( add_colony() );
        expect_unit( add_unit() );
        expect_unit( add_unit() );
        REQUIRE( f() == expected );
      }
      SECTION( "colony, foreign, hidden" ) {
        add_colony( e_nation::french );
        REQUIRE( f() == expected );
      }
      SECTION( "colony, foreign, fogged" ) {
        Colony const& colony = add_colony( e_nation::french );
        expect_no_colony( colony );
        make_fogged( colony );
        REQUIRE( f() == expected );
      }
      SECTION( "colony, foreign, clear" ) {
        expect_no_colony( add_colony( e_nation::french ) );
        make_clear();
        REQUIRE( f() == expected );
      }
      SECTION( "colony, foreign, with units" ) {
        expect_no_colony( add_colony( e_nation::french ) );
        expect_no_unit( add_unit( e_nation::french ) );
        expect_no_unit( add_unit( e_nation::french ) );
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
        add_unit( e_nation::french );
        REQUIRE( f() == expected );
      }
      SECTION( "one unit, foreign, clear" ) {
        make_clear();
        expect_no_unit( add_unit( e_nation::french ) );
        REQUIRE( f() == expected );
      }
      SECTION( "two units, foreign, clear" ) {
        make_clear();
        expect_no_unit( add_unit( e_nation::french ) );
        expect_no_unit( add_unit( e_nation::french ) );
        REQUIRE( f() == expected );
      }
    }
  }
}

TEST_CASE( "[disband] disband_tile_ui_interaction" ) {
  world w;
}

TEST_CASE( "[disband] execute_disband" ) {
  world w;
}

} // namespace
} // namespace rn
