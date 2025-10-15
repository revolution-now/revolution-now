/****************************************************************
**camera-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-06-19.
*
* Description: Unit tests for the camera module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/camera.hpp"

// Testing.
#include "test/fake/world.hpp"
#include "test/mocks/iuser-config.hpp"

// config
#include "src/config/user.rds.hpp"

// ss
#include "src/ss/land-view.rds.hpp"
#include "src/ss/terrain.hpp"

// refl
#include "src/refl/to-str.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

using ::gfx::size;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct world : testing::World {
  world() {
    add_default_player();

    mock_user_config.EXPECT__read().by_default().returns(
        config_user );
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

  config_user_t config_user;
  MockIUserConfig mock_user_config;
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[camera] zoom_in/zoom_out" ) {
  world w;
  w.create_default_map();
  Camera camera( w.mock_user_config, w.land_view().viewport,
                 w.terrain().world_size_tiles() );

  ZoomChanged ex;

  auto& zoom = w.land_view().viewport.zoom;
  zoom       = 1.0;

  REQUIRE( zoom == 1.0 );

  ex = { .value_changed = false, .bucket_changed = false };
  REQUIRE( camera.zoom_in() == ex );
  REQUIRE( zoom == 1.0 );

  w.config_user.camera.can_zoom_positive = true;

  ex = { .value_changed = true, .bucket_changed = true };
  REQUIRE( camera.zoom_in() == ex );
  REQUIRE( zoom == 2.0 );

  ex = { .value_changed = false, .bucket_changed = false };
  REQUIRE( camera.zoom_in() == ex );
  REQUIRE( zoom == 2.0 );

  zoom = 0.8;
  ex   = { .value_changed = true, .bucket_changed = true };
  REQUIRE( camera.zoom_in() == ex );
  REQUIRE( zoom == 1.0 );

  zoom = 0.9;
  ex   = { .value_changed = true, .bucket_changed = true };
  REQUIRE( camera.zoom_in() == ex );
  REQUIRE( zoom == 2.0 );

  zoom = 1.2;
  ex   = { .value_changed = true, .bucket_changed = true };
  REQUIRE( camera.zoom_in() == ex );
  REQUIRE( zoom == 2.0 );

  ex = { .value_changed = false, .bucket_changed = false };
  REQUIRE( camera.zoom_in() == ex );
  REQUIRE( zoom == 2.0 );

  zoom = 19;
  ex   = { .value_changed = true, .bucket_changed = true };
  REQUIRE( camera.zoom_out() == ex );
  REQUIRE( zoom == 2.0 );

  ex = { .value_changed = true, .bucket_changed = true };
  REQUIRE( camera.zoom_out() == ex );
  REQUIRE( zoom == 1.0 );

  zoom = 3.5;
  ex   = { .value_changed = true, .bucket_changed = true };
  REQUIRE( camera.zoom_out() == ex );
  REQUIRE( zoom == 2.0 );

  zoom = 1.2;
  ex   = { .value_changed = true, .bucket_changed = true };
  REQUIRE( camera.zoom_out() == ex );
  REQUIRE( zoom == 1.0 );

  zoom = 1.05;
  ex   = { .value_changed = true, .bucket_changed = true };
  REQUIRE( camera.zoom_out() == ex );
  REQUIRE( zoom == 0.5 );

  ex = { .value_changed = true, .bucket_changed = true };
  REQUIRE( camera.zoom_out() == ex );
  REQUIRE( zoom == 0.25 );

  zoom = 0.4;
  ex   = { .value_changed = true, .bucket_changed = true };
  REQUIRE( camera.zoom_out() == ex );
  REQUIRE( zoom == 0.25 );

  ex = { .value_changed = true, .bucket_changed = true };
  REQUIRE( camera.zoom_out() == ex );
  REQUIRE( zoom == 0.125 );

  ex = { .value_changed = true, .bucket_changed = true };
  REQUIRE( camera.zoom_out() == ex );
  REQUIRE( zoom == 0.0625 );

  zoom = 0.0800;
  ex   = { .value_changed = true, .bucket_changed = true };
  REQUIRE( camera.zoom_out() == ex );
  REQUIRE( zoom == 0.0625 );

  ex = { .value_changed = true, .bucket_changed = true };
  REQUIRE( camera.zoom_out() == ex );
  REQUIRE( zoom == 0.03125 );

  ex = { .value_changed = true, .bucket_changed = true };
  REQUIRE( camera.zoom_out() == ex );
  REQUIRE( zoom == 0.015625 );

  ex = { .value_changed = true, .bucket_changed = true };
  REQUIRE( camera.zoom_out() == ex );
  REQUIRE( zoom == 0.0078125 );

  ex = { .value_changed = true, .bucket_changed = true };
  REQUIRE( camera.zoom_out() == ex );
  REQUIRE( zoom == 0.00390625 );

  ex = { .value_changed = false, .bucket_changed = false };
  REQUIRE( camera.zoom_out() == ex );
  REQUIRE( zoom == 0.00390625 );
}

TEST_CASE( "[camera] center_on_tile" ) {
  world w;
  w.create_default_map();
  Camera camera( w.mock_user_config, w.land_view().viewport,
                 w.terrain().world_size_tiles() );

  auto& center_x = w.land_view().viewport.center_x;
  auto& center_y = w.land_view().viewport.center_y;

  REQUIRE( center_x == 0 );
  REQUIRE( center_y == 0 );

  camera.center_on_tile( { .x = 5, .y = 8 } );
  REQUIRE( center_x == 5 * 32 + 16 );
  REQUIRE( center_y == 8 * 32 + 16 );

  camera.center_on_tile( { .x = 0, .y = 0 } );
  REQUIRE( center_x == 0 * 32 + 16 );
  REQUIRE( center_y == 0 * 32 + 16 );
}

TEST_CASE( "[camera] map_dimensions_tiles" ) {
  world w;
  w.create_default_map();
  Camera camera( w.mock_user_config, w.land_view().viewport,
                 w.terrain().world_size_tiles() );

  REQUIRE( camera.map_dimensions_tiles() ==
           size{ .w = 3, .h = 3 } );
}

} // namespace
} // namespace rn
