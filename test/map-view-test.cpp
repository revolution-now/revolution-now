/****************************************************************
**map-view-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-07-27.
*
* Description: Unit tests for the map-view module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/map-view.hpp"

// Testing.
#include "test/fake/world.hpp"
#include "test/mocks/iuser-config.hpp"
#include "test/mocks/land-view-plane.hpp"

// Revolution Now
#include "src/camera.hpp"
#include "src/imap-updater.hpp"
#include "src/plane-stack.hpp"
#include "src/visibility.hpp"

// config
#include "src/config/user.rds.hpp"

// ss
#include "src/ss/land-view.rds.hpp"
#include "src/ss/ref.hpp"
#include "src/ss/settings.rds.hpp"
#include "src/ss/terrain.hpp"
#include "src/ss/unit-composition.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

using ::gfx::point;
using ::gfx::rect;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct world : testing::World {
  world() {
    add_player( e_player::english );
    add_player( e_player::french );
    set_default_player_type( e_player::english );
    create_default_map();

    mock_user_config.EXPECT__read().by_default().returns(
        config_user );
  }

  void create_default_map() {
    static MapSquare const _ = make_ocean();
    static MapSquare const L = make_grassland();
    vector<MapSquare> tiles{
      L, L, L, //
      L, _, L, //
      L, L, L, //
    };
    build_map( std::move( tiles ), 3 );
  }

  config_user_t config_user;
  MockIUserConfig mock_user_config;
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[map-view] can_open_colony_on_tile" ) {
  world w;

  IVisibility const* viz = {};
  VisibilityForPlayer const viz_english( w.ss(),
                                         e_player::english );
  VisibilityForPlayer const viz_french( w.ss(),
                                        e_player::french );
  VisibilityEntire const viz_entire( w.ss() );

  auto const f = [&] [[clang::noinline]] ( point const tile ) {
    BASE_CHECK( viz );
    return can_open_colony_on_tile( *viz, tile );
  };

  point const kTile1 = { .x = 0, .y = 0 };
  point const kTile2 = { .x = 2, .y = 2 };
  point const kTile3 = { .x = 1, .y = 0 };

  SECTION( "viz_english" ) {
    viz = &viz_english;

    REQUIRE( f( kTile1 ) == nothing );
    REQUIRE( f( kTile2 ) == nothing );
    REQUIRE( f( kTile3 ) == nothing );

    w.add_colony( kTile1, e_player::english );
    w.add_colony( kTile2, e_player::french );
    REQUIRE( f( kTile1 ) == nothing );
    REQUIRE( f( kTile2 ) == nothing );
    REQUIRE( f( kTile3 ) == nothing );

    w.make_clear( kTile1, e_player::english );
    w.make_clear( kTile2, e_player::english );
    w.make_clear( kTile3, e_player::english );
    REQUIRE( f( kTile1 ) == ColonyId{ 1 } );
    REQUIRE( f( kTile2 ) == nothing );
    REQUIRE( f( kTile3 ) == nothing );

    w.make_fogged( kTile2, e_player::english );
    w.make_fogged( kTile3, e_player::english );
    REQUIRE( f( kTile1 ) == ColonyId{ 1 } );
    REQUIRE( f( kTile2 ) == nothing );
    REQUIRE( f( kTile3 ) == nothing );
  }

  // In this one we use viz_french but still adjust visibility on
  // the map for the english player, otherwise this test would be
  // equivalent to the last one just with two players flipped.
  SECTION( "viz_french" ) {
    viz = &viz_french;

    REQUIRE( f( kTile1 ) == nothing );
    REQUIRE( f( kTile2 ) == nothing );
    REQUIRE( f( kTile3 ) == nothing );

    w.add_colony( kTile1, e_player::english );
    w.add_colony( kTile2, e_player::french );
    REQUIRE( f( kTile1 ) == nothing );
    REQUIRE( f( kTile2 ) == nothing );
    REQUIRE( f( kTile3 ) == nothing );

    w.make_clear( kTile1, e_player::english );
    w.make_clear( kTile2, e_player::english );
    w.make_clear( kTile3, e_player::english );
    REQUIRE( f( kTile1 ) == nothing );
    REQUIRE( f( kTile2 ) == nothing );
    REQUIRE( f( kTile3 ) == nothing );

    w.make_fogged( kTile2, e_player::english );
    w.make_fogged( kTile3, e_player::english );
    REQUIRE( f( kTile1 ) == nothing );
    REQUIRE( f( kTile2 ) == nothing );
    REQUIRE( f( kTile3 ) == nothing );
  }

  SECTION( "viz_entire" ) {
    viz = &viz_entire;

    REQUIRE( f( kTile1 ) == nothing );
    REQUIRE( f( kTile2 ) == nothing );
    REQUIRE( f( kTile3 ) == nothing );

    w.add_colony( kTile1, e_player::english );
    w.add_colony( kTile2, e_player::french );
    REQUIRE( f( kTile1 ) == ColonyId{ 1 } );
    REQUIRE( f( kTile2 ) == ColonyId{ 2 } );
    REQUIRE( f( kTile3 ) == nothing );

    w.make_clear( kTile1, e_player::english );
    w.make_clear( kTile2, e_player::english );
    w.make_clear( kTile3, e_player::english );
    REQUIRE( f( kTile1 ) == ColonyId{ 1 } );
    REQUIRE( f( kTile2 ) == ColonyId{ 2 } );
    REQUIRE( f( kTile3 ) == nothing );

    w.make_fogged( kTile2, e_player::english );
    w.make_fogged( kTile3, e_player::english );
    REQUIRE( f( kTile1 ) == ColonyId{ 1 } );
    REQUIRE( f( kTile2 ) == ColonyId{ 2 } );
    REQUIRE( f( kTile3 ) == nothing );
  }
}

TEST_CASE( "[map-view] can_activate_units_on_tile" ) {
  world w;
  using E = vector<UnitId>;

  using namespace ::gfx::literals;

  IVisibility const* viz = {};
  VisibilityForPlayer const viz_english( w.ss(),
                                         e_player::english );
  VisibilityForPlayer const viz_french( w.ss(),
                                        e_player::french );
  VisibilityEntire const viz_entire( w.ss() );

  auto const f = [&] [[clang::noinline]] ( point const tile ) {
    BASE_CHECK( viz );
    return can_activate_units_on_tile( w.ss(), *viz, tile );
  };

  point const kTile1 = { .x = 0, .y = 0 };
  point const kTile2 = { .x = 2, .y = 2 };
  point const kTile3 = { .x = 1, .y = 0 };

  w.set_player_active();

  SECTION( "viz_english" ) {
    viz = &viz_english;

    REQUIRE( f( kTile1 ) == E{} );
    REQUIRE( f( kTile2 ) == E{} );
    REQUIRE( f( kTile3 ) == E{} );

    Colony& colony = w.add_colony( kTile1, e_player::english );
    w.add_unit_indoors( colony.id, e_indoor_job::bells );
    w.add_unit_on_map( e_unit_type::soldier, kTile1,
                       e_player::english );
    w.add_unit_on_map( e_unit_type::dragoon, kTile1,
                       e_player::english );
    w.add_unit_on_map( e_unit_type::soldier, kTile2,
                       e_player::french );
    w.add_unit_on_map( e_unit_type::dragoon, kTile2,
                       e_player::french );
    w.add_dwelling_and_brave( kTile3, e_tribe::tupi );
    BASE_CHECK( viz->visible( kTile1 ) ==
                e_tile_visibility::clear );
    BASE_CHECK( viz->visible( kTile2 ) ==
                e_tile_visibility::hidden );
    REQUIRE( f( kTile1 ) == E{ UnitId{ 2 }, UnitId{ 3 } } );
    REQUIRE( f( kTile2 ) == E{} );
    REQUIRE( f( kTile3 ) == E{} );

    w.make_clear( kTile1, e_player::english );
    w.make_clear( kTile2, e_player::english );
    w.make_clear( kTile3, e_player::english );
    REQUIRE( f( kTile1 ) == E{ UnitId{ 2 }, UnitId{ 3 } } );
    REQUIRE( f( kTile2 ) == E{} );
    REQUIRE( f( kTile3 ) == E{} );

    w.make_fogged( kTile2, e_player::english );
    w.make_fogged( kTile3, e_player::english );
    REQUIRE( f( kTile1 ) == E{ UnitId{ 2 }, UnitId{ 3 } } );
    REQUIRE( f( kTile2 ) == E{} );
    REQUIRE( f( kTile3 ) == E{} );
  }

  // In this one we use viz_french but still adjust visibility on
  // the map for the english player and have the english player
  // be the active player, otherwise this test would be equiva-
  // lent to the last one just with two players flipped.
  SECTION( "viz_french" ) {
    viz = &viz_french;

    REQUIRE( f( kTile1 ) == E{} );
    REQUIRE( f( kTile2 ) == E{} );
    REQUIRE( f( kTile3 ) == E{} );

    Colony& colony = w.add_colony( kTile1, e_player::english );
    w.add_unit_indoors( colony.id, e_indoor_job::bells );
    w.add_unit_on_map( e_unit_type::soldier, kTile1,
                       e_player::english );
    w.add_unit_on_map( e_unit_type::dragoon, kTile1,
                       e_player::english );
    w.add_unit_on_map( e_unit_type::soldier, kTile2,
                       e_player::french );
    w.add_unit_on_map( e_unit_type::dragoon, kTile2,
                       e_player::french );
    w.add_dwelling_and_brave( kTile3, e_tribe::tupi );
    BASE_CHECK( viz->visible( kTile1 ) ==
                e_tile_visibility::hidden );
    BASE_CHECK( viz->visible( kTile2 ) ==
                e_tile_visibility::clear );
    REQUIRE( f( kTile1 ) == E{} );
    REQUIRE( f( kTile2 ) == E{} );
    REQUIRE( f( kTile3 ) == E{} );

    w.make_clear( kTile1, e_player::english );
    w.make_clear( kTile2, e_player::english );
    w.make_clear( kTile3, e_player::english );
    REQUIRE( f( kTile1 ) == E{} );
    REQUIRE( f( kTile2 ) == E{} );
    REQUIRE( f( kTile3 ) == E{} );

    w.make_fogged( kTile2, e_player::english );
    w.make_fogged( kTile3, e_player::english );
    REQUIRE( f( kTile1 ) == E{} );
    REQUIRE( f( kTile2 ) == E{} );
    REQUIRE( f( kTile3 ) == E{} );
  }

  SECTION( "viz_entire" ) {
    viz = &viz_entire;

    REQUIRE( f( kTile1 ) == E{} );
    REQUIRE( f( kTile2 ) == E{} );
    REQUIRE( f( kTile3 ) == E{} );

    Colony& colony = w.add_colony( kTile1, e_player::english );
    w.add_unit_indoors( colony.id, e_indoor_job::bells );
    w.add_unit_on_map( e_unit_type::soldier, kTile1,
                       e_player::english );
    w.add_unit_on_map( e_unit_type::dragoon, kTile1,
                       e_player::english );
    w.add_unit_on_map( e_unit_type::soldier, kTile2,
                       e_player::french );
    w.add_unit_on_map( e_unit_type::dragoon, kTile2,
                       e_player::french );
    w.add_dwelling_and_brave( kTile3, e_tribe::tupi );
    BASE_CHECK( viz->visible( kTile1 ) ==
                e_tile_visibility::clear );
    BASE_CHECK( viz->visible( kTile2 ) ==
                e_tile_visibility::clear );
    REQUIRE( f( kTile1 ) == E{ UnitId{ 2 }, UnitId{ 3 } } );
    REQUIRE( f( kTile2 ) == E{} );
    REQUIRE( f( kTile3 ) == E{} );

    w.make_clear( kTile1, e_player::english );
    w.make_clear( kTile2, e_player::english );
    w.make_clear( kTile3, e_player::english );
    REQUIRE( f( kTile1 ) == E{ UnitId{ 2 }, UnitId{ 3 } } );
    REQUIRE( f( kTile2 ) == E{} );
    REQUIRE( f( kTile3 ) == E{} );

    w.make_fogged( kTile2, e_player::english );
    w.make_fogged( kTile3, e_player::english );
    REQUIRE( f( kTile1 ) == E{ UnitId{ 2 }, UnitId{ 3 } } );
    REQUIRE( f( kTile2 ) == E{} );
    REQUIRE( f( kTile3 ) == E{} );
  }

  SECTION( "viz_entire, no active player" ) {
    viz = &viz_entire;
    w.set_no_player_active();

    REQUIRE( f( kTile1 ) == E{} );
    REQUIRE( f( kTile2 ) == E{} );
    REQUIRE( f( kTile3 ) == E{} );

    Colony& colony = w.add_colony( kTile1, e_player::english );
    w.add_unit_indoors( colony.id, e_indoor_job::bells );
    w.add_unit_on_map( e_unit_type::soldier, kTile1,
                       e_player::english );
    w.add_unit_on_map( e_unit_type::dragoon, kTile1,
                       e_player::english );
    w.add_unit_on_map( e_unit_type::soldier, kTile2,
                       e_player::french );
    w.add_unit_on_map( e_unit_type::dragoon, kTile2,
                       e_player::french );
    w.add_dwelling_and_brave( kTile3, e_tribe::tupi );
    BASE_CHECK( viz->visible( kTile1 ) ==
                e_tile_visibility::clear );
    BASE_CHECK( viz->visible( kTile2 ) ==
                e_tile_visibility::clear );
    REQUIRE( f( kTile1 ) == E{} );
    REQUIRE( f( kTile2 ) == E{} );
    REQUIRE( f( kTile3 ) == E{} );

    w.make_clear( kTile1, e_player::english );
    w.make_clear( kTile2, e_player::english );
    w.make_clear( kTile3, e_player::english );
    REQUIRE( f( kTile1 ) == E{} );
    REQUIRE( f( kTile2 ) == E{} );
    REQUIRE( f( kTile3 ) == E{} );

    w.make_fogged( kTile2, e_player::english );
    w.make_fogged( kTile3, e_player::english );
    REQUIRE( f( kTile1 ) == E{} );
    REQUIRE( f( kTile2 ) == E{} );
    REQUIRE( f( kTile3 ) == E{} );
  }
}

TEST_CASE( "[map-view] reveal_entire_map" ) {
  world w;
  MockLandViewPlane mock_land_view;
  w.planes().get().set_bottom<ILandViewPlane>( mock_land_view );

  auto const f = [&] [[clang::noinline]] {
    reveal_entire_map( w.ss(), w.ts() );
  };

  auto& show_indian_moves =
      w.settings().in_game_options.game_menu_options
          [e_game_menu_option::show_indian_moves];
  auto& show_foreign_moves =
      w.settings().in_game_options.game_menu_options
          [e_game_menu_option::show_foreign_moves];
  auto const& map_revealed = w.land_view().map_revealed;

  // Starting state sanity check.
  REQUIRE_FALSE( show_indian_moves );
  REQUIRE_FALSE( show_foreign_moves );
  REQUIRE( map_revealed.holds<MapRevealed::no_special_view>() );

  show_indian_moves  = true;
  show_foreign_moves = true;
  mock_land_view.EXPECT__set_visibility( maybe<e_player>{} );
  f();
  REQUIRE_FALSE( show_indian_moves );
  REQUIRE_FALSE( show_foreign_moves );
  REQUIRE( map_revealed.holds<MapRevealed::entire>() );
}

TEST_CASE( "[map-view] valid_goto_target_tiles" ) {
  world w;
  rect expected;
  Camera camera( w.mock_user_config, w.land_view().viewport,
                 w.terrain().world_size_tiles() );

  auto const f = [&] [[clang::noinline]] {
    return valid_goto_target_tiles( camera );
  };

  expected = { .origin = { .x = -1, .y = 0 },
               .size   = { .w = 5, .h = 3 } };
  REQUIRE( f() == expected );
}

} // namespace
} // namespace rn
