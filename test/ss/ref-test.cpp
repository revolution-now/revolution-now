/****************************************************************
**ref.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-12.
*
* Description: Unit tests for the src/ss/ref.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/ss/ref.hpp"

// ss
#include "src/ss/root.hpp"
#include "src/ss/terrain-enums.rds.hpp"

// gfx
#include "gfx/cdr-matrix.hpp"

// refl
#include "refl/cdr.hpp"
#include "refl/to-str.hpp"

// cdr
#include "cdr/converter.hpp"
#include "cdr/ext-base.hpp"
#include "cdr/ext-builtin.hpp"
#include "cdr/ext-std.hpp"

// base
#include "src/base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace ::std;
using namespace ::cdr::literals;

using ::Catch::Contains;
using ::cdr::list;
using ::cdr::null;
using ::cdr::table;
using ::cdr::value;
using ::cdr::testing::conv_from_bt;
using ::gfx::point;
using ::gfx::size;

TEST_CASE( "[ss/ref] RootState convenience methods" ) {
  RootState l;
  RootState r;
  REQUIRE( root_states_equal( l, r ) );
  r.land_view.map_revealed = MapRevealed::entire{};
  REQUIRE_FALSE( root_states_equal( l, r ) );
  RootState old_r = r;
  assign_src_to_dst( as_const( l ), r );
  REQUIRE( root_states_equal( l, r ) );
  REQUIRE_FALSE( root_states_equal( r, old_r ) );
}

TEST_CASE( "[ss/ref] validate_full_game_state" ) {
  SS ss;
  RootState& root = ss.root;

  // Patches to satisfy validation.
  root.land_view.viewport.zoom = 1.0;

  auto const f = [&] [[clang::noinline]] {
    return ss.as_const.validate_full_game_state();
  };

  REQUIRE( f() == valid );

  root.zzz_terrain.modify_entire_map( []( RealTerrain& trn ) {
    trn.map = MapMatrix( size{ .w = 2, .h = 2 } );
  } );
  REQUIRE( f() == valid );

  root.zzz_terrain.mutable_square_at( point{ .x = 0, .y = 1 } ) =
      MapSquare{ .surface = e_surface::water, .road = false };
  REQUIRE( f() == valid );

  root.zzz_terrain.mutable_square_at( point{ .x = 0, .y = 1 } ) =
      MapSquare{ .surface = e_surface::water, .road = true };
  REQUIRE_THAT( f().error(),
                Contains( "water tiles cannot have a road" ) );

  root.turn.time_point.year = -1;
  REQUIRE_THAT( f().error(),
                Contains( "game year must be >= 0" ) );

  root.version.major = -1;
  REQUIRE_THAT(
      f().error(),
      Contains( "major version number must be >= 0" ) );
}

TEST_CASE( "[ss/ref] validate_non_terrain_game_state" ) {
  SS ss;
  RootState& root = ss.root;

  // Patches to satisfy validation.
  root.land_view.viewport.zoom = 1.0;

  auto const f = [&] [[clang::noinline]] {
    return ss.as_const.validate_non_terrain_game_state();
  };

  REQUIRE( f() == valid );

  root.zzz_terrain.modify_entire_map( []( RealTerrain& trn ) {
    trn.map = MapMatrix( size{ .w = 2, .h = 2 } );
  } );
  REQUIRE( f() == valid );

  root.zzz_terrain.mutable_square_at( point{ .x = 0, .y = 1 } ) =
      MapSquare{ .surface = e_surface::water, .road = false };
  REQUIRE( f() == valid );

  root.zzz_terrain.mutable_square_at( point{ .x = 0, .y = 1 } ) =
      MapSquare{ .surface = e_surface::water, .road = true };
  // NOTE: terrain validation is not run.
  REQUIRE( f() == valid );

  root.turn.time_point.year = -1;
  REQUIRE_THAT( f().error(),
                Contains( "game year must be >= 0" ) );

  root.version.major = -1;
  REQUIRE_THAT(
      f().error(),
      Contains( "major version number must be >= 0" ) );
}

} // namespace
} // namespace rn
