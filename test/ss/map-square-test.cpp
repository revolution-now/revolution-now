/****************************************************************
**map-square-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2026-04-03.
*
* Description: Unit tests for the ss/map-square module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/ss/map-square.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

using ::base::invalid;
using ::base::valid;
using ::Catch::Contains;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[ss/map-square] validate" ) {
  using enum e_surface;
  using enum e_land_overlay;
  using enum e_river;
  using enum e_biome;
  using enum e_natural_resource;

  MapSquare o;

  auto const f = [&] [[clang::noinline]] {
    return o.validate();
  };

  REQUIRE( f() == valid );

  SECTION( "water" ) {
    o.surface = water;
    SECTION( "lcr" ) {
      o.lost_city_rumor = true;
      REQUIRE_THAT(
          f().error(),
          Contains(
              "water tiles cannot have a lost city rumor." ) );
    }
    SECTION( "road" ) {
      o.road = true;
      REQUIRE_THAT(
          f().error(),
          Contains( "water tiles cannot have a road." ) );
    }
    SECTION( "irrigation" ) {
      o.irrigation = true;
      REQUIRE_THAT(
          f().error(),
          Contains( "water tiles cannot have irrigation." ) );
    }
    SECTION( "hills" ) {
      o.overlay = hills;
      REQUIRE_THAT(
          f().error(),
          Contains( "water tiles cannot have an overlay "
                    "(forest, mountains, or hills)." ) );
    }
    SECTION( "mountains" ) {
      o.overlay = mountains;
      REQUIRE_THAT(
          f().error(),
          Contains( "water tiles cannot have an overlay "
                    "(forest, mountains, or hills)." ) );
    }
    SECTION( "forest" ) {
      o.overlay = forest;
      REQUIRE_THAT(
          f().error(),
          Contains( "water tiles cannot have an overlay "
                    "(forest, mountains, or hills)." ) );
    }
    SECTION( "ground resource non-fish" ) {
      o.ground_resource = deer;
      REQUIRE_THAT(
          f().error(),
          Contains( "the only prime resource available on water "
                    "tiles is fish." ) );
    }
    SECTION( "ground resource fish" ) {
      o.ground_resource = fish;
      REQUIRE( f() == valid );
    }
    SECTION( "forest resource" ) {
      o.forest_resource = deer;
      REQUIRE_THAT( f().error(),
                    Contains( "water tiles cannot have a prime "
                              "forest resource." ) );
    }
    SECTION( "forest resource fish" ) {
      o.forest_resource = fish;
      REQUIRE_THAT( f().error(),
                    Contains( "water tiles cannot have a prime "
                              "forest resource." ) );
    }
  }

  SECTION( "land" ) {
    o.surface = land;
    SECTION( "forest on arctic" ) {
      o.ground  = arctic;
      o.overlay = forest;
      REQUIRE_THAT(
          f().error(),
          Contains( "cannot have forest on an arctic tile." ) );
    }
    SECTION( "sea lane" ) {
      o.sea_lane = true;
      REQUIRE_THAT(
          f().error(),
          Contains( "cannot have Sea Lane on land tiles." ) );
    }
    SECTION( "ground fish resource" ) {
      o.ground_resource = fish;
      REQUIRE_THAT( f().error(),
                    Contains( "prime fishing resource not "
                              "allowed on land tiles" ) );
    }
    SECTION( "forest fish resource" ) {
      o.forest_resource = fish;
      REQUIRE_THAT( f().error(),
                    Contains( "prime fishing resource not "
                              "allowed on forest/land tiles" ) );
    }
    SECTION( "minor river" ) {
      o.river = minor;
      SECTION( "hills" ) {
        o.overlay = hills;
        REQUIRE( f() == valid );
      }
      SECTION( "mountains" ) {
        o.overlay = mountains;
        REQUIRE_THAT(
            f().error(),
            Contains( "cannot have a minor river and a mountain "
                      "on the same tile." ) );
      }
    }
    SECTION( "major river" ) {
      o.river = major;
      SECTION( "hills" ) {
        o.overlay = hills;
        REQUIRE_THAT( f().error(),
                      Contains( "cannot have a major river and "
                                "hills on the same tile." ) );
      }
      SECTION( "mountains" ) {
        o.overlay = mountains;
        REQUIRE( f() == valid );
      }
    }
  }
}

} // namespace
} // namespace rn
